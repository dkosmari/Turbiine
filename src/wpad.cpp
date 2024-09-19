/*
 * Turbiine - Turn any controller into a turbo controller.
 *
 * Copyright (C) 2024  Daniel K. O.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <algorithm>
#include <array>
#include <bitset>
#include <concepts>
#include <cstdint>
#include <ranges>
#include <variant>

// #include <coreinit/thread.h> // DEBUG
#include <padscore/wpad.h>

#include <notifications/notifications.h>
#include <wups/function_patching.h>

#include <wupsxx/button_combo.hpp>
#include <wupsxx/logger.hpp>

#include "wpad.hpp"

#include "cfg.hpp"
#include <wupsxx/../../src/wpad_status.h>


using std::array;
using std::int32_t;
using std::uint32_t;
using std::uint8_t;
using std::views::enumerate;

using wups::utils::button_combo;

namespace logger = wups::logger;


namespace wpad {


    template<typename... Ts>
    struct overloaded : Ts... {
        using Ts::operator ()...;
    };


    template<typename T,
             typename... Ts>
    T&
    ensure(std::variant<Ts...>& v)
    {
        if (!holds_alternative<T>(v))
            v = T{};
        return get<T>(v);
    }


    constexpr unsigned max_wpads = 7;


    namespace core {

        constexpr array button_list = {
            WPAD_BUTTON_LEFT,
            WPAD_BUTTON_RIGHT,
            WPAD_BUTTON_DOWN,
            WPAD_BUTTON_UP,
            WPAD_BUTTON_PLUS,
            WPAD_BUTTON_2,
            WPAD_BUTTON_1,
            WPAD_BUTTON_B,
            WPAD_BUTTON_A,
            WPAD_BUTTON_MINUS,
        };

        constexpr unsigned max_buttons = button_list.size();

        using button_set = std::bitset<max_buttons>;

        struct pad_state_t {
            button_set turbo;
            button_set fake_hold;
            button_set suppress;
            array<uint8_t, max_buttons> age{};
        };

    } // namespace core


    namespace nunchuk {

        constexpr array button_list = {
            WPAD_NUNCHUK_BUTTON_Z,
            WPAD_NUNCHUK_BUTTON_C,
        };

        constexpr unsigned max_buttons = button_list.size();

        using button_set = std::bitset<max_buttons>;

        struct pad_state_t {
            button_set turbo;
            button_set fake_hold;
            button_set suppress;
            array<uint8_t, max_buttons> age{};
        };

    } // namespace nunchuk


    namespace classic {

        constexpr array button_list = {
            WPAD_CLASSIC_BUTTON_UP,
            WPAD_CLASSIC_BUTTON_LEFT,
            WPAD_CLASSIC_BUTTON_ZR,
            WPAD_CLASSIC_BUTTON_X,
            WPAD_CLASSIC_BUTTON_A,
            WPAD_CLASSIC_BUTTON_Y,
            WPAD_CLASSIC_BUTTON_B,
            WPAD_CLASSIC_BUTTON_ZL,
            WPAD_CLASSIC_BUTTON_R,
            WPAD_CLASSIC_BUTTON_PLUS,
            WPAD_CLASSIC_BUTTON_MINUS,
            WPAD_CLASSIC_BUTTON_L,
            WPAD_CLASSIC_BUTTON_DOWN,
            WPAD_CLASSIC_BUTTON_RIGHT,
        };

        constexpr unsigned max_buttons = button_list.size();

        using button_set = std::bitset<max_buttons>;

        struct pad_state_t {
            button_set turbo;
            button_set fake_hold;
            button_set suppress;
            array<uint8_t, max_buttons> age{};
        };

    } // namespace classic


    namespace pro {

        constexpr array button_list = {
            WPAD_PRO_BUTTON_UP,
            WPAD_PRO_BUTTON_LEFT,
            WPAD_PRO_TRIGGER_ZR,
            WPAD_PRO_BUTTON_X,
            WPAD_PRO_BUTTON_A,
            WPAD_PRO_BUTTON_Y,
            WPAD_PRO_BUTTON_B,
            WPAD_PRO_TRIGGER_ZL,
            WPAD_PRO_TRIGGER_R,
            WPAD_PRO_BUTTON_PLUS,
            WPAD_PRO_BUTTON_MINUS,
            WPAD_PRO_TRIGGER_L,
            WPAD_PRO_BUTTON_DOWN,
            WPAD_PRO_BUTTON_RIGHT,
        };

        constexpr unsigned max_buttons = button_list.size();

        using button_set = std::bitset<max_buttons>;

        struct pad_state_t {
            button_set turbo;
            button_set fake_hold;
            button_set suppress;
            array<uint8_t, max_buttons> age{};
        };

    } // namespace pro


    using ext_state_t = std::variant<std::monostate,
                                     nunchuk::pad_state_t,
                                     classic::pad_state_t,
                                     pro::pad_state_t>;


    struct pad_state_t {

        core::pad_state_t core;
        ext_state_t  ext;
        bool toggling = false;


        bool
        no_ext_turbo()
            const
        {
            return visit(overloaded{[](std::monostate) { return true; },
                                    [](const auto& st) { return st.turbo.none(); }},
                         ext);
        }


        void
        clear_and_suppress_buttons(WPADStatus* status)
        {
            switch (status->extensionType) {

            case WPAD_EXT_CORE:
            case WPAD_EXT_MPLUS:
                // Keep all held buttons suppressed
                for (auto [idx, btn] : enumerate(core::button_list))
                    if (status->buttons & btn)
                        core.suppress.set(idx);
                // Clear buttons.
                status->buttons = 0;
                // Get rid of any extension state.
                ext = std::monostate{};
                break;

            case WPAD_EXT_NUNCHUK:
            case WPAD_EXT_MPLUS_NUNCHUK:
                {
                    auto xstatus = reinterpret_cast<WPADNunchukStatus*>(status);
                    // Keep all held core buttons suppressed.
                    for (auto [idx, btn] : enumerate(core::button_list))
                        if (xstatus->core.buttons & btn)
                            core.suppress.set(idx);
                    // Keep all nunchuk buttons suppressed.
                    auto& xext = ensure<nunchuk::pad_state_t>(ext);
                    for (auto [idx, btn] : enumerate(nunchuk::button_list))
                        if (xstatus->core.buttons & btn)
                            xext.suppress.set(idx);
                    // Clear buttons: both core and nunchuk are stored here.
                    xstatus->core.buttons = 0;
                }
                break;

            case WPAD_EXT_CLASSIC:
            case WPAD_EXT_MPLUS_CLASSIC:
                {
                    auto xstatus = reinterpret_cast<WPADClassicStatus*>(status);
                    // Keep all held core buttons suppressed.
                    for (auto [idx, btn] : enumerate(core::button_list))
                        if (xstatus->core.buttons & btn)
                            core.suppress.set(idx);
                    // Keep all classic buttons suppressed.
                    auto& xext = ensure<classic::pad_state_t>(ext);
                    for (auto [idx, btn] : enumerate(classic::button_list))
                        if (xstatus->ext.buttons & btn)
                            xext.suppress.set(idx);
                    // Clear buttons.
                    xstatus->core.buttons = 0;
                    xstatus->ext.buttons = 0;
                }
                break;

            case WPAD_EXT_PRO_CONTROLLER:
                {
                    auto xstatus = reinterpret_cast<WPADProStatus*>(status);
                    // Keep all pro buttons suppressed.
                    auto& xext = ensure<pro::pad_state_t>(ext);
                    for (auto [idx, btn] : enumerate(pro::button_list))
                        if (xstatus->ext.buttons & btn)
                            xext.suppress.set(idx);
                    // Clear buttons.
                    xstatus->ext.buttons = 0;
                    // Note: we ignore core buttons, they're not supposed to be set.
                }
                break;

            } // switch
        }

    };


    array<pad_state_t, max_wpads> pads;


    // Reset all variables
    void
    reset()
    {
        pads.fill({});
    }


    wups::utils::wpad::core::button_set
    make_button_set(WPADButton b)
    {
        return {b};
    }


    wups::utils::wpad::ext_button_set
    make_button_set(WPADNunchukButton b)
    {
        return wups::utils::wpad::nunchuk::button_set{b};
    }


    wups::utils::wpad::ext_button_set
    make_button_set(WPADClassicButton b)
    {
        return wups::utils::wpad::classic::button_set{b};
    }


    wups::utils::wpad::ext_button_set
    make_button_set(WPADProButton b)
    {
        return wups::utils::wpad::pro::button_set{b};
    }


    template<typename Trb,
             typename Btn>
    void
    toggle_button(pad_state_t& pad,
                  Trb& turbo,
                  unsigned idx,
                  Btn btn,
                  WPADChan channel)
    {
        pad.toggling = false;

        turbo.flip(idx);

        wups::utils::wpad::button_set bs = make_button_set(btn);

        char buf[32];
        const char* on_off = turbo.test(idx) ? "turbo" : "normal";
        std::snprintf(buf, sizeof buf,
                      "%s = %s",
                      to_glyph(bs).c_str(),
                      on_off);
        NotificationModule_AddInfoNotificationEx(buf,
                                                 3,
                                                 {0xff, 0xff, 0xff, 0xff},
                                                 {0x20, 0x20, 0x40, 0xff},
                                                 nullptr,
                                                 nullptr,
                                                 true);
        logger::printf("wpad %u button %s = %s\n",
                       unsigned{channel},
                       to_string(bs).c_str(),
                       on_off);
    }


    void
    run_core_turbo_logic(pad_state_t& pad,
                         WPADStatus* status,
                         WPADChan channel)
    {
        // Early out: don't do anything if there's no turbo enabled, and not toggling turbo.
        if (!pad.toggling && pad.core.turbo.none())
            return;

        const auto& state = wups::utils::wpad::get_button_state(channel);

        for (auto [core_idx, core_btn] : enumerate(core::button_list)) {
            const auto not_core_btn = ~uint32_t{core_btn};

            // If this is a suppressed button, don't process it, keep it clear and
            // skip further turbo processing.
            if (pad.core.suppress.test(core_idx)) {
                // if the button is not held, or was released, we stop supressing it
                if (!(state.core.hold & core_btn) || (state.core.release & core_btn))
                    pad.core.suppress.reset(core_idx);

                status->buttons &= not_core_btn;
                continue;
            }

            if (pad.toggling && (state.core.trigger & core_btn)) {

                // We're in the toggling state, and a button was triggered.
                toggle_button(pad, pad.core.turbo, core_idx, core_btn, channel);

                // Hide this event from the game.
                status->buttons &= not_core_btn;

                pad.core.fake_hold.reset(core_idx);
                // This button will be suppressed until a release event happens.
                pad.core.suppress.set(core_idx);

                pad.core.age[core_idx] = 0;

            } else {

                // We're not in the toggling state, just check if it's a turbinated button
                // held down.

                bool turbinated = pad.core.turbo.test(core_idx);
                if (turbinated && (state.core.hold & core_btn)) {

                    if (++pad.core.age[core_idx] >= cfg::period) {

                        pad.core.fake_hold.flip(core_idx);
                        pad.core.age[core_idx] = 0;

                        if (pad.core.fake_hold.test(core_idx))
                            status->buttons |= core_btn; // simulate a press event
                        else
                            status->buttons &= not_core_btn; // simulate a release event

                    }

                } else // if no turbo action, just copy the real button state
                    pad.core.fake_hold.set(core_idx, status->buttons & core_btn);

            }
        }
    }


    void
    run_turbo_logic(pad_state_t& pad,
                    WPADNunchukStatus* status,
                    WPADChan channel)
    {
        auto& xpad = ensure<nunchuk::pad_state_t>(pad.ext);

        // Early out: don't do anything if there's no turbo enabled, and not toggling turbo.
        if (!pad.toggling && xpad.turbo.none())
            return;

        const auto& state = wups::utils::wpad::get_button_state(channel);
        if (!holds_alternative<wups::utils::wpad::nunchuk_button_state>(state.ext))
            [[unlikely]]
            return;
        const auto& xstate = get<wups::utils::wpad::nunchuk_button_state>(state.ext);

        for (auto [ext_idx, ext_btn] : enumerate(nunchuk::button_list)) {
            const auto not_ext_btn = ~uint32_t{ext_btn};

            // If this is a suppressed button, don't process it, keep it clear and
            // skip further turbo processing.
            if (xpad.suppress.test(ext_idx)) {
                // if the button is not held, or was released, we stop supressing it
                if (!(xstate.hold & ext_btn) || (xstate.release & ext_btn))
                    xpad.suppress.reset(ext_idx);

                status->core.buttons &= not_ext_btn;
                continue;
            }

            if (pad.toggling && (xstate.trigger & ext_btn)) {

                // We're in the toggling state, and a button was triggered.
                toggle_button(pad, xpad.turbo, ext_idx, ext_btn, channel);

                // Hide this event from the game.
                status->core.buttons &= not_ext_btn;

                xpad.fake_hold.reset(ext_idx);
                // This button will be suppressed until a release event happens.
                xpad.suppress.set(ext_idx);

                xpad.age[ext_idx] = 0;

            } else {

                // We're not in the toggling state, just check if it's a turbinated button
                // held down.

                bool turbinated = xpad.turbo.test(ext_idx);
                if (turbinated && (xstate.hold & ext_btn)) {

                    if (++xpad.age[ext_idx] >= cfg::period) {

                        xpad.fake_hold.flip(ext_idx);
                        xpad.age[ext_idx] = 0;

                        if (xpad.fake_hold.test(ext_idx))
                            status->core.buttons |= ext_btn; // simulate a press event
                        else
                            status->core.buttons &= not_ext_btn; // simulate a release event

                    }

                } else // if no turbo action, just copy the real button state
                    xpad.fake_hold.set(ext_idx, status->core.buttons & ext_btn);

            }

        }

    }


    void
    run_turbo_logic(pad_state_t& pad,
                    WPADClassicStatus* status,
                    WPADChan channel)
    {
        auto& xpad = ensure<classic::pad_state_t>(pad.ext);

        // Early out: don't do anything if there's no turbo enabled, and not toggling turbo.
        if (!pad.toggling && xpad.turbo.none())
            return;

        const auto& state = wups::utils::wpad::get_button_state(channel);
        if (!holds_alternative<wups::utils::wpad::classic_button_state>(state.ext))
            [[unlikely]]
            return;
        const auto& xstate = get<wups::utils::wpad::classic_button_state>(state.ext);

        for (auto [ext_idx, ext_btn] : enumerate(classic::button_list)) {
            const auto not_ext_btn = ~uint32_t{ext_btn};

            // If this is a suppressed button, don't process it, keep it clear and
            // skip further turbo processing.
            if (xpad.suppress.test(ext_idx)) {
                // if the button is not held, or was released, we stop supressing it
                if (!(xstate.hold & ext_btn) || (xstate.release & ext_btn))
                    xpad.suppress.reset(ext_idx);

                status->ext.buttons &= not_ext_btn;
                continue;
            }

            if (pad.toggling && (xstate.trigger & ext_btn)) {

                // We're in the toggling state, and a button was triggered.
                toggle_button(pad, xpad.turbo, ext_idx, ext_btn, channel);

                // Hide this event from the game.
                status->ext.buttons &= not_ext_btn;

                xpad.fake_hold.reset(ext_idx);
                // This button will be suppressed until a release event happens.
                xpad.suppress.set(ext_idx);

                xpad.age[ext_idx] = 0;

            } else {

                // We're not in the toggling state, just check if it's a turbinated button
                // held down.

                bool turbinated = xpad.turbo.test(ext_idx);
                if (turbinated && (xstate.hold & ext_btn)) {

                    if (++xpad.age[ext_idx] >= cfg::period) {

                        xpad.fake_hold.flip(ext_idx);
                        xpad.age[ext_idx] = 0;

                        if (xpad.fake_hold.test(ext_idx))
                            status->ext.buttons |= ext_btn; // simulate a press event
                        else
                            status->ext.buttons &= not_ext_btn; // simulate a release event

                    }

                } else // if no turbo action, just copy the real button state
                    xpad.fake_hold.set(ext_idx, status->ext.buttons & ext_btn);

            }

        }

    }


    void
    run_turbo_logic(pad_state_t& pad,
                    WPADProStatus* status,
                    WPADChan channel)
    {
        auto& xpad = ensure<pro::pad_state_t>(pad.ext);

        // Early out: don't do anything if there's no turbo enabled, and not toggling turbo.
        if (!pad.toggling && xpad.turbo.none())
            return;

        const auto& state = wups::utils::wpad::get_button_state(channel);
        if (!holds_alternative<wups::utils::wpad::pro_button_state>(state.ext))
            [[unlikely]]
            return;
        const auto& xstate = get<wups::utils::wpad::pro_button_state>(state.ext);

        for (auto [ext_idx, ext_btn] : enumerate(pro::button_list)) {
            const auto not_ext_btn = ~uint32_t{ext_btn};

            // If this is a suppressed button, don't process it, keep it clear and
            // skip further turbo processing.
            if (xpad.suppress.test(ext_idx)) {
                // if the button is not held, or was released, we stop supressing it
                if (!(xstate.hold & ext_btn) || (xstate.release & ext_btn))
                    xpad.suppress.reset(ext_idx);

                status->ext.buttons &= not_ext_btn;
                continue;
            }

            if (pad.toggling && (xstate.trigger & ext_btn)) {

                // We're in the toggling state, and a button was triggered.
                toggle_button(pad, xpad.turbo, ext_idx, ext_btn, channel);

                // Hide this event from the game.
                status->ext.buttons &= not_ext_btn;

                xpad.fake_hold.reset(ext_idx);
                // This button will be suppressed until a release event happens.
                xpad.suppress.set(ext_idx);

                xpad.age[ext_idx] = 0;

            } else {

                // We're not in the toggling state, just check if it's a turbinated button
                // held down.

                bool turbinated = xpad.turbo.test(ext_idx);
                if (turbinated && (xstate.hold & ext_btn)) {

                    if (++xpad.age[ext_idx] >= cfg::period) {

                        xpad.fake_hold.flip(ext_idx);
                        xpad.age[ext_idx] = 0;

                        if (xpad.fake_hold.test(ext_idx))
                            status->ext.buttons |= ext_btn; // simulate a press event
                        else
                            status->ext.buttons &= not_ext_btn; // simulate a release event

                    }

                } else // if no turbo action, just copy the real button state
                    xpad.fake_hold.set(ext_idx, status->ext.buttons & ext_btn);

            }

        } // for each possible button

    }


    void
    run_turbo_logic(pad_state_t& pad,
                    WPADStatus* status,
                    WPADChan channel)
    {
        switch (status->extensionType) {
        case WPAD_EXT_CORE:
        case WPAD_EXT_MPLUS:
            pad.ext = {};
            run_core_turbo_logic(pad, status, channel);
            break;
        case WPAD_EXT_NUNCHUK:
        case WPAD_EXT_MPLUS_NUNCHUK:
            run_core_turbo_logic(pad, status, channel);
            run_turbo_logic(pad,
                            reinterpret_cast<WPADNunchukStatus*>(status),
                            channel);
            break;
        case WPAD_EXT_CLASSIC:
        case WPAD_EXT_MPLUS_CLASSIC:
            run_core_turbo_logic(pad, status, channel);
            run_turbo_logic(pad,
                            reinterpret_cast<WPADClassicStatus*>(status),
                            channel);
            break;
        case WPAD_EXT_PRO_CONTROLLER:
            run_turbo_logic(pad,
                            reinterpret_cast<WPADProStatus*>(status),
                            channel);
            break;
        }
    }


    DECL_FUNCTION(void,
                  WPADRead,
                  WPADChan channel,
                  WPADStatus* status)
    {
        real_WPADRead(channel, status);
        if (!cfg::enabled)
            return;
        if (!status) [[unlikely]]
            return;
        if (channel < 0 || channel >= pads.size()) [[unlikely]]
            return;

        if (!wups::utils::wpad::update(channel, status))
            return;

        auto& pad = pads[channel];

        bool combo_activated = false;
        for (const auto& combo : cfg::toggle_combo)
            if (wups::utils::wpad::triggered(channel, combo))
                combo_activated = true;

        // Note: when a combo is activated, don't do any turbo processing.
        if (combo_activated) [[unlikely]] {
            // logger::printf("wpad combo activated on %u (thread = %p)\n",
            //                (unsigned)channel,
            //                OSGetCurrentThread());
            // Enter or leave toggling state.
            pad.toggling = !pad.toggling;

            // Discard buttons being held down, mark them as suppressed.
            pad.clear_and_suppress_buttons(status);
        }
        else [[likely]]
            run_turbo_logic(pad, status, channel);
    }


    WUPS_MUST_REPLACE(WPADRead, WUPS_LOADER_LIBRARY_PADSCORE, WPADRead);

} // namespace wpad
