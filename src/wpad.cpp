/*
 * Turbiine - Turn any controller into a turbo controller.
 *
 * Copyright (C) 2025  Daniel K. O.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <algorithm>
#include <array>
#include <cstdint>
#include <ranges>

#include <coreinit/time.h>

#include <wups/function_patching.h>

#include <wupsxx/cafe_glyphs.h>
#include <wupsxx/logger.hpp>

#include "wpad.hpp"

#include "cfg.hpp"
#include "notify.hpp"


using std::array;
using std::int32_t;
using std::uint32_t;
using std::uint8_t;
using std::views::enumerate;

namespace logger = wups::logger;


namespace wpad {

    constexpr unsigned max_wpads = 7;


    // Class to keep track of trigger/release bits.
    struct aux_t {

        uint32_t trigger = 0;
        uint32_t hold    = 0;
        uint32_t release = 0;


        void
        reset()
            noexcept
        {
            trigger = 0;
            hold    = 0;
            release = 0;
        }


        void
        update(uint32_t buttons)
            noexcept
        {
            uint32_t changed = hold ^ buttons;
            hold = buttons;
            trigger = changed &  buttons;
            release = changed & ~buttons;
        }

    };


    namespace core {

        struct button_info_t {
            WPADButton button;
            const char* name;
            const char* glyph;
        };

        constexpr button_info_t button_list[] = {
            { WPAD_BUTTON_UP,    "WPAD_BUTTON_UP",    CAFE_GLYPH_WIIMOTE_BTN_UP    },
            { WPAD_BUTTON_DOWN,  "WPAD_BUTTON_DOWN",  CAFE_GLYPH_WIIMOTE_BTN_DOWN  },
            { WPAD_BUTTON_LEFT,  "WPAD_BUTTON_LEFT",  CAFE_GLYPH_WIIMOTE_BTN_LEFT  },
            { WPAD_BUTTON_RIGHT, "WPAD_BUTTON_RIGHT", CAFE_GLYPH_WIIMOTE_BTN_RIGHT },
            { WPAD_BUTTON_A,     "WPAD_BUTTON_A",     CAFE_GLYPH_WIIMOTE_BTN_A     },
            { WPAD_BUTTON_B,     "WPAD_BUTTON_B",     CAFE_GLYPH_WIIMOTE_BTN_B     },
            { WPAD_BUTTON_PLUS,  "WPAD_BUTTON_PLUS",  CAFE_GLYPH_WIIMOTE_BTN_PLUS  },
            { WPAD_BUTTON_MINUS, "WPAD_BUTTON_MINUS", CAFE_GLYPH_WIIMOTE_BTN_MINUS },
            { WPAD_BUTTON_1,     "WPAD_BUTTON_1",     CAFE_GLYPH_WIIMOTE_BTN_1     },
            { WPAD_BUTTON_2,     "WPAD_BUTTON_2",     CAFE_GLYPH_WIIMOTE_BTN_2     },
        };

        constexpr unsigned num_buttons = std::size(button_list);

    } // namespace core


    namespace ext {

        namespace nunchuk {

            struct button_info_t {
                WPADNunchukButton button;
                const char* name;
                const char* glyph;
            };

            constexpr button_info_t button_list[] = {
                { WPAD_NUNCHUK_BUTTON_C, "WPAD_NUNCHUK_BUTTON_C", CAFE_GLYPH_NUNCHUK_BTN_C },
                { WPAD_NUNCHUK_BUTTON_Z, "WPAD_NUNCHUK_BUTTON_Z", CAFE_GLYPH_NUNCHUK_BTN_Z },
            };

            constexpr unsigned num_buttons = std::size(button_list);

        } // namespace nunchuk


        namespace classic {

            struct button_info_t {
                WPADClassicButton button;
                const char* name;
                const char* glyph;
            };

            constexpr button_info_t button_list[] = {
                { WPAD_CLASSIC_BUTTON_UP,    "WPAD_CLASSIC_BUTTON_UP",    CAFE_GLYPH_CLASSIC_BTN_UP    },
                { WPAD_CLASSIC_BUTTON_DOWN,  "WPAD_CLASSIC_BUTTON_DOWN",  CAFE_GLYPH_CLASSIC_BTN_DOWN  },
                { WPAD_CLASSIC_BUTTON_LEFT,  "WPAD_CLASSIC_BUTTON_LEFT",  CAFE_GLYPH_CLASSIC_BTN_LEFT  },
                { WPAD_CLASSIC_BUTTON_RIGHT, "WPAD_CLASSIC_BUTTON_RIGHT", CAFE_GLYPH_CLASSIC_BTN_RIGHT },
                { WPAD_CLASSIC_BUTTON_A,     "WPAD_CLASSIC_BUTTON_A",     CAFE_GLYPH_CLASSIC_BTN_A     },
                { WPAD_CLASSIC_BUTTON_B,     "WPAD_CLASSIC_BUTTON_B",     CAFE_GLYPH_CLASSIC_BTN_B     },
                { WPAD_CLASSIC_BUTTON_X,     "WPAD_CLASSIC_BUTTON_X",     CAFE_GLYPH_CLASSIC_BTN_X     },
                { WPAD_CLASSIC_BUTTON_Y,     "WPAD_CLASSIC_BUTTON_Y",     CAFE_GLYPH_CLASSIC_BTN_Y     },
                { WPAD_CLASSIC_BUTTON_L,     "WPAD_CLASSIC_BUTTON_L",     CAFE_GLYPH_CLASSIC_BTN_L     },
                { WPAD_CLASSIC_BUTTON_R,     "WPAD_CLASSIC_BUTTON_R",     CAFE_GLYPH_CLASSIC_BTN_R     },
                { WPAD_CLASSIC_BUTTON_ZL,    "WPAD_CLASSIC_BUTTON_ZL",    CAFE_GLYPH_CLASSIC_BTN_ZL    },
                { WPAD_CLASSIC_BUTTON_ZR,    "WPAD_CLASSIC_BUTTON_ZR",    CAFE_GLYPH_CLASSIC_BTN_ZR    },
                { WPAD_CLASSIC_BUTTON_PLUS,  "WPAD_CLASSIC_BUTTON_PLUS",  CAFE_GLYPH_CLASSIC_BTN_PLUS  },
                { WPAD_CLASSIC_BUTTON_MINUS, "WPAD_CLASSIC_BUTTON_MINUS", CAFE_GLYPH_CLASSIC_BTN_MINUS },
            };

            constexpr unsigned num_buttons = std::size(button_list);

        } // namespace classic


        namespace pro {

            struct button_info_t {
                WPADProButton button;
                const char* name;
                const char* glyph;
            };

            constexpr button_info_t button_list[] = {
                { WPAD_PRO_BUTTON_UP,    "WPAD_PRO_BUTTON_UP",    CAFE_GLYPH_PRO_BTN_UP    },
                { WPAD_PRO_BUTTON_DOWN,  "WPAD_PRO_BUTTON_DOWN",  CAFE_GLYPH_PRO_BTN_DOWN  },
                { WPAD_PRO_BUTTON_LEFT,  "WPAD_PRO_BUTTON_LEFT",  CAFE_GLYPH_PRO_BTN_LEFT  },
                { WPAD_PRO_BUTTON_RIGHT, "WPAD_PRO_BUTTON_RIGHT", CAFE_GLYPH_PRO_BTN_RIGHT },
                { WPAD_PRO_BUTTON_ZR,    "WPAD_PRO_BUTTON_ZR",    CAFE_GLYPH_PRO_BTN_ZR    },
                { WPAD_PRO_BUTTON_X,     "WPAD_PRO_BUTTON_X",     CAFE_GLYPH_PRO_BTN_X     },
                { WPAD_PRO_BUTTON_A,     "WPAD_PRO_BUTTON_A",     CAFE_GLYPH_PRO_BTN_A     },
                { WPAD_PRO_BUTTON_Y,     "WPAD_PRO_BUTTON_Y",     CAFE_GLYPH_PRO_BTN_Y     },
                { WPAD_PRO_BUTTON_B,     "WPAD_PRO_BUTTON_B",     CAFE_GLYPH_PRO_BTN_B     },
                { WPAD_PRO_BUTTON_ZL,    "WPAD_PRO_BUTTON_ZL",    CAFE_GLYPH_PRO_BTN_ZL    },
                { WPAD_PRO_BUTTON_R,     "WPAD_PRO_BUTTON_R",     CAFE_GLYPH_PRO_BTN_R     },
                { WPAD_PRO_BUTTON_PLUS,  "WPAD_PRO_BUTTON_PLUS",  CAFE_GLYPH_PRO_BTN_PLUS  },
                { WPAD_PRO_BUTTON_MINUS, "WPAD_PRO_BUTTON_MINUS", CAFE_GLYPH_PRO_BTN_MINUS },
                { WPAD_PRO_BUTTON_L,     "WPAD_PRO_BUTTON_L",     CAFE_GLYPH_PRO_BTN_L     },
            };

            constexpr unsigned num_buttons = std::size(button_list);

        } // namespace pro


        constexpr unsigned num_buttons = std::max({ nunchuk::num_buttons, classic::num_buttons, pro::num_buttons });

    } // namespace ext


    template<unsigned N>
    struct state_t {

        uint32_t turbo = 0;
        uint32_t fake_hold = 0;
        uint32_t suppressed = 0;
        array<OSTime, N> last_turbo_action{};
        aux_t aux;


        void
        reset()
            noexcept
        {
            turbo       = 0;
            fake_hold   = 0;
            suppressed  = 0;
            last_turbo_action.fill(0);
            aux.reset();
        }


        void
        update(uint32_t buttons)
            noexcept
        {
            aux.update(buttons);
        }


        template<typename BTN>
        void
        process_buttons(WPADChan channel,
                        BTN& buttons,
                        bool& toggling,
                        OSTime period,
                        const auto& button_list)
        {
            aux.update(buttons);

            for (auto [idx, info] : enumerate(button_list)) {
                auto [btn, btn_name, btn_glyph] = info;
                const auto not_btn = ~static_cast<BTN>(btn);

                // Button suppression logic.
                if (suppressed & btn) {
                    // if the button is not held, we stop suppressing it
                    if (!(aux.hold & btn))
                        suppressed &= not_btn;

                    buttons &= not_btn;
                    continue;
                }

                // Turbo toggling logic.
                if (toggling && (aux.trigger & btn)) {

                    toggling = false;
                    turbo ^= btn;

                    const char* on_off = turbo & btn ? "turbo" : "normal";

                    notify::info("wiimote %d button %s is %s",
                                 int(channel),
                                 btn_glyph,
                                 on_off);

                    // Hide this button event from the game.
                    buttons &= not_btn;

                    // This button will be suppressed until a release event happens.
                    suppressed |= btn;

                    last_turbo_action[idx] = 0;

                    continue;

                }

                // Early out: nothing else to process if button is not held.
                if (!(aux.hold & btn)) {
                    fake_hold &= not_btn;
                    continue;
                }

                OSTime now = OSGetSystemTime();

                if (aux.trigger & btn) {
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
                            buttons |= btn;
                        } else {
                            // simulate a release event
                            buttons &= not_btn;
                        }
                    }
                } // if turbo action

            } // for each button

        }


    };




    struct wpad_state {

        state_t<core::num_buttons> core;
        state_t<ext::num_buttons>  ext;
        bool toggling = false;
        WPADExtensionType ext_type = WPAD_EXT_CORE;


        void
        reset()
            noexcept
        {
            core.reset();
            ext.reset();
            toggling = false;
            ext_type = WPAD_EXT_CORE;
        }


        bool
        flip_toggling()
            noexcept
        {
            return toggling = !toggling;
        }


        void
        process_wpad_read(WPADChan channel,
                          WPADStatus* status,
                          OSTime period)
        {
            if (ext_type != status->extensionType) {
                ext.reset();
                ext_type = static_cast<WPADExtensionType>(status->extensionType);
            }

            switch (ext_type) {
                case WPAD_EXT_CORE:
                case WPAD_EXT_MPLUS:
                    core.process_buttons(channel,
                                         status->buttons,
                                         toggling,
                                         period,
                                         core::button_list);
                    break;

                case WPAD_EXT_NUNCHUK:
                case WPAD_EXT_MPLUS_NUNCHUK:
                    core.process_buttons(channel,
                                         status->buttons,
                                         toggling,
                                         period,
                                         core::button_list);
                    ext.process_buttons(channel,
                                        status->buttons,
                                        toggling,
                                        period,
                                        ext::nunchuk::button_list);
                    break;


                case WPAD_EXT_CLASSIC:
                case WPAD_EXT_MPLUS_CLASSIC:
                    core.process_buttons(channel,
                                         status->buttons,
                                         toggling,
                                         period,
                                         core::button_list);
                    ext.process_buttons(channel,
                                        reinterpret_cast<WPADStatusClassic*>(status)->buttons,
                                        toggling,
                                        period,
                                        ext::classic::button_list);
                    break;

                case WPAD_EXT_PRO_CONTROLLER:
                    ext.process_buttons(channel,
                                        reinterpret_cast<WPADStatusPro*>(status)->buttons,
                                        toggling,
                                        period,
                                        ext::pro::button_list);
                    break;

                default:
                    ;

            } // switch (ext_type)

        }

    };


    array<wpad_state, max_wpads> states;


    // Reset all variables.
    void
    reset()
    {
        logger::printf("Resetting turbo state for wpads\n");
        for (auto& st : states)
            st.reset();
    }


    void
    on_toggle(WPADChan channel)
    {
        if (states[channel].flip_toggling())
            notify::info("Toggling turbo on vpad %d...", int(channel));
        else
            notify::info("Canceled turbo toggle on vpad %d.", int(channel));
    }


    DECL_FUNCTION(void,
                  WPADRead,
                  WPADChan channel,
                  WPADStatus* status)
    {
        real_WPADRead(channel, status);
        if (!cfg::enabled.value)
            return;
        if (!status) [[unlikely]]
            return;
        if (channel < 0 || channel >= states.size()) [[unlikely]]
            return;

        OSTime period = OSMillisecondsToTicks(cfg::period.value.count());
        auto& state = states[channel];
        state.process_wpad_read(channel, status, period);
    }

    WUPS_MUST_REPLACE(WPADRead, WUPS_LOADER_LIBRARY_PADSCORE, WPADRead);

} // namespace wpad
