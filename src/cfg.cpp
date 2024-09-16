/*
 * Turbiine - Turn any controller into a turbo controller.
 *
 * Copyright (C) 2024  Daniel K. O.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <wupsxx/bool_item.hpp>
#include <wupsxx/button_combo_item.hpp>
#include <wupsxx/category.hpp>
#include <wupsxx/init.hpp>
#include <wupsxx/logger.hpp>
#include <wupsxx/storage.hpp>

#include "cfg.hpp"

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

        const array<button_combo, max_toggle_combos> toggle_combo = {
            vpad::button_set{VPAD_BUTTON_TV, VPAD_BUTTON_ZL},
        };

    } // namespace defaults


    bool enabled = defaults::enabled;

    array<button_combo, max_toggle_combos> toggle_combo = defaults::toggle_combo;


    void
    load()
    {
        wups::storage::load_or_init("enabled", enabled, defaults::enabled);

        for (unsigned i = 0; i < max_toggle_combos; ++i)
            wups::storage::load_or_init("toggle" + std::to_string(i + 1),
                                        toggle_combo[i],
                                        defaults::toggle_combo[i]);
    }


    void
    save()
    {
        wups::storage::store("enabled", enabled);

        for (unsigned i = 0; i < max_toggle_combos; ++i)
            wups::storage::store("toggle" + std::to_string(i + 1),
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

        // TODO: add a button item to turn all turbo states off.

        for  (unsigned i = 0; i < max_toggle_combos; ++i)
            root.add(button_combo_item::create("Toggle " + std::to_string(i + 1),
                                               toggle_combo[i],
                                               defaults::toggle_combo[i]));

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