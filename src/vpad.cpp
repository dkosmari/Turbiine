/*
 * Turbiine - Turn any controller into a turbo controller.
 *
 * Copyright (C) 2025  Daniel K. O.
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

#include "vpad.hpp"

#include "cfg.hpp"
#include "notify.hpp"


using std::array;
using std::chrono::milliseconds;
using std::int32_t;
using std::uint32_t;
using std::uint8_t;
using std::views::enumerate;

namespace logger = wups::logger;


namespace vpad {

    constexpr unsigned max_vpads = 2;


    struct button_info_t {
        VPADButtons button;
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
        uint32_t turbo = 0;
        uint32_t fake_hold = 0;
        uint32_t suppressed = 0;
        array<OSTime, num_buttons> last_turbo_action{};
        bool     toggling = false;


        void
        reset()
            noexcept
        {
            turbo = 0;
            fake_hold = 0;
            suppressed = 0;
            last_turbo_action.fill(0);
            toggling = false;
        }


        bool
        flip_toggling()
            noexcept
        {
            return toggling = !toggling;
        }


        void
        process_vpad_read(VPADChan channel,
                          VPADStatus& status,
                          OSTime period)
        {
            for (auto [idx, info] : enumerate(button_list)) {
                auto [btn, btn_name, btn_glyph] = info;
                const auto not_btn = ~uint32_t{btn};

                // Button suppression logic.
                if (suppressed & btn) {
                    // if the button is not held, or was released, we stop suppressing it
                    if (!(status.hold & btn) || (status.release & btn))
                        suppressed &= not_btn;

                    status.hold    &= not_btn;
                    status.trigger &= not_btn;
                    status.release &= not_btn;

                    continue;
                }

                // Turbo toggling logic.
                if (toggling && (status.trigger & btn)) {

                    toggling = false;
                    turbo ^= btn;

                    const char* on_off = turbo & btn ? "turbo" : "normal";

                    notify::info("vpad %d button %s is %s",
                                 int(channel),
                                 btn_glyph,
                                 on_off);

                    // Hide this button event from the game.
                    status.hold    &= not_btn;
                    status.trigger &= not_btn;
                    status.release &= not_btn;

                    // This button will be suppressed until a release event happens.
                    suppressed |= btn;

                    last_turbo_action[idx] = 0;

                    continue;

                }

                // Early out: nothing else to process if button is not held.
                if (!(status.hold & btn)) {
                    fake_hold &= not_btn;
                    continue;
                }

                OSTime now = OSGetSystemTime();

                if (status.trigger & btn) {
                    // Button was just pressed.
                    fake_hold |= btn;
                    last_turbo_action[idx] = now;
                }

                // If button is held and turbinated, generate fake presses.
                if (turbo & btn) {
                    OSTime age = now - last_turbo_action[idx];
                    if (age >= period) {
                        last_turbo_action[idx] = now;
                        fake_hold ^= btn;
                        if (fake_hold & btn) {
                            // simulate a press event
                            status.hold    |= btn;
                            status.trigger |= btn;
                            status.release &= not_btn;
                        } else {
                            // simulate a release event
                            status.hold    &= not_btn;
                            status.trigger &= not_btn;
                            status.release |= btn;
                        }
                    }
                } // if turbo action

            } // for each button
        }

    };


    array<vpad_state, max_vpads> states;


    // Reset all variables.
    void
    reset()
    {
        logger::printf("Resetting turbo state for vpads\n");
        for (auto& st : states)
            st.reset();
    }


    void
    on_toggle(VPADChan channel)
    {
        if (states[channel].flip_toggling())
            notify::info("Toggling turbo on vpad %d...", int(channel));
        else
            notify::info("Canceled turbo toggle on vpad %d.", int(channel));
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

        OSTime period = OSMillisecondsToTicks(cfg::period.value.count());
        auto& state = states[channel];

        bool is_loose = !VPADGetButtonProcMode(channel);
        int real_count = is_loose ? 1 : count;

        // Process all samples from oldest (back) to newest (front).
        for (int idx = real_count - 1; idx >= 0; --idx)
            state.process_vpad_read(channel, status[idx], period);

        if (is_loose) {
            // Every sample should have the same button state.
            for (int idx = 1; idx < result; ++idx) {
                status[idx].hold    = status[0].hold;
                status[idx].trigger = status[0].trigger;
                status[idx].release = status[0].release;
            }
        }

        return result;
    }


    WUPS_MUST_REPLACE(VPADRead, WUPS_LOADER_LIBRARY_VPAD, VPADRead);

} // namespace vpad
