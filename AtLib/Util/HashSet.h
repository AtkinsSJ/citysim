/*
 * Copyright (c) 2015-2026, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <Util/Assert.h>
#include <Util/Hash.h>
#include <Util/RawAllocator.h>

template<Hashable T>
class HashSet {
public:
    explicit HashSet(size_t initial_capacity, float max_load_factor = 0.75f)
        : m_max_load_factor(max_load_factor)
    {
        ASSERT(max_load_factor < 1.0f);

        if (initial_capacity > 0) {
            expand(ceil_s32(initial_capacity / max_load_factor));
        }
    }

    HashSet()
        : HashSet(0)
    {
    }

    HashSet(HashSet const& other)
        : HashSet(other.capacity(), other.m_max_load_factor)
    {
        put_all(other);
    }

    HashSet& operator=(HashSet const& other)
    {
        if (&other == this)
            return *this;

        clear();
        m_max_load_factor = other.m_max_load_factor;
        put_all(other);
        return *this;
    }

    HashSet(HashSet&& other)
        : m_entries(move(other.m_entries))
        , m_count(other.m_count)
        , m_max_load_factor(other.m_max_load_factor)
    {
        other.m_entries = {};
        other.m_count = 0;
    }

    HashSet& operator=(HashSet&& other)
    {
        m_count = other.m_count;
        m_max_load_factor = other.m_max_load_factor;
        m_entries = move(other.m_entries);

        other.m_count = 0;
        other.m_entries = {};

        return *this;
    }

    ~HashSet()
    {
        clear();

        if (!m_entries.is_empty())
            RawAllocator::the().deallocate(m_entries);
    }

    size_t count() const { return m_count; }
    size_t capacity() const { return m_entries.size(); }
    bool is_empty() const { return m_count == 0; }

    bool contains(T const& value) const
    {
        if (m_count == 0)
            return false;

        return find_entry(value).is_occupied;
    }

    template<typename Equals>
    bool contains(Hash hash, Equals const& equals) const
    {
        if (m_count == 0)
            return false;

        return find_entry(hash, equals).is_occupied;
    }

    Optional<T&> find(T const& value)
    {
        if (m_count == 0)
            return {};

        if (auto& entry = find_entry(value); entry.is_occupied)
            return entry.value;

        return {};
    }

    Optional<T const&> find(T const& value) const
    {
        auto result = const_cast<HashSet*>(this)->find(value);
        if (result.has_value())
            return result.release_value();
        return {};
    }

    template<typename Equals>
    Optional<T&> find(Hash hash, Equals const& equals)
    {
        if (m_count == 0)
            return {};

        if (auto& entry = find_entry(hash, equals); entry.is_occupied)
            return entry.value;

        return {};
    }

    template<typename Equals>
    Optional<T const&> find(Hash hash, Equals const& equals) const
    {
        auto result = const_cast<HashSet*>(this)->find(hash, equals);
        if (result.has_value())
            return result.release_value();
        return {};
    }

    T& put(T const& value)
    {
        auto& entry = find_or_add_entry(value);

        if (!entry.is_occupied) {
            m_count++;
            entry.is_occupied = true;
            entry.is_tombstone = false;
        }

        new (&entry.value) T(move(value));

        return entry.value;
    }

    template<typename Equals, typename CreateValue>
    T& ensure(Hash hash, Equals const& equals, CreateValue const& create_value)
    {
        auto& entry = find_or_add_entry(hash, equals);
        if (!entry.is_occupied) {
            m_count++;
            entry.is_occupied = true;
            entry.is_tombstone = false;
            entry.value = create_value();
        }
        return entry.value;
    }

    void put_all(HashSet const& source)
    {
        for (auto value : source)
            put(value);
    }

    void remove(T const& value)
    {
        if (m_count == 0)
            return;

        if (auto& entry = find_entry(value); entry.is_occupied) {
            entry.replace_with_tombstone();
            m_count--;
        }
    }

    template<typename Equals>
    void remove(Hash hash, Equals const& equals)
    {
        if (m_count == 0)
            return;

        if (auto& entry = find_entry(hash, equals); entry.is_occupied) {
            entry.replace_with_tombstone();
            m_count--;
        }
    }

    void clear()
    {
        for (auto& entry : m_entries)
            entry.clear();
        m_count = 0;
    }

    template<typename SetT, typename ValueT>
    class Iterator {
    public:
        Iterator(Badge<HashSet>, SetT& table, size_t index)
            : m_table(table)
            , m_index(index)
        {
            while (m_index < m_table.capacity() && !m_table.m_entries[m_index].is_occupied)
                ++m_index;
        }

        ValueT& operator*() { return m_table.m_entries[m_index].value; }
        ValueT* operator->() { return &m_table.m_entries[m_index].value; }

        Iterator& operator++()
        {
            ++m_index;
            while (m_index < m_table.capacity() && !m_table.m_entries[m_index].is_occupied)
                ++m_index;
            return *this;
        }

        Iterator operator++(int)
        {
            auto result = *this;
            ++(*this);
            return result;
        }

        bool operator==(Iterator const& other)
        {
            return &m_table == &other.m_table
                && m_index == other.m_index;
        }

        bool operator!=(Iterator const& other)
        {
            return !(*this == other);
        }

    private:
        SetT& m_table;
        size_t m_index;
    };
    using MutableIterator = Iterator<HashSet<T>, T>;
    using ConstIterator = Iterator<HashSet<T> const, T const>;
    friend MutableIterator;
    friend ConstIterator;

    MutableIterator begin()
    {
        return MutableIterator({}, *this, 0);
    }

    MutableIterator end()
    {
        return MutableIterator({}, *this, m_entries.size());
    }

    ConstIterator begin() const
    {
        return ConstIterator({}, *this, 0);
    }

    ConstIterator end() const
    {
        return ConstIterator({}, *this, m_entries.size());
    }

private:
    struct Entry {
        // FIXME: Optional<T> instead of is_occupied?
        // Or... a Variant of T, Empty, Tombstone?
        T value;
        bool is_occupied { false };
        bool is_tombstone { false };

        void replace_with_tombstone()
        {
            clear();
            is_tombstone = true;
        }

        void clear()
        {
            is_occupied = false;
            is_tombstone = false;
            if constexpr (requires { value.~T(); }) {
                value.~T();
            } else {
                value = {};
            }
        }
    };

    void expand(size_t new_capacity)
    {
        ASSERT(new_capacity > 0);
        ASSERT(new_capacity > capacity());

        size_t old_count = m_count;
        auto old_entries = m_entries;

        m_entries = RawAllocator::the().allocate_multiple<Entry>(new_capacity);
        m_count = 0;

        if (old_count != 0) {
            // Migrate old entries over
            for (auto& old_entry : old_entries) {
                if (old_entry.is_occupied)
                    put(move(old_entry.value));
            }

            RawAllocator::the().deallocate(old_entries);
        }

        ASSERT(old_count == m_count);
    }

    template<typename Equals>
    Entry& find_entry(Hash hash, Equals const& equals)
    {
        u32 index = hash % capacity();
        Entry* result = nullptr;

        // "Linear probing" - on collision, just keep going until you find an empty slot
        size_t items_checked = 0;
        while (true) {
            auto& entry = m_entries[index];

            if (entry.is_tombstone) {
                // Store the first tombstone we find, in case we fail to find the "real" option
                if (result == nullptr)
                    result = &entry;
            } else if (!entry.is_occupied || (hash == HashTraits<T>::hash(entry.value) && equals(entry.value))) {
                // If the entry is unoccupied, we'd rather re-use the tombstone we found above
                if (entry.is_occupied || result == nullptr) {
                    result = &entry;
                }
                break;
            }

            index = (index + 1) % capacity();

            // Prevent the edge case infinite loop if all unoccupied spaces are gravestones
            items_checked++;
            if (items_checked >= capacity())
                break;
        }

        ASSERT(result != nullptr);
        return *result;
    }

    template<typename Equals>
    Entry const& find_entry(Hash hash, Equals const& equals) const
    {
        return const_cast<HashSet*>(this)->find_entry(hash, equals);
    }

    Entry& find_entry(T const& key)
    {
        return find_entry(HashTraits<T>::hash(key), [&key](T const& value) {
            return HashTraits<T>::equals(key, value);
        });
    }

    Entry const& find_entry(T const& key) const
    {
        return const_cast<HashSet*>(this)->find_entry(key);
    }

    template<typename Equals>
    Entry& find_or_add_entry(Hash hash, Equals const& equals)
    {
        auto expand_and_find_new_entry = [&] -> Entry& {
            auto new_capacity = max(8, ceil_s32((m_count + 1) / m_max_load_factor), capacity() * 2);
            expand(new_capacity);

            // We now have to search again, because the result we got before is now invalid
            return find_entry(hash, equals);
        };

        if (capacity() == 0) {
            // We're at 0 capacity, so expand
            return expand_and_find_new_entry();
        }

        auto& result = find_entry(hash, equals);
        if (!result.is_occupied) {
            // Expand if needed!
            if (m_count + 1 > (capacity() * m_max_load_factor))
                return expand_and_find_new_entry();
        }
        return result;
    }

    Entry& find_or_add_entry(T const& key)
    {
        return find_or_add_entry(HashTraits<T>::hash(key), [&key](T const& value) {
            return HashTraits<T>::equals(key, value);
        });
    }

    Span<Entry> m_entries {};
    size_t m_count { 0 };

    float m_max_load_factor { 0 };
};
