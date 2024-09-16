/*
 * Turbiine - Turn any controller into a turbo controller.
 *
 * Copyright (C) 2024  Daniel K. O.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <cstdint>
#include <stdexcept>

#include <notifications/notifications.h>
#include <wups.h>

#include <wupsxx/logger.hpp>

#include "cfg.hpp"
#include "vpad.hpp"
#include "wpad.hpp"

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


INITIALIZE_PLUGIN()
{
    logger::guard guard{PACKAGE_NAME};

    try {
        auto notify_status = NotificationModule_InitLibrary();
        if (notify_status != NOTIFICATION_MODULE_RESULT_SUCCESS)
            throw std::runtime_error{NotificationModule_GetStatusStr(notify_status)};
        cfg::init();
    }
    catch (std::exception& e) {
        logger::printf("ERROR during plugin init: %s\n", e.what());
    }
}


DEINITIALIZE_PLUGIN()
{
    NotificationModule_DeInitLibrary();
}


ON_APPLICATION_START()
{
    logger::initialize(PACKAGE_NAME);
}


ON_APPLICATION_ENDS()
{
    vpad::reset();
    wpad::reset();
    logger::finalize();
}
