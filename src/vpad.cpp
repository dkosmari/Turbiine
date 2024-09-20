/*
 * Turbiine - Turn any controller into a turbo controller.
 *
 * Copyright (C) 2024  Daniel K. O.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <array>
#include <bitset>
#include <cstdint>
#include <cstdio>
#include <ranges>

#include <vpad/input.h>

#include <wups/function_patching.h>

#include <wupsxx/button_combo.hpp>
#include <wupsxx/logger.hpp>

#include "vpad.hpp"

#include "cfg.hpp"
#include "notify.hpp"


using std::array;
using std::int32_t;
using std::uint32_t;
using std::uint8_t;
using std::views::enumerate;

using wups::utils::button_combo;

namespace logger = wups::logger;


namespace vpad {

    constexpr unsigned max_vpads = 2;


    constexpr array button_list = {
        VPAD_BUTTON_A,
        VPAD_BUTTON_B,
        VPAD_BUTTON_X,
        VPAD_BUTTON_Y,
        VPAD_BUTTON_LEFT,
        VPAD_BUTTON_RIGHT,
        VPAD_BUTTON_UP,
        VPAD_BUTTON_DOWN,
        VPAD_BUTTON_L,
        VPAD_BUTTON_ZL,
        VPAD_BUTTON_R,
        VPAD_BUTTON_ZR,
        VPAD_BUTTON_PLUS,
        VPAD_BUTTON_MINUS,
    };

    constexpr unsigned max_buttons = button_list.size();

    using button_set = std::bitset<max_buttons>;

    struct pad_state_t {
        button_set turbo;
        button_set fake_hold;
        button_set suppress;
        array<uint8_t, max_buttons> age{};
        bool       toggling = false;
    };

    array<pad_state_t, max_vpads> state;


    // reset all variables
    void
    reset()
    {
        logger::printf("Resetting vpads\n");
        state.fill({});
    }


    void
    toggle_button(pad_state_t& pad,
                  unsigned idx,
                  VPADChan channel)
    {
        pad.toggling = false;
        pad.turbo.flip(idx);

        auto btn = button_list[idx];

        wups::utils::vpad::button_set bs{btn};

        const char* on_off = pad.turbo.test(idx) ? "turbo" : "normal";
        notify::info("%s = %s",
                     to_glyph(bs).c_str(),
                     on_off);

        logger::printf("vpad %u button %s [%u] = %s\n",
                       unsigned{channel},
                       to_string(bs).c_str(),
                       idx,
                       on_off);
    }


    void
    run_turbo_logic(pad_state_t& pad,
                    VPADStatus& status,
                    VPADChan channel)
    {
        for (auto [idx, btn] : enumerate(button_list)) {

            const auto not_btn = ~uint32_t{btn};

            // If this is a suppressed button, don't process it, keep it clear and
            // skip further turbo processing.
            if (pad.suppress.test(idx)) {
                // if the button is not held, or was released, we stop suppressing it
                if (!(status.hold & btn) || (status.release & btn))
                    pad.suppress.reset(idx);

                status.hold    &= not_btn;
                status.trigger &= not_btn;
                status.release &= not_btn;

                continue;
            }

            if (pad.toggling && (status.trigger & btn)) {

                // We're in the toggling state, and a button was triggered.
                toggle_button(pad, idx, channel);

                // Hide this event from the game.
                status.hold    &= not_btn;
                status.trigger &= not_btn;
                status.release &= not_btn;

                pad.fake_hold.reset(idx);

                // This button will be suppressed until a release event happens.
                pad.suppress.set(idx);

                pad.age[idx] = 0;

            } else {

                // We're not in the toggling state, just check if it's a turbinated button
                // held down.

                bool turbinated = pad.turbo.test(idx);
                if (turbinated && (status.hold & btn)) {

                    if (++pad.age[idx] >= cfg::period) {

                        pad.fake_hold.flip(idx);
                        pad.age[idx] = 0;

                        if (pad.fake_hold.test(idx)) {
                            // simulate a press event
                            status.trigger |= btn;
                            status.release &= not_btn;
                        } else {
                            // simulate a release event
                            status.hold    &= not_btn;
                            status.trigger &= not_btn;
                            status.release |= btn;
                        }

                    }

                } else // if no turbo action, just copy the real button state
                    pad.fake_hold.set(idx, status.hold & btn);

            }
        }
    }


    DECL_FUNCTION(int32_t,
                  VPADRead,
                  VPADChan channel,
                  VPADStatus* buf,
                  uint32_t count,
                  VPADReadError* error)
    {
        int32_t result = real_VPADRead(channel, buf, count, error);
        if (error && *error) [[unlikely]]
            return result;
        if (!cfg::enabled)
            return result;
        if (!buf) [[unlikely]]
            return result;

        auto& pad = state[channel];

        bool is_loose = !VPADGetButtonProcMode(channel);
        int32_t real_count = is_loose ? 1 : result;
        for (int32_t idx = real_count - 1; idx >= 0; --idx) {
            VPADStatus& status = buf[idx];
            if (wups::utils::vpad::update(channel, status)) {

                bool combo_activated = false;
                for (const auto& combo : cfg::toggle_combo)
                    if (wups::utils::vpad::triggered(channel, combo)) {
                        combo_activated = true;
                        break;
                    }

                // Note: when a combo is activated, don't do any turbo processing.
                if (combo_activated) [[unlikely]] {

                    // Enter or leave toggling state.
                    pad.toggling = !pad.toggling;

                    notify::info(pad.toggling
                                 ? "Toggling turbo..."
                                 : "Canceled turbo toggle.");

                    // Keep all held buttons suppressed.
                    for (auto [btn_idx, btn] : enumerate(button_list))
                        if (status.hold & btn)
                            pad.suppress.set(btn_idx);

                    // Discard all buttons.

                    // Note: buttons that were triggered right now are not released, since
                    // their trigger event will never be recorded. We only release the
                    // buttons that were being held before the trigger.
                    status.release = status.hold ^ status.trigger;
                    status.hold = 0;
                    status.trigger = 0;

                } else [[likely]]
                    try {
                        run_turbo_logic(pad, status, channel);
                    }
                    catch (std::exception& e) {
                        logger::printf("Error running VPAD turbo logic: %s\n", e.what());
                    }

            }

        }

        if (is_loose) {
            // Every sample in buf should have the same button state.
            for (int32_t idx = 1; idx < result; ++idx) {
                buf[idx].hold = buf[0].hold;
                buf[idx].trigger = buf[0].trigger;
                buf[idx].release = buf[0].release;
            }
        }

        return result;
    }


    WUPS_MUST_REPLACE(VPADRead, WUPS_LOADER_LIBRARY_VPAD, VPADRead);

} // namespace vpad
