/*
 * Turbiine - Turn any controller into a turbo controller.
 *
 * Copyright (C) 2025  Daniel K. O.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef CORE_HPP
#define CORE_HPP

#include <wupsxx/shortcut.hpp>


namespace core {

    void
    reset();


    void
    on_toggle(wups::shortcut::ctr_set src,
              wups::shortcut::handle handle);

} // namespace core

#endif
