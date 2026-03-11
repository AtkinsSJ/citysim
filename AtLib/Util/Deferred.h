/*
 * Copyright (c) 2025, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

template<typename Callback>
class Deferred {
public:
    Deferred(Callback callback)
        : m_callback(move(callback))
    {
    }

    ~Deferred()
    {
        if (m_enabled)
            m_callback();
    }

    void disable() { m_enabled = false; }

private:
    Callback m_callback;
    bool m_enabled { true };
};
