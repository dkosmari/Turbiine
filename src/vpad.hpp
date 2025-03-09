/*
 * Turbiine - Turn any controller into a turbo controller.
 *
 * Copyright (C) 2025  Daniel K. O.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef VPAD_HPP
#define VPAD_HPP

#include <vpad/input.h>


namespace vpad {

    void reset();

    void on_toggle(VPADChan channel);

} // namespace vpad

#endif
