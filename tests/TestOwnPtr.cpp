/*
 * Copyright (c) 2025, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include "Harness/Harness.h"
#include <Util/OwnPtr.h>

void test_main()
{
    int times_constructed = 0;
    int times_destructed = 0;

    struct ConstructionCounter {
        explicit ConstructionCounter(int& times_constructed, int& times_destructed)
            : m_times_constructed(times_constructed)
            , m_times_destructed(times_destructed)
        {
            ++m_times_constructed;
        }

        ~ConstructionCounter()
        {
            ++m_times_destructed;
        }

        int& m_times_constructed;
        int& m_times_destructed;
    };

    auto reset = [&] {
        times_constructed = 0;
        times_destructed = 0;
    };

    // Basic OwnPtr
    {
        OwnPtr<ConstructionCounter> value;
        EXPECT(!value);
        value = adopt_own_if_nonnull(new ConstructionCounter(times_constructed, times_destructed));
        EXPECT(value);
        EXPECT(times_constructed == 1);
        EXPECT(times_destructed == 0);

        value = nullptr;
        EXPECT(times_constructed == 1);
        EXPECT(times_destructed == 1);
    }
    EXPECT(times_constructed == 1);
    EXPECT(times_destructed == 1);
    reset();

    // OwnPtr moving
    {
        OwnPtr<ConstructionCounter> first = adopt_own_if_nonnull(new ConstructionCounter(times_constructed, times_destructed));
        EXPECT(times_constructed == 1);
        EXPECT(times_destructed == 0);

        OwnPtr second = move(first);
        EXPECT(!first);
        EXPECT(second);
        EXPECT(times_constructed == 1);
        EXPECT(times_destructed == 0);
    }
    EXPECT(times_constructed == 1);
    EXPECT(times_destructed == 1);
    reset();

    // Basic NonnullOwnPtr
    {
        NonnullOwnPtr<ConstructionCounter> first = adopt_own(*new ConstructionCounter(times_constructed, times_destructed));
        EXPECT(times_constructed == 1);
        EXPECT(times_destructed == 0);

        NonnullOwnPtr second = move(first);
        EXPECT(times_constructed == 1);
        EXPECT(times_destructed == 0);
    }
    EXPECT(times_constructed == 1);
    EXPECT(times_destructed == 1);
    reset();

    // NonnullOwnPtr from OwnPtr
    {
        OwnPtr<ConstructionCounter> own_ptr = adopt_own_if_nonnull(new ConstructionCounter(times_constructed, times_destructed));
        EXPECT(times_constructed == 1);
        EXPECT(times_destructed == 0);

        auto nonnull_own_ptr = own_ptr.release_nonnull();
        EXPECT(!own_ptr);
        EXPECT(times_constructed == 1);
        EXPECT(times_destructed == 0);
    }
    EXPECT(times_constructed == 1);
    EXPECT(times_destructed == 1);
    reset();

    // OwnPtr from NonnullOwnPtr (construction)
    {
        NonnullOwnPtr<ConstructionCounter> nonnull_own_ptr = adopt_own(*new ConstructionCounter(times_constructed, times_destructed));
        EXPECT(times_constructed == 1);
        EXPECT(times_destructed == 0);

        OwnPtr own_ptr = move(nonnull_own_ptr);
        EXPECT(own_ptr);
        EXPECT(times_constructed == 1);
        EXPECT(times_destructed == 0);
    }
    EXPECT(times_constructed == 1);
    EXPECT(times_destructed == 1);
    reset();

    // OwnPtr from NonnullOwnPtr (assignment)
    {
        NonnullOwnPtr<ConstructionCounter> nonnull_own_ptr = adopt_own(*new ConstructionCounter(times_constructed, times_destructed));
        EXPECT(times_constructed == 1);
        EXPECT(times_destructed == 0);

        OwnPtr<ConstructionCounter> own_ptr = adopt_own_if_nonnull(new ConstructionCounter(times_constructed, times_destructed));
        EXPECT(own_ptr);
        EXPECT(times_constructed == 2);
        EXPECT(times_destructed == 0);

        own_ptr = move(nonnull_own_ptr);
        EXPECT(own_ptr);
        EXPECT(times_constructed == 2);
        EXPECT(times_destructed == 1);
    }
    EXPECT(times_constructed == 2);
    EXPECT(times_destructed == 2);
    reset();
}
