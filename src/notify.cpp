/*
 * Turbiine - Turn any controller into a turbo controller.
 *
 * Copyright (C) 2024  Daniel K. O.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <cstdio>
#include <string>

#include <notifications/notifications.h>

#include <wupsxx/logger.hpp>

#include "notify.hpp"

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif


namespace notify {

    const std::string prefix = "[" PACKAGE_NAME "] ";

    const float duration = 3;

    const NMColor fg_color = {0xff, 0xff, 0xff, 0xff};
    const NMColor bg_color = {0x10, 0x10, 0x40, 0xff};


    void
    info(const char* fmt, ...)
        noexcept
    {
        std::va_list args;
        va_start(args, fmt);
        vinfo(fmt, args);
        va_end(args);
    }


    void
    vinfo(const char* fmt, std::va_list args)
        noexcept
    {
        try {
            std::va_list args_backup;
            va_copy(args_backup, args);
            int sz = std::vsnprintf(nullptr, 0, fmt, args_backup);
            va_end(args_backup);

            if (sz > 0) {
                std::string buf(prefix.size() + sz + 1, '\0');
                buf.replace(0, prefix.size(), prefix);
                std::vsnprintf(buf.data() + prefix.size(),
                               sz + 1,
                               fmt, args);
                NotificationModule_AddInfoNotificationEx(buf.c_str(),
                                                         duration,
                                                         fg_color,
                                                         bg_color,
                                                         nullptr,
                                                         nullptr,
                                                         true);
            }
        }
        catch (std::exception& e) {
            wups::logger::printf("Notification error: %s\n", e.what());
        }
        catch (...) {
            wups::logger::printf("Unknown notification error.\n");
        }
    }

} // namespace notify
