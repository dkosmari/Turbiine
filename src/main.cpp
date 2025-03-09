/*
 * Turbiine - Turn any controller into a turbo controller.
 *
 * Copyright (C) 2025  Daniel K. O.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <cstdint>
#include <stdexcept>

#include <notifications/notifications.h>
#include <buttoncombo/api.h>
#include <wups.h>

#include <wupsxx/logger.hpp>

#include "cfg.hpp"
#include "core.hpp"

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif


using std::int32_t;
using std::uint32_t;

namespace logger = wups::logger;


WUPS_PLUGIN_NAME(PACKAGE_NAME);
WUPS_PLUGIN_DESCRIPTION("Button goes BRRRRRRT!");
WUPS_PLUGIN_VERSION(PACKAGE_VERSION);
WUPS_PLUGIN_AUTHOR("Daniel K. O.");
WUPS_PLUGIN_LICENSE("GPLv3");


WUPS_USE_WUT_DEVOPTAB();
WUPS_USE_STORAGE(PACKAGE_TARNAME);


INITIALIZE_PLUGIN()
{
    logger::guard guard{PACKAGE_NAME};

    try {
        auto notify_status = NotificationModule_InitLibrary();
        if (notify_status)
            throw std::runtime_error{NotificationModule_GetStatusStr(notify_status)};

        auto combo_status = ButtonComboModule_InitLibrary();
        if (combo_status)
            throw std::runtime_error{ButtonComboModule_GetStatusStr(combo_status)};

        cfg::initialize();
    }
    catch (std::exception& e) {
        logger::printf("Error initializing: %s\n", e.what());
    }
}


DEINITIALIZE_PLUGIN()
{
    logger::guard guard{PACKAGE_NAME};
    try {
        cfg::finalize();
    }
    catch (std::exception& e) {
        logger::printf("Error finalizing: %s\n", e.what());
    }

    ButtonComboModule_DeInitLibrary();
    NotificationModule_DeInitLibrary();
}


ON_APPLICATION_START()
{
    logger::initialize(PACKAGE_NAME);
}


ON_APPLICATION_ENDS()
{
    core::reset();
    logger::finalize();
}
