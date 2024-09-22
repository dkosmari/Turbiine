/*
 * Turbiine - Turn any controller into a turbo controller.
 *
 * Copyright (C) 2024  Daniel K. O.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <wupsxx/bool_item.hpp>
#include <wupsxx/button_combo_item.hpp>
#include <wupsxx/int_item.hpp>
#include <wupsxx/category.hpp>
#include <wupsxx/init.hpp>
#include <wupsxx/logger.hpp>
#include <wupsxx/storage.hpp>

#include "cfg.hpp"

#include "reset_turbo_item.hpp"

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif


using std::array;

using namespace wups::config;
using namespace wups::utils;

namespace logger = wups::logger;


WUPS_USE_WUT_DEVOPTAB();
WUPS_USE_STORAGE(PACKAGE_TARNAME);


namespace cfg {

    namespace defaults {

        const bool enabled = false;

        const int period = 1;

        const array<button_combo, max_toggle_combos> toggle_combo = {
            vpad::button_set{VPAD_BUTTON_TV,
                             VPAD_BUTTON_ZL},
            wpad::button_set{wpad::core::button_set{WPAD_BUTTON_MINUS,
                                                    WPAD_BUTTON_PLUS,
                                                    WPAD_BUTTON_B}},
            wpad::button_set{wpad::classic::button_set{WPAD_CLASSIC_BUTTON_DOWN,
                                                       WPAD_CLASSIC_BUTTON_MINUS,
                                                       WPAD_CLASSIC_BUTTON_ZL}},
            wpad::button_set{wpad::pro::button_set{WPAD_PRO_BUTTON_DOWN,
                                                   WPAD_PRO_BUTTON_MINUS,
                                                   WPAD_PRO_TRIGGER_ZL}}
        };

    } // namespace defaults


    bool enabled = defaults::enabled;

    int period = defaults::period;

    array<button_combo, max_toggle_combos> toggle_combo = defaults::toggle_combo;


    void
    load()
    {
        using wups::storage::load_or_init;

        load_or_init("enabled", enabled, defaults::enabled);

        load_or_init("period", period, defaults::period);

        for (unsigned i = 0; i < max_toggle_combos; ++i)
            load_or_init("toggle" + std::to_string(i + 1),
                         toggle_combo[i],
                         defaults::toggle_combo[i]);
    }


    void
    save()
    {
        using wups::storage::store;

        store("enabled", enabled);

        store("period", period);

        for (unsigned i = 0; i < max_toggle_combos; ++i)
            store("toggle" + std::to_string(i + 1),
                  toggle_combo[i]);

        wups::storage::save();
    }


    void
    menu_open(category& root)
    {
        logger::initialize(PACKAGE_NAME);

        root.add(bool_item::create("Enabled",
                                   enabled,
                                   defaults::enabled,
                                   "yes", "no"));

        root.add(int_item::create("Period",
                                  period,
                                  defaults::period,
                                  1, 100));

        for  (unsigned i = 0; i < max_toggle_combos; ++i)
            root.add(button_combo_item::create("Toggle turbo " + std::to_string(i + 1),
                                               toggle_combo[i],
                                               defaults::toggle_combo[i]));

        root.add(reset_turbo_item::create());
    }


    void
    menu_close()
    {
        try {
            save();
            logger::finalize();
        }
        catch (...) {
            logger::finalize();
            throw;
        }
    }


    void
    init()
    {
        wups::config::init(PACKAGE_NAME, menu_open, menu_close);

        cfg::load();
    }

} // namespace cfg
