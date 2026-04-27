/*
 * Copyright (c) 2026, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once
#include "HashTable.h"

template<typename K, typename V>
class HashMap {
public:
private:
    struct Entry {
        K key;
        V value;

        Hash hash() const { return HashTraits<K>::hash(key); }
        bool equals(Entry const& other) const { return HashTraits<K>::equals(key, other.key); }
    };

    HashTable<Entry> m_table;
};
