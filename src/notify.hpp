/*
 * Turbiine - Turn any controller into a turbo controller.
 *
 * Copyright (C) 2024  Daniel K. O.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef NOTIFY_HPP
#define NOTIFY_HPP

#include <cstdarg>


namespace notify {

    __attribute__(( __format__ (__printf__, 1, 2)))
    void info(const char* fmt, ...) noexcept;

    void vinfo(const char* fmt, std::va_list args) noexcept;

} // namespace notify

#endif
