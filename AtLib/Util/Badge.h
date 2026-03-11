/*
 * Copyright (c) 2025, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

template<typename T>
class Badge {
    friend T;
    constexpr Badge() = default;
};
