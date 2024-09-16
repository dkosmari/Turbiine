/*
 * Turbiine - Turn any controller into a turbo controller.
 *
 * Copyright (C) 2024  Daniel K. O.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef CFG_HPP
#define CFG_HPP

#include <array>

#include <wupsxx/button_combo.hpp>


namespace cfg {

    inline constexpr unsigned max_toggle_combos = 3;

    extern bool enabled;
    extern std::array<wups::utils::button_combo,
                      max_toggle_combos> toggle_combo;

    void init();

} // namespace cfg

#endif
