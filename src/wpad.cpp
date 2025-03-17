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
#include <wupsxx/notify.hpp>

#include "wpad.hpp"

#include "cfg.hpp"


using std::array;
using std::int32_t;
using std::uint32_t;
using std::uint8_t;
using std::views::enumerate;

namespace logger = wups::logger;
namespace notify = wups::notify;


namespace wpad {

    constexpr unsigned max_wpads = 7;


    // Simple class to track buttons triggered and released; also perform suppression logic.
    template<typename T>
    struct button_tracker {

        T hold     = 0;
        T trigger  = 0;
        T release  = 0;
        T suppress = 0;


        void
        reset()
            noexcept
        {
            hold     = 0;
            trigger  = 0;
            release  = 0;
            suppress = 0;
        }


        void
        update(T buttons)
            noexcept
        {
            T changed = hold ^ buttons;
            hold      = buttons;
            trigger   = changed &  buttons;
            release   = changed & ~buttons;
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


    template<typename T,
             unsigned N>
    struct state_t {

        T turbo      = 0;
        T fake_hold  = 0;
        array<OSTime, N> last_turbo_action{};
        button_tracker<T> tracker;


        void
        reset()
            noexcept
        {
            turbo       = 0;
            fake_hold   = 0;
            last_turbo_action.fill(0);
            tracker.reset();
        }


        void
        update(T buttons)
            noexcept
        {
            tracker.update(buttons);
        }


        template<typename BTN>
        void
        process_buttons(WPADChan channel,
                        BTN& buttons,
                        bool& toggling,
                        OSTime period,
                        const auto& button_list)
        {
            tracker.update(buttons);

            for (auto [idx, info] : enumerate(button_list)) {
                auto [btn, btn_name, btn_glyph] = info;
                const auto not_btn = ~static_cast<BTN>(btn);

                // Turbo toggling logic.
                if (toggling && (tracker.trigger & btn)) [[unlikely]]{

                    toggling = false;
                    turbo ^= btn;

                    const char* on_off = turbo & btn ? "turbo" : "normal";

                    notify::info::show("Wiimote %d button %s is %s",
                                       int(channel) + 1,
                                       btn_glyph,
                                       on_off);

                    // Hide this press from the game.
                    buttons &= not_btn;

                    // Block this button until it's released.
                    tracker.suppress |= btn;

                    last_turbo_action[idx] = 0;

                    continue;

                }

                OSTime now = OSGetSystemTime();

                if (tracker.trigger & btn) {
                    // Button was just pressed.
                    fake_hold |= btn;
                    last_turbo_action[idx] = now;
                    continue;
                }

                if (tracker.release & btn) {
                    // Button was just released.
                    fake_hold &= not_btn;
                    last_turbo_action[idx] = 0;
                    continue;
                }

                // If button is held and turbinated, do turbo action.
                if (buttons & btn && turbo & btn) {
                    OSTime age = now - last_turbo_action[idx];
                    if (age >= period) {
                        // time to generate turbo events
                        last_turbo_action[idx] = now;
                        fake_hold ^= btn;
                        if (fake_hold & btn) {
                            // simulate a press event
                            buttons |= btn;
                        } else {
                            // simulate a release event
                            buttons &= not_btn;
                        }
                    } else {
                        // in between turbo events, just copy fake_hold
                        buttons = (buttons & not_btn) | (fake_hold & btn);
                    }
                } // if turbo action

            } // for each button

        }

    }; // state_t<T, N>


    struct wpad_state {

        state_t<uint16_t, core::num_buttons> core;
        state_t<uint32_t, ext::num_buttons>  ext;
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
        logger::printf("Resetting turbo state for wiimotes\n");
        for (auto& st : states)
            st.reset();
    }


    void
    on_toggle(WPADChan channel)
    {
        if (states[channel].flip_toggling())
            notify::info::show("Toggling turbo on wiimote %d...", int(channel) + 1);
        else
            notify::info::show("Canceled turbo toggle on wiimote %d.", int(channel) + 1);
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
        if (status->error)
            return;

        OSTime period = OSMillisecondsToTicks(cfg::period.value.count());
        auto& state = states[channel];
        state.process_wpad_read(channel, status, period);
    }

    WUPS_MUST_REPLACE(WPADRead, WUPS_LOADER_LIBRARY_PADSCORE, WPADRead);

} // namespace wpad
