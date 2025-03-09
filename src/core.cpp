/*
 * Turbiine - Turn any controller into a turbo controller.
 *
 * Copyright (C) 2025  Daniel K. O.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <wupsxx/logger.hpp>

#include "core.hpp"

#include "vpad.hpp"
#include "wpad.hpp"


namespace logger = wups::logger;


namespace core {

    void
    reset()
    {
        vpad::reset();
        wpad::reset();
    }


    void
    on_toggle(wups::button_combo::ctr_set controllers,
              wups::button_combo::handle)
    {
        switch (controllers) {
            case BUTTON_COMBO_MODULE_CONTROLLER_VPAD_0:
                vpad::on_toggle(VPAD_CHAN_0);
                break;
            case BUTTON_COMBO_MODULE_CONTROLLER_VPAD_1:
                vpad::on_toggle(VPAD_CHAN_1);
                break;
            case BUTTON_COMBO_MODULE_CONTROLLER_WPAD_0:
                wpad::on_toggle(WPAD_CHAN_0);
                break;
            case BUTTON_COMBO_MODULE_CONTROLLER_WPAD_1:
                wpad::on_toggle(WPAD_CHAN_1);
                break;
            case BUTTON_COMBO_MODULE_CONTROLLER_WPAD_2:
                wpad::on_toggle(WPAD_CHAN_2);
                break;
            case BUTTON_COMBO_MODULE_CONTROLLER_WPAD_3:
                wpad::on_toggle(WPAD_CHAN_3);
                break;
            case BUTTON_COMBO_MODULE_CONTROLLER_WPAD_4:
                wpad::on_toggle(WPAD_CHAN_4);
                break;
            case BUTTON_COMBO_MODULE_CONTROLLER_WPAD_5:
                wpad::on_toggle(WPAD_CHAN_5);
                break;
            case BUTTON_COMBO_MODULE_CONTROLLER_WPAD_6:
                wpad::on_toggle(WPAD_CHAN_6);
                break;
            default:
                logger::printf("Invalid controller to toggle: 0x%x\n", unsigned{controllers});
        }
    }


} // namespace core
