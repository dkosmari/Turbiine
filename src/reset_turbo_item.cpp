/*
 * Turbiine - Turn any controller into a turbo controller.
 *
 * Copyright (C) 2025  Daniel K. O.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "reset_turbo_item.hpp"

#include "core.hpp"


reset_turbo_item::reset_turbo_item() :
    wups::button_item{"Disable all turbos..."}
{}


std::unique_ptr<reset_turbo_item>
reset_turbo_item::create()
{
    return std::make_unique<reset_turbo_item>();
}


void
reset_turbo_item::on_started()
{
    core::reset();

    current_state = state::stopped;
}
