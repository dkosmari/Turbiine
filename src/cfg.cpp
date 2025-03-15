/*
 * Turbiine - Turn any controller into a turbo controller.
 *
 * Copyright (C) 2025  Daniel K. O.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <vector>

#include <wupsxx/bool_item.hpp>
#include <wupsxx/button_combo_item.hpp>
#include <wupsxx/duration_items.hpp>
#include <wupsxx/category.hpp>
#include <wupsxx/init.hpp>
#include <wupsxx/logger.hpp>
#include <wupsxx/storage.hpp>

#include "cfg.hpp"

#include "core.hpp"
#include "reset_turbo_item.hpp"

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif


using std::array;
using std::chrono::milliseconds;
using wups::button_combo::combo;
using wups::option;

namespace logger = wups::logger;
using namespace std::literals;


namespace cfg {


    WUPSXX_OPTION("Enabled",
                  bool, enabled, false);

    WUPSXX_OPTION("Period",
                  milliseconds, period, 16ms, 1ms, 1000ms);

    WUPSXX_OPTION("Toggle turbo 1",
                  combo, toggle1, combo::from_vpad(VPAD_BUTTON_TV | VPAD_BUTTON_ZL));

    WUPSXX_OPTION("Toggle turbo 2",
                  combo, toggle2, combo::from_wpad_core(WPAD_BUTTON_MINUS |
                                                        WPAD_BUTTON_PLUS |
                                                        WPAD_BUTTON_B));

    WUPSXX_OPTION("Toggle turbo 3",
                  combo, toggle3, combo::from_wpad_classic({},
                                                           WPAD_CLASSIC_BUTTON_DOWN |
                                                           WPAD_CLASSIC_BUTTON_MINUS |
                                                           WPAD_CLASSIC_BUTTON_ZL));


    const std::vector<wups::option_base*> all_options{
        &enabled,
        &period,
        &toggle1,
        &toggle2,
        &toggle3,
    };


    wups::button_combo::handle toggle1_handle;
    wups::button_combo::handle toggle2_handle;
    wups::button_combo::handle toggle3_handle;


    void
    load()
        noexcept
    {
        for (auto& opt : all_options)
            try {
                opt->load();
            }
            catch (std::exception& e) {
                logger::printf("Error loading config item '%s': %s\n",
                               opt->key.data(),
                               e.what());
            }
    }


    void
    save()
        noexcept
    {
        try {
            for (const auto& opt : all_options)
                opt->store();
            wups::save();
        }
        catch (std::exception& e) {
            logger::printf("Error saving config: %s\n", e.what());
        }
    }


    void
    menu_open(wups::category& root)
    {
        using wups::make_item;

        // keep logger enabled until menu is closed
        logger::initialize(PACKAGE_NAME);

        root.add(make_item(enabled, "yes", "no"));
        root.add(make_item(period));
        root.add(make_item(toggle1, toggle1_handle));
        root.add(make_item(toggle2, toggle2_handle));
        root.add(make_item(toggle3, toggle3_handle));
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
    initialize()
    {
        using wups::button_combo::create;

        wups::init(PACKAGE_NAME, menu_open, menu_close);
        cfg::load();

        try {
            auto [handle, conflict] = create(PACKAGE_NAME " toggle 1",
                                             toggle1.value,
                                             core::on_toggle);
            toggle1_handle = handle;
        }
        catch (std::exception& e) {
            logger::printf("Error creating combo for toggle 1: %s\n", e.what());
        }
        try {
            auto [handle, conflict] = create(PACKAGE_NAME " toggle 2",
                                             toggle2.value,
                                             core::on_toggle);
            toggle2_handle = handle;
        }
        catch (std::exception& e) {
            logger::printf("Error creating combo for toggle 2: %s\n", e.what());
        }
        try {
            auto [handle, conflict] = create(PACKAGE_NAME " toggle 3",
                                             toggle3.value,
                                             core::on_toggle);
            toggle3_handle = handle;
        }
        catch (std::exception& e) {
            logger::printf("Error creating combo for toggle 3: %s\n", e.what());
        }
    }


    void
    finalize()
    {
        using wups::button_combo::destroy;
        destroy(toggle1_handle);
        destroy(toggle2_handle);
        destroy(toggle3_handle);
    }

} // namespace cfg
