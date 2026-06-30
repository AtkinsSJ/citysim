/*
 * Copyright (c) 2026, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <Util/Basic.h>
#include <flecs.h>

struct mod_budget {
    explicit mod_budget(flecs::world&);
};

class Budget {
public:
    explicit Budget(s32 funds);

    bool can_afford(s32 cost) const;
    void spend(s32 cost);

    s32 funds() const { return m_funds; }
    s32 monthly_expenditure() const { return m_monthly_expenditure; }

    void show_cost_tooltip(s32 cost) const;

private:
    s32 m_funds { 0 };
    s32 m_monthly_expenditure { 0 };
};
