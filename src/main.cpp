/*
 * Turbiine - Turn any controller into a turbo controller.
 *
 * Copyright (C) 2025  Daniel K. O.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <cstdint>
#include <stdexcept>

#include <wups.h>

#include <wupsxx/logger.hpp>
#include <wupsxx/notify.hpp>
#include <wupsxx/shortcut.hpp>

#include "cfg.hpp"
#include "core.hpp"

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif


namespace logger   = wups::logger;
namespace notify   = wups::notify;
namespace shortcut = wups::shortcut;


using std::int32_t;
using std::uint32_t;

WUPS_PLUGIN_NAME(PACKAGE_NAME);
WUPS_PLUGIN_DESCRIPTION("Button goes BRRRRRRT!");
WUPS_PLUGIN_VERSION(PACKAGE_VERSION);
WUPS_PLUGIN_AUTHOR("Daniel K. O.");
WUPS_PLUGIN_LICENSE("GPLv3");


WUPS_USE_WUT_DEVOPTAB();
WUPS_USE_STORAGE(PACKAGE_TARNAME);


INITIALIZE_PLUGIN()
{
    logger::set_prefix(PACKAGE_NAME);
    logger::guard guard;

    try {
        notify::initialize(PACKAGE_NAME);
        notify::info::set_text_color(0xff, 0xff, 0xff);
        notify::info::set_bg_color(0x10, 0x10, 0x40);

        shortcut::initialize(PACKAGE_NAME);

        cfg::initialize();
    }
    catch (std::exception& e) {
        logger::printf("Error initializing: %s\n", e.what());
    }
}


DEINITIALIZE_PLUGIN()
{
    logger::guard guard;
    try {
        cfg::finalize();
    }
    catch (std::exception& e) {
        logger::printf("Error finalizing: %s\n", e.what());
    }

    shortcut::finalize();
    notify::finalize();
}


ON_APPLICATION_START()
{
    logger::initialize();
}


ON_APPLICATION_ENDS()
{
    core::reset();
    logger::finalize();
}
