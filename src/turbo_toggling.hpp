/*
 * Turbiine - Turn any controller into a turbo controller.
 *
 * Copyright (C) 2026  Daniel K. O.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef TURBO_TOGGLING_HPP
#define TURBO_TOGGLING_HPP

/*
 * NOTE: Toggling a turbo state requires two transitions:
 *
 *   - user activates the shortcut to toggle turbo;
 *
 *   - toggling state goes to `waiting' until all buttons are released;
 *
 *   - once all buttons are released, the state goes to `reading';
 *
 *   - the first triggered button is the one that will have its turbo state changed;
 *
 *   - toggling state goes back to `normal'.
 */
enum class turbo_toggling {
    normal,
    waiting, // waiting until all buttons are released
    reading, // waiting for a button to be triggered
};

#endif
