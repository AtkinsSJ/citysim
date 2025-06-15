/*
 * Copyright (c) 2015-2025, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

template<typename T>
struct Maybe {
    bool isValid;
    T value;

    T orDefault(T defaultValue)
    {
        return isValid ? value : defaultValue;
    }
};

template<typename T>
Maybe<T> makeSuccess(T value)
{
    Maybe<T> result;
    result.isValid = true;
    result.value = value;

    return result;
}

template<typename T>
Maybe<T> makeFailure()
{
    Maybe<T> result = {};
    result.isValid = false;

    return result;
}

template<typename T>
bool allAreValid(Maybe<T> input)
{
    return input.isValid;
}

template<typename T, typename... TS>
bool allAreValid(Maybe<T> first, Maybe<TS>... rest)
{
    return first.isValid && allAreValid(rest...);
}
