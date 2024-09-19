/*
 * Turbiine - Turn any controller into a turbo controller.
 *
 * Copyright (C) 2024  Daniel K. O.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef RESET_TURBO_ITEM_HPP
#define RESET_TURBO_ITEM_HPP

#include <memory>

#include <wupsxx/button_item.hpp>


struct reset_turbo_item : wups::config::button_item {


    reset_turbo_item();


    static
    std::unique_ptr<reset_turbo_item>
    create();


    virtual
    void
    on_started() override;

};



#endif
