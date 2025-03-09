/*
 * Turbiine - Turn any controller into a turbo controller.
 *
 * Copyright (C) 2025  Daniel K. O.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef WPAD_HPP
#define WPAD_HPP

#include <padscore/wpad.h>

namespace wpad {

    void reset();

    void on_toggle(WPADChan channel);

} // namespace wpad

#endif
