/*
 * Copyright (c) 2026, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <Util/HashSet.h>

template<Hashable K, typename V>
class HashMap {
public:
    using KeyTraits = HashTraits<K>;

    struct Entry {
        K key;
        V value;

        Hash hash() const { return KeyTraits::hash(key); }
        bool equals(Entry const& other) const { return KeyTraits::equals(key, other.key); }
    };

    explicit HashMap(size_t initial_capacity, float max_load_factor = 0.75f)
        : m_entries(initial_capacity, max_load_factor)
    {
    }

    HashMap()
        : HashMap(0)
    {
    }

    HashMap(HashMap const& other)
        : m_entries(other.m_entries)
    {
    }

    HashMap& operator=(HashMap const& other)
    {
        m_entries = other.m_entries;
        return *this;
    }

    HashMap(HashMap&& other)
        : m_entries(move(other.m_entries))
    {
        other.m_entries = {};
    }

    HashMap& operator=(HashMap&& other)
    {
        m_entries = move(other.m_entries);
        other.m_entries = {};

        return *this;
    }

    ~HashMap() = default;

    size_t count() const { return m_entries.count(); }
    size_t capacity() const { return m_entries.capacity(); }
    bool is_empty() const { return m_entries.is_empty(); }

    bool contains(K const& key) const
    {
        return m_entries.contains(KeyTraits::hash(key), [&key](Entry const& entry) {
            return KeyTraits::equals(key, entry.key);
        });
    }

    Optional<V&> get(K const& key)
    {
        return m_entries.find(KeyTraits::hash(key), [&key](Entry const& entry) {
                            return KeyTraits::equals(key, entry.key);
                        })
            .template map<V&>([](Entry& entry) -> V& { return entry.value; });
    }

    Optional<V const&> get(K const& key) const
    {
        return m_entries.find(KeyTraits::hash(key), [&key](Entry const& entry) {
                            return KeyTraits::equals(key, entry.key);
                        })
            .template map<V const&>([](Entry const& entry) -> V const& { return entry.value; });
    }

    V& set(K const& key, V const& value)
    {
        return m_entries.put({ key, value }).value;
    }

    void set_all(HashMap const& other)
    {
        for (auto& [key, value] : other)
            set(key, value);
    }

    template<typename CreateValue>
    V& ensure(K const& key, CreateValue const& create_value)
    {
        auto& entry = m_entries.ensure(
            KeyTraits::hash(key),
            [&key](Entry const& entry) { return KeyTraits::equals(key, entry.key); },
            [&] { return Entry { key, create_value() }; });
        return entry.value;
    }

    void remove(K const& key)
    {
        m_entries.remove(KeyTraits::hash(key), [&key](Entry const& entry) {
            return KeyTraits::equals(key, entry.key);
        });
    }

    void clear()
    {
        m_entries.clear();
    }

    HashSet<Entry>::MutableIterator begin()
    {
        return m_entries.begin();
    }

    HashSet<Entry>::MutableIterator end()
    {
        return m_entries.end();
    }

    HashSet<Entry>::ConstIterator begin() const
    {
        return m_entries.begin();
    }

    HashSet<Entry>::ConstIterator end() const
    {
        return m_entries.end();
    }

private:
    HashSet<Entry> m_entries;
};
