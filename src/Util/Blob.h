/*
 * Copyright (c) 2015-2025, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <Util/Basic.h>

class Blob {
public:
    Blob(smm size, u8* data)
        : m_size(size)
        , m_data(data)
    {
    }

    Blob() = default;

    smm size() const { return m_size; }
    u8 const* data() const { return m_data; }
    u8* writable_data() { return m_data; }

private:
    smm m_size;
    u8* m_data;
};
