/*
 * Copyright (c) 2015-2026, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <Util/Assert.h>
#include <Util/Basic.h>
#include <Util/Forward.h>
#include <Util/Maths.h>
#include <Util/MemoryArena.h>
#include <Util/RawAllocator.h>
#include <Util/String.h>

template<typename T>
class HashTable {
public:
    explicit HashTable(size_t initial_capacity, float max_load_factor = 0.75f)
        : m_max_load_factor(max_load_factor)
    {
        ASSERT(max_load_factor < 1.0f);

        if (initial_capacity > 0) {
            expand(ceil_s32(initial_capacity / max_load_factor));
        }
    }

    HashTable()
        : HashTable(0)
    {
    }

    HashTable(HashTable const& other)
        : HashTable(other.capacity(), other.m_max_load_factor)
    {
        put_all(other);
    }

    HashTable& operator=(HashTable const& other)
    {
        if (&other == this)
            return *this;

        clear();
        m_max_load_factor = other.m_max_load_factor;
        put_all(other);
        return *this;
    }

    HashTable(HashTable&& other)
        : m_entries(move(other.m_entries))
        , m_count(other.m_count)
        , m_max_load_factor(other.m_max_load_factor)
    {
        other.m_entries = {};
        other.m_count = 0;
    }

    HashTable& operator=(HashTable&& other)
    {
        m_count = other.m_count;
        m_max_load_factor = other.m_max_load_factor;
        m_entries = move(other.m_entries);

        other.m_count = 0;
        other.m_entries = {};

        return *this;
    }

    ~HashTable()
    {
        clear();

        if (!m_entries.is_empty())
            RawAllocator::the().deallocate(m_entries);
    }

    size_t count() const { return m_count; }
    size_t capacity() const { return m_entries.size(); }

    bool contains(T const& value) const
    {
        if (m_count == 0)
            return false;

        return find_entry(value).is_occupied;
    }

    Optional<T*> find(T const& value)
    {
        if (m_count == 0)
            return {};

        if (auto& entry = find_entry(value); entry.is_occupied)
            return &entry.value;

        return {};
    }

    Optional<T const*> find(T const& value) const
    {
        return const_cast<HashTable*>(this)->find(value);
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

    void put_all(HashTable const& source)
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

    void clear()
    {
        for (auto& entry : m_entries)
            entry.clear();
        m_count = 0;
    }

    class Iterator {
    public:
        Iterator(Badge<HashTable>, HashTable const& table, size_t index)
            : m_table(table)
            , m_index(index)
        {
        }

        T const& operator*() { return m_table.m_entries[m_index].value; }
        T const* operator->() { return &m_table.m_entries[m_index].value; }

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
        HashTable const& m_table;
        size_t m_index;
    };
    friend Iterator;

    Iterator begin() const
    {
        return Iterator({}, *this, 0);
    }

    Iterator end() const
    {
        return Iterator({}, *this, m_entries.size());
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

    Entry& find_entry(T const& needle)
    {
        u32 hash = HashTraits<T>::hash(needle);
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
            } else if (!entry.is_occupied || (hash == HashTraits<T>::hash(entry.value) && HashTraits<T>::equals(needle, entry.value))) {
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

    Entry const& find_entry(T const& key) const
    {
        return const_cast<HashTable*>(this)->find_entry(key);
    }

    Entry& find_or_add_entry(T const& key)
    {
        auto expand_and_find_new_entry = [&] -> Entry& {
            auto new_capacity = max(8, ceil_s32((m_count + 1) / m_max_load_factor), capacity() * 2);
            expand(new_capacity);

            // We now have to search again, because the result we got before is now invalid
            return find_entry(key);
        };

        if (capacity() == 0) {
            // We're at 0 capacity, so expand
            return expand_and_find_new_entry();
        }

        auto& result = find_entry(key);
        if (!result.is_occupied) {
            // Expand if needed!
            if (m_count + 1 > (capacity() * m_max_load_factor))
                return expand_and_find_new_entry();
        }
        return result;
    }

    Span<Entry> m_entries {};
    size_t m_count { 0 };

    float m_max_load_factor { 0 };
};

//
// It's a Hash Table! Keys go in, items go out
//
// We have a couple of different ways of creating one:
// 1) initHashTable() - the HashTable will manage its own memory, and will grow to make room
// 2) allocateFixedSizeHashTable() - The HashTable's memory is allocated from an Arena, and it won't grow
// 3) initFixedSizeHashTable() - Like 2, except you manually pass the memory in, so you can get it from
//    anywhere. Use calculateHashTableDataSize() to do the working out for you.
//

template<typename T>
struct StringHashTableEntry {
    bool isOccupied;
    bool isGravestone; // Apparently the correct term is "tombstone", oops.

    String key;

    T value;

    void clear()
    {
        isOccupied = false;
        isGravestone = false;
        key = {};
        if constexpr (requires { value.~T(); }) {
            value.~T();
        } else {
            value = {};
        }
    }
};

template<typename T>
struct StringHashTableIterator;

template<typename T>
class StringHashTable {
    // FIXME: Hack so that StringTable::intern() can poke at things it shouldn't.
    friend StringTable;

    friend StringHashTableIterator<T>;

public:
    explicit StringHashTable(size_t initial_capacity, float max_load_factor = 0.75f)
        : m_max_load_factor(max_load_factor)
        , m_key_data_arena("HashTable"_s, 4_KB, 4_KB)
    {
        ASSERT(max_load_factor < 1.0f);

        if (initial_capacity > 0) {
            expand(ceil_s32(initial_capacity / max_load_factor));
        }
    }

    StringHashTable()
        : StringHashTable(0)
    {
    }

    StringHashTable(StringHashTable const& other)
        : StringHashTable(other.capacity(), other.m_max_load_factor)
    {
        put_all(other);
    }

    StringHashTable& operator=(StringHashTable const& other)
    {
        if (&other == this)
            return *this;

        clear();
        m_max_load_factor = other.m_max_load_factor;
        put_all(other);
        return *this;
    }

    StringHashTable(StringHashTable&& other)
        : m_entries(move(other.m_entries))
        , m_count(other.m_count)
        , m_max_load_factor(other.m_max_load_factor)
        , m_key_data_arena(move(other.m_key_data_arena))
    {
        other.m_entries = {};
        other.m_count = 0;
    }

    StringHashTable& operator=(StringHashTable&& other)
    {
        m_count = other.m_count;
        m_max_load_factor = other.m_max_load_factor;
        m_entries = move(other.m_entries);
        m_key_data_arena = move(other.m_key_data_arena);

        other.m_count = 0;
        other.m_entries = {};

        return *this;
    }

    ~StringHashTable()
    {
        if (!m_entries.is_empty())
            deallocate_raw(m_entries.raw_data());
    }

    size_t count() const { return m_count; }
    size_t capacity() const { return m_entries.size(); }

    bool contains(String key) const
    {
        if (m_entries == nullptr)
            return false;

        return find_entry(key)->isOccupied;
    }

    Optional<T*> find(String key)
    {
        if (m_entries.is_empty())
            return {};

        if (auto* entry = find_entry(key); entry->isOccupied)
            return &entry->value;

        return {};
    }

    Optional<T const*> find(String key) const
    {
        return const_cast<StringHashTable*>(this)->find(key);
    }

    Optional<T> find_value(String key) const
    {
        if (m_entries.is_empty())
            return {};

        if (auto* entry = find_entry(key); entry->isOccupied)
            return entry->value;

        return {};
    }

    T& ensure(String key, T value)
    {
        StringHashTableEntry<T>* entry = find_or_add_entry(key);
        if (!entry->isOccupied) {
            m_count++;
            entry->isOccupied = true;
            entry->isGravestone = false;

            String theKey = m_key_data_arena.allocate_string(key);
            theKey.hash();
            entry->key = theKey;

            new (&entry->value) T(move(value));
        }
        return entry->value;
    }

    T& put(String key, T value)
    {
        StringHashTableEntry<T>* entry = find_or_add_entry(key);

        if (!entry->isOccupied) {
            m_count++;
            entry->isOccupied = true;
            entry->isGravestone = false;

            String theKey = m_key_data_arena.allocate_string(key);
            theKey.hash();
            entry->key = theKey;
        }

        new (&entry->value) T(move(value));

        return entry->value;
    }

    void put_all(StringHashTable const& source)
    {
        // FIXME: Remove const cast when we're const correct
        for (auto it = const_cast<StringHashTable&>(source).iterate(); it.hasNext(); it.next()) {
            auto entry = it.getEntry();
            put(entry->key, entry->value);
        }
    }

    void remove(String key)
    {
        if (m_entries.is_empty())
            return;

        StringHashTableEntry<T>* entry = find_entry(key);
        if (entry->isOccupied) {
            entry->isGravestone = true;
            entry->isOccupied = false;
            m_count--;
        }
    }

    void clear()
    {
        if (m_count > 0) {
            m_count = 0;
            if (!m_entries.is_empty()) {
                for (size_t i = 0; i < capacity(); i++) {
                    m_entries[i].clear();
                }
            }
        }
    }

    StringHashTableIterator<T> iterate()
    {
        StringHashTableIterator<T> iterator = {};

        iterator.hashTable = this;
        iterator.currentIndex = 0;

        // If the table is empty, we can skip some work.
        iterator.isDone = m_count == 0;

        // If the first entry is unoccupied, we need to skip ahead
        if (!iterator.isDone && !iterator.getEntry()->isOccupied) {
            iterator.next();
        }

        return iterator;
    }

private:
    void expand(size_t newCapacity)
    {
        ASSERT(newCapacity > 0);
        ASSERT(newCapacity > capacity());

        size_t old_count = m_count;
        auto old_entries = m_entries;

        m_entries = { newCapacity, reinterpret_cast<StringHashTableEntry<T>*>(allocate_raw(newCapacity * sizeof(StringHashTableEntry<T>))) };
        m_count = 0;

        if (!old_entries.is_empty()) {
            // Migrate old entries over
            for (auto const& old_entry : old_entries) {
                if (old_entry.isOccupied)
                    put(old_entry.key, old_entry.value);
            }

            deallocate_raw(old_entries.raw_data());
        }

        ASSERT(old_count == m_count);
    }

    StringHashTableEntry<T>* find_entry(String key)
    {
        u32 hash = key.hash();
        u32 index = hash % capacity();
        StringHashTableEntry<T>* result = nullptr;

        // "Linear probing" - on collision, just keep going until you find an empty slot
        size_t itemsChecked = 0;
        while (true) {
            auto& entry = m_entries[index];

            if (entry.isGravestone) {
                // Store the first gravestone we find, in case we fail to find the "real" option
                if (result == nullptr)
                    result = &entry;
            } else if (!entry.isOccupied || (hash == entry.key.hash() && key == entry.key)) {
                // If the entry is unoccupied, we'd rather re-use the gravestone we found above
                if (entry.isOccupied || result == nullptr) {
                    result = &entry;
                }
                break;
            }

            index = (index + 1) % capacity();

            // Prevent the edge case infinite loop if all unoccupied spaces are gravestones
            itemsChecked++;
            if (itemsChecked >= capacity())
                break;
        }

        return result;
    }

    StringHashTableEntry<T> const* find_entry(String key) const
    {
        return const_cast<StringHashTable*>(this)->find_entry(key);
    }

    StringHashTableEntry<T>* find_or_add_entry(String key)
    {
        auto expand_and_find_new_entry = [&] {
            auto new_capacity = max(8, ceil_s32((m_count + 1) / m_max_load_factor), capacity() * 2);
            expand(new_capacity);

            // We now have to search again, because the result we got before is now invalid
            return find_entry(key);
        };

        if (capacity() == 0) {
            // We're at 0 capacity, so expand
            return expand_and_find_new_entry();
        }

        auto result = find_entry(key);
        if (!result->isOccupied) {
            // Expand if needed!
            if (m_count + 1 > (capacity() * m_max_load_factor))
                result = expand_and_find_new_entry();
        }
        return result;
    }

    Span<StringHashTableEntry<T>> m_entries {};
    size_t m_count { 0 };

    float m_max_load_factor { 0 };

    // @Size: In a lot of cases, we already store the key in a separate StringTable, so having
    // it stored here too is redundant. But, keys are small so it's unlikely to cause any real
    // issues.
    // - Sam, 08/01/2020
    // FIXME: Make Strings own their memory!
    MemoryArena m_key_data_arena;
};

template<typename T>
struct StringHashTableIterator {
    StringHashTable<T>* hashTable;
    s32 currentIndex;

    bool isDone;

    // Methods
    bool hasNext()
    {
        return !isDone;
    }

    void next()
    {
        while (!isDone) {
            currentIndex++;

            if (currentIndex >= hashTable->capacity()) {
                isDone = true;
            } else {
                // Only stop iterating if we find an occupied entry
                if (getEntry()->isOccupied) {
                    break;
                }
            }
        }
    }

    T* get()
    {
        return &getEntry()->value;
    }

    StringHashTableEntry<T>* getEntry()
    {
        return &hashTable->m_entries[currentIndex];
    }
};
