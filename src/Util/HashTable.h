/*
 * Copyright (c) 2015-2025, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <Util/Basic.h>
#include <Util/Forward.h>
#include <Util/Maths.h>
#include <Util/MemoryArena.h>
#include <Util/String.h>

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
struct HashTableEntry {
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
struct HashTableIterator;

template<typename T>
class HashTable {
    // FIXME: Hack so that StringTable::intern() can poke at things it shouldn't.
    friend StringTable;

    friend HashTableIterator<T>;

public:
    static smm calculate_desired_count(size_t capacity, float max_load_factor)
    {
        return ceil_s32(static_cast<float>(capacity) / max_load_factor);
    }

    explicit HashTable(size_t initial_capacity, float max_load_factor = 0.75f)
        : m_max_load_factor(max_load_factor)
        , m_key_data_arena("HashTable"_s, KB(4), KB(4))
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
        : HashTable(other.m_capacity, other.m_max_load_factor)
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
        : m_count(other.m_count)
        , m_capacity(other.m_capacity)
        , m_max_load_factor(other.m_max_load_factor)
        , m_has_fixed_memory(other.m_has_fixed_memory)
        , m_entries(move(other.m_entries))
        , m_key_data_arena(move(other.m_key_data_arena))
    {
        other.m_count = 0;
        other.m_entries = nullptr;
        other.m_has_fixed_memory = false;
        other.m_entries = nullptr;
    }

    HashTable& operator=(HashTable&& other)
    {
        m_count = other.m_count;
        m_capacity = other.m_capacity;
        m_max_load_factor = other.m_max_load_factor;
        m_has_fixed_memory = other.m_has_fixed_memory;
        m_entries = move(other.m_entries);
        m_key_data_arena = move(other.m_key_data_arena);

        other.m_count = 0;
        other.m_capacity = 0;
        other.m_entries = nullptr;
        other.m_has_fixed_memory = false;

        return *this;
    }

    static HashTable allocate_fixed_size(MemoryArena& arena, size_t capacity, float max_load_factor = 0.75f)
    {
        auto slot_count = calculate_desired_count(capacity, max_load_factor);
        auto* entries = arena.allocate_multiple<HashTableEntry<T>>(slot_count);

        return HashTable {
            { "FixedSizeHashTable"_s, KB(4), KB(4) },
            entries,
            static_cast<size_t>(ceil(static_cast<float>(capacity) / max_load_factor)),
            max_load_factor
        };
    }

    ~HashTable()
    {
        if (!m_has_fixed_memory && m_entries != nullptr) {
            deallocateRaw(m_entries);
        }
    }

    // Methods
    bool isInitialised()
    {
        return (m_entries != nullptr || m_max_load_factor > 0.0f);
    }

    size_t count() const { return m_count; }
    size_t capacity() const { return m_capacity; }

    bool contains(String key)
    {
        if (m_entries == nullptr)
            return false;

        HashTableEntry<T>* entry = findEntry(key);
        if (!entry->isOccupied) {
            return false;
        } else {
            return true;
        }
    }

    HashTableEntry<T>* findEntry(String key)
    {
        ASSERT(isInitialised());

        u32 hash = key.hash();
        u32 index = hash % m_capacity;
        HashTableEntry<T>* result = nullptr;

        // "Linear probing" - on collision, just keep going until you find an empty slot
        size_t itemsChecked = 0;
        while (true) {
            HashTableEntry<T>* entry = m_entries + index;

            if (entry->isGravestone) {
                // Store the first gravestone we find, in case we fail to find the "real" option
                if (result == nullptr)
                    result = entry;
            } else if ((entry->isOccupied == false) || (hash == entry->key.hash() && key == entry->key)) {
                // If the entry is unoccupied, we'd rather re-use the gravestone we found above
                if (entry->isOccupied || result == nullptr) {
                    result = entry;
                }
                break;
            }

            index = (index + 1) % m_capacity;

            // Prevent the edge case infinite loop if all unoccupied spaces are gravestones
            itemsChecked++;
            if (itemsChecked >= m_capacity)
                break;
        }

        return result;
    }

    HashTableEntry<T>* findOrAddEntry(String key)
    {
        ASSERT(isInitialised());

        auto expand_and_find_new_entry = [&] {
            ASSERT(!m_has_fixed_memory);

            auto new_capacity = max(8, ceil_s32((m_count + 1) / m_max_load_factor), m_capacity * 2);
            expand(new_capacity);

            // We now have to search again, because the result we got before is now invalid
            return findEntry(key);
        };

        if (m_capacity == 0) {
            // We're at 0 capacity, so expand
            return expand_and_find_new_entry();
        }

        auto result = findEntry(key);
        if (!result->isOccupied) {
            // Expand if needed!
            if (m_count + 1 > (m_capacity * m_max_load_factor))
                result = expand_and_find_new_entry();
        }
        return result;
    }

    Optional<T*> find(String key)
    {
        if (!m_entries)
            return {};

        if (HashTableEntry<T>* entry = findEntry(key); entry->isOccupied)
            return &entry->value;

        return {};
    }

    Optional<T> find_value(String key)
    {
        if (!m_entries)
            return {};

        if (HashTableEntry<T>* entry = findEntry(key); entry->isOccupied)
            return entry->value;

        return {};
    }

    T* findOrAdd(String key)
    {
        HashTableEntry<T>* entry = findOrAddEntry(key);
        if (!entry->isOccupied) {
            m_count++;
            entry->isOccupied = true;
            entry->isGravestone = false;

            String theKey = m_key_data_arena.allocate_string(key);
            theKey.hash();
            entry->key = theKey;
        }

        return &entry->value;
    }

    T& ensure(String key, T value)
    {
        HashTableEntry<T>* entry = findOrAddEntry(key);
        if (!entry->isOccupied) {
            m_count++;
            entry->isOccupied = true;
            entry->isGravestone = false;

            String theKey = m_key_data_arena.allocate_string(key);
            theKey.hash();
            entry->key = theKey;

            entry->value = move(value);
        }
        return entry->value;
    }

    T* put(String key, T value = {})
    {
        HashTableEntry<T>* entry = findOrAddEntry(key);

        if (!entry->isOccupied) {
            m_count++;
            entry->isOccupied = true;
            entry->isGravestone = false;

            String theKey = m_key_data_arena.allocate_string(key);
            theKey.hash();
            entry->key = theKey;
        }

        entry->value = value;

        return &entry->value;
    }

    void putAll(HashTable<T>* source)
    {
        for (auto it = source->iterate(); it.hasNext(); it.next()) {
            auto entry = it.getEntry();
            put(entry->key, entry->value);
        }
    }

    void put_all(HashTable const& source)
    {
        // FIXME: Remove const cast when we're const correct
        for (auto it = const_cast<HashTable&>(source).iterate(); it.hasNext(); it.next()) {
            auto entry = it.getEntry();
            put(entry->key, entry->value);
        }
    }

    void removeKey(String key)
    {
        if (m_entries == nullptr)
            return;

        HashTableEntry<T>* entry = findEntry(key);
        if (entry->isOccupied) {
            entry->isGravestone = true;
            entry->isOccupied = false;
            m_count--;
        }
    }

    void clear()
    {
        ASSERT(!m_has_fixed_memory);

        if (m_count > 0) {
            m_count = 0;
            if (m_entries != nullptr) {
                for (size_t i = 0; i < m_capacity; i++) {
                    m_entries[i].clear();
                }
            }
        }
    }

    HashTableIterator<T> iterate()
    {
        HashTableIterator<T> iterator = {};

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
    HashTable(MemoryArena&& arena, HashTableEntry<T>* entries, size_t capacity, float max_load_factor)
        : m_capacity(capacity)
        , m_max_load_factor(max_load_factor)
        , m_has_fixed_memory(true)
        , m_entries(entries)
        // TODO: Eliminate the keyDataArena somehow
        , m_key_data_arena(move(arena))
    {
    }

    void expand(size_t newCapacity)
    {
        ASSERT(!m_has_fixed_memory);
        ASSERT(newCapacity > 0);
        ASSERT(newCapacity > m_capacity);

        size_t old_count = m_count;
        size_t old_capacity = m_capacity;
        HashTableEntry<T>* old_entries = m_entries;

        m_capacity = newCapacity;
        m_entries = (HashTableEntry<T>*)allocateRaw(newCapacity * sizeof(HashTableEntry<T>));
        m_count = 0;

        if (old_entries != nullptr) {
            // Migrate old entries over
            for (size_t i = 0; i < old_capacity; i++) {
                HashTableEntry<T>* oldEntry = old_entries + i;

                if (oldEntry->isOccupied) {
                    put(oldEntry->key, oldEntry->value);
                }
            }

            deallocateRaw(old_entries);
        }

        ASSERT(old_count == m_count);
    }

    size_t m_count { 0 };
    size_t m_capacity { 0 };

    float m_max_load_factor { 0 };
    bool m_has_fixed_memory { false }; // Fixed-memory HashTables don't expand in size
    HashTableEntry<T>* m_entries {};

    // @Size: In a lot of cases, we already store the key in a separate StringTable, so having
    // it stored here too is redundant. But, keys are small so it's unlikely to cause any real
    // issues.
    // - Sam, 08/01/2020
    // FIXME: Make Strings own their memory!
    MemoryArena m_key_data_arena;
};

template<typename T>
struct HashTableIterator {
    HashTable<T>* hashTable;
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

    HashTableEntry<T>* getEntry()
    {
        return hashTable->m_entries + currentIndex;
    }
};
