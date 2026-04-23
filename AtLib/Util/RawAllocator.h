/*
 * Copyright (c) 2026, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <Util/Allocator.h>

class RawAllocator final : public Allocator {
public:
    virtual ~RawAllocator() override = default;

    static RawAllocator& the();

private:
    Span<u8> allocate_internal(size_t size) override;
    void deallocate_internal(Span<u8>) override;
};
