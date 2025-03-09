/*
 * Turbiine - Turn any controller into a turbo controller.
 *
 * Copyright (C) 2025  Daniel K. O.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef CORE_HPP
#define CORE_HPP

#include <wupsxx/button_combo.hpp>


namespace core {

    void
    reset();


    void
    on_toggle(wups::button_combo::ctr_set controllers,
              wups::button_combo::handle handle);

} // namespace core

#endif
