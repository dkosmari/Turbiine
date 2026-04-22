/*
 * Turbiine - Turn any controller into a turbo controller.
 *
 * Copyright (C) 2025-2026  Daniel K. O.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <array>
#include <cstdint>
#include <cstdio>
#include <ranges>

#include <coreinit/time.h>

#include <wups/function_patching.h>

#include <wupsxx/cafe_glyphs.h>
#include <wupsxx/logger.hpp>
#include <wupsxx/notify.hpp>

#include "vpad.hpp"

#include "cfg.hpp"
#include "turbo_toggling.hpp"


using std::array;
using std::chrono::milliseconds;
using std::int32_t;
using std::uint32_t;
using std::uint8_t;
using std::views::enumerate;

namespace logger = wups::logger;
namespace notify = wups::notify;

namespace vpad {

    constexpr unsigned max_vpads = 2;

    struct button_info_t {
        uint32_t button;
        const char* name;
        const char* glyph;
    };

    constexpr button_info_t button_list[] = {
        { VPAD_BUTTON_UP,    "VPAD_BUTTON_UP",    CAFE_GLYPH_GAMEPAD_BTN_UP    },
        { VPAD_BUTTON_DOWN,  "VPAD_BUTTON_DOWN",  CAFE_GLYPH_GAMEPAD_BTN_DOWN  },
        { VPAD_BUTTON_LEFT,  "VPAD_BUTTON_LEFT",  CAFE_GLYPH_GAMEPAD_BTN_LEFT  },
        { VPAD_BUTTON_RIGHT, "VPAD_BUTTON_RIGHT", CAFE_GLYPH_GAMEPAD_BTN_RIGHT },
        { VPAD_BUTTON_A,     "VPAD_BUTTON_A",     CAFE_GLYPH_GAMEPAD_BTN_A     },
        { VPAD_BUTTON_B,     "VPAD_BUTTON_B",     CAFE_GLYPH_GAMEPAD_BTN_B     },
        { VPAD_BUTTON_X,     "VPAD_BUTTON_X",     CAFE_GLYPH_GAMEPAD_BTN_X     },
        { VPAD_BUTTON_Y,     "VPAD_BUTTON_Y",     CAFE_GLYPH_GAMEPAD_BTN_Y     },
        { VPAD_BUTTON_L,     "VPAD_BUTTON_L",     CAFE_GLYPH_GAMEPAD_BTN_L     },
        { VPAD_BUTTON_R,     "VPAD_BUTTON_R",     CAFE_GLYPH_GAMEPAD_BTN_R     },
        { VPAD_BUTTON_ZL,    "VPAD_BUTTON_ZL",    CAFE_GLYPH_GAMEPAD_BTN_ZL    },
        { VPAD_BUTTON_ZR,    "VPAD_BUTTON_ZR",    CAFE_GLYPH_GAMEPAD_BTN_ZR    },
        { VPAD_BUTTON_PLUS,  "VPAD_BUTTON_PLUS",  CAFE_GLYPH_GAMEPAD_BTN_PLUS  },
        { VPAD_BUTTON_MINUS, "VPAD_BUTTON_MINUS", CAFE_GLYPH_GAMEPAD_BTN_MINUS },
    };

    constexpr unsigned num_buttons = std::size(button_list);


    struct vpad_state {
        array<OSTime, num_buttons> last_turbo_action{};
        uint32_t turbinated  = 0;
        // NOTE: During turbo action, the button bit in `turbo_hold' gets toggled.
        uint32_t turbo_hold  = 0;
        // NOTE: Right after a button is toggled, we don't start doing turbo action until
        // it's released. The corresponding bit in `suppressed' is used to track if turbo
        // action should be skipped, because that button has not been released yet, since
        // the toggle event.
        uint32_t suppressed = 0;
        turbo_toggling toggling = turbo_toggling::normal;

        void
        reset()
            noexcept
        {
            last_turbo_action.fill(0);
            turbinated  = 0;
            turbo_hold  = 0;
            suppressed = 0;
            toggling   = turbo_toggling::normal;
        }


        void
        process_vpad_read(VPADChan channel,
                          VPADStatus& status,
                          OSTime period,
                          OSTime now)
        {
            // Turbo toggling logic: if all buttons were released we can transition
            // waiting -> reading.
            if (toggling == turbo_toggling::waiting && status.hold == 0)
                toggling = turbo_toggling::reading;

            for (auto [idx, info] : enumerate(button_list)) {
                auto [btn_flag, btn_name, btn_glyph] = info;

                // Button suppression logic.
                if (suppressed & btn_flag) [[unlikely]] {

                    // if the button is not held, or was released, we stop suppressing it
                    if (!(status.hold & btn_flag) || (status.release & btn_flag))
                        suppressed &= ~btn_flag;

                    status.hold    &= ~btn_flag;
                    status.trigger &= ~btn_flag;
                    status.release &= ~btn_flag;

                    // NOTE: We skip all other button processing for suppressed buttons.
                    continue;

                }

                // Turbo toggling logic: from `reading' state we check for triggers.
                if (toggling == turbo_toggling::reading
                    && status.trigger & btn_flag) [[unlikely]] {

                    toggling = turbo_toggling::normal;
                    turbinated ^= btn_flag;

                    const char* on_off = turbinated & btn_flag ? "turbo" : "normal";

                    logger::printf("VPAD %d button %s is %s\n",
                                   int(channel),
                                   btn_name,
                                   on_off);
                    notify::info::show("Gamepad %d button %s is %s",
                                       int(channel) + 1,
                                       btn_glyph,
                                       on_off);

                    // Hide this button event from the game.
                    status.hold    &= ~btn_flag;
                    status.trigger &= ~btn_flag;
                    status.release &= ~btn_flag;

                    // This button will be suppressed until a release event happens.
                    suppressed |= btn_flag;

                    last_turbo_action[idx] = 0;

                    // NOTE: We skip further processing if the button was just toggled.
                    continue;

                }

                if (status.trigger & btn_flag) [[unlikely]] {
                    // Button was just pressed.
                    turbo_hold |= btn_flag;
                    last_turbo_action[idx] = now;
                    continue;
                }

                if (status.release & btn_flag) [[unlikely]] {
                    // Button was just released.
                    turbo_hold &= ~btn_flag;
                    last_turbo_action[idx] = 0;
                    continue;
                }

                // If button is held and turbinated, do turbo action.
                if (status.hold & btn_flag && turbinated & btn_flag) {
                    OSTime age = now - last_turbo_action[idx];
                    if (age >= period) {
                        // time to generate turbo events
                        last_turbo_action[idx] = now;
                        turbo_hold ^= btn_flag;
                        if (turbo_hold & btn_flag) {
                            // simulate a press event
                            status.hold    |=  btn_flag;
                            status.trigger |=  btn_flag;
                            status.release &= ~btn_flag;
                        } else {
                            // simulate a release event
                            status.hold    &= ~btn_flag;
                            status.trigger &= ~btn_flag;
                            status.release |=  btn_flag;
                        }
                    } else {
                        // in between turbo events, just copy turbo_hold
                        status.hold = (status.hold & ~btn_flag) | (turbo_hold & btn_flag);
                    }
                } // if turbo action

            } // for each button
        }

    }; // struct vpad_state


    array<vpad_state, max_vpads> states;


    // Reset all variables.
    void
    reset()
    {
        logger::printf("Resetting vpads\n");
        for (auto& st : states)
            st.reset();
    }


    void
    on_toggle(VPADChan channel)
    {
        switch (states[channel].toggling) {
            using enum turbo_toggling;

            case normal:
                notify::info::show("Toggling turbo on gamepad %d...", int(channel) + 1);
                states[channel].toggling = waiting;
                break;

            case waiting:
            case reading:
                notify::info::show("Canceled turbo toggle on gamepad %d.", int(channel) + 1);
                states[channel].toggling = normal;
                break;
        }
    }


    DECL_FUNCTION(int32_t,
                  VPADRead,
                  VPADChan channel,
                  VPADStatus* status,
                  uint32_t count,
                  VPADReadError* error)
    {
        int32_t result = real_VPADRead(channel, status, count, error);
        if (error && *error)
            return result;
        if (result < 1)
            return result;
        if (!cfg::enabled.value)
            return result;
        if (!status) [[unlikely]]
            return result;
        if (channel < 0 || channel >= states.size()) [[unlikely]]
            return result;

        OSTime now = OSGetSystemTime();
        OSTime period = OSMillisecondsToTicks(cfg::period.value.count());
        auto& state = states[channel];

        bool is_loose = !VPADGetButtonProcMode(channel);
        if (is_loose) {
            // Every sample has the same button state, so we only care about the first.
            state.process_vpad_read(channel, status[0], period, now);
            // Copy modified button state to the rest of the buffer.
            for (int idx = 1; idx < result; ++idx) {
                status[idx].hold    = status[0].hold;
                status[idx].trigger = status[0].trigger;
                status[idx].release = status[0].release;
            }
        } else {
            // Every sample has different button state, process from oldest (back) to
            // newest (front).
            for (int idx = result - 1; idx >= 0; --idx)
                state.process_vpad_read(channel, status[idx], period, now);
        }

        return result;
    }


    WUPS_MUST_REPLACE(VPADRead, WUPS_LOADER_LIBRARY_VPAD, VPADRead);

} // namespace vpad
