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

using Money = s32;

class Budget {
public:
    explicit Budget(Money funds);

    bool can_afford(Money cost) const;
    void spend(Money cost);

    Money funds() const { return m_funds; }
    Money monthly_expenditure() const { return m_monthly_expenditure; }

    void show_cost_tooltip(Money cost) const;

private:
    Money m_funds { 0 };
    Money m_monthly_expenditure { 0 };
};
