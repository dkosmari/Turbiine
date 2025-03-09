/*
 * Turbiine - Turn any controller into a turbo controller.
 *
 * Copyright (C) 2025  Daniel K. O.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef CFG_HPP
#define CFG_HPP

#include <array>
#include <chrono>

#include <wupsxx/option.hpp>


namespace cfg {

    extern wups::option<bool> enabled;
    extern wups::option<std::chrono::milliseconds> period;

    void initialize();
    void finalize();

} // namespace cfg

#endif
