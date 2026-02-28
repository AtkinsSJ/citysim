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
        , m_has_fixed_memory(other.m_has_fixed_memory)
        , m_key_data_arena(move(other.m_key_data_arena))
    {
        other.m_entries = {};
        other.m_count = 0;
        other.m_has_fixed_memory = false;
    }

    HashTable& operator=(HashTable&& other)
    {
        m_count = other.m_count;
        m_max_load_factor = other.m_max_load_factor;
        m_has_fixed_memory = other.m_has_fixed_memory;
        m_entries = move(other.m_entries);
        m_key_data_arena = move(other.m_key_data_arena);

        other.m_count = 0;
        other.m_entries = {};
        other.m_has_fixed_memory = false;

        return *this;
    }

    static HashTable allocate_fixed_size(MemoryArena& arena, size_t capacity, float max_load_factor = 0.75f)
    {
        auto slot_count = static_cast<size_t>(ceil_s32(static_cast<float>(capacity) / max_load_factor));
        auto entries = arena.allocate_multiple<HashTableEntry<T>>(slot_count);

        return HashTable {
            { "FixedSizeHashTable"_s, KB(4), KB(4) },
            move(entries),
            max_load_factor
        };
    }

    ~HashTable()
    {
        // FIXME: We *should* clear() here, but right now that fails if this HashTable was allocated from a MemoryArena,
        //        because the arena may have already deallocated that memory.
        if (!m_has_fixed_memory && !m_entries.is_empty())
            deallocateRaw(m_entries.raw_data());
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
        return const_cast<HashTable*>(this)->find(key);
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
        HashTableEntry<T>* entry = find_or_add_entry(key);
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

    T& put(String key, T value)
    {
        HashTableEntry<T>* entry = find_or_add_entry(key);

        if (!entry->isOccupied) {
            m_count++;
            entry->isOccupied = true;
            entry->isGravestone = false;

            String theKey = m_key_data_arena.allocate_string(key);
            theKey.hash();
            entry->key = theKey;
        }

        entry->value = move(value);

        return entry->value;
    }

    void put_all(HashTable const& source)
    {
        // FIXME: Remove const cast when we're const correct
        for (auto it = const_cast<HashTable&>(source).iterate(); it.hasNext(); it.next()) {
            auto entry = it.getEntry();
            put(entry->key, entry->value);
        }
    }

    void remove(String key)
    {
        if (m_entries.is_empty())
            return;

        HashTableEntry<T>* entry = find_entry(key);
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
            if (!m_entries.is_empty()) {
                for (size_t i = 0; i < capacity(); i++) {
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
    HashTable(MemoryArena&& arena, Span<HashTableEntry<T>> entries, float max_load_factor)
        : m_entries(entries)
        , m_max_load_factor(max_load_factor)
        , m_has_fixed_memory(true)
        // TODO: Eliminate the keyDataArena somehow
        , m_key_data_arena(move(arena))
    {
    }

    void expand(size_t newCapacity)
    {
        ASSERT(!m_has_fixed_memory);
        ASSERT(newCapacity > 0);
        ASSERT(newCapacity > capacity());

        size_t old_count = m_count;
        auto old_entries = m_entries;

        m_entries = { newCapacity, reinterpret_cast<HashTableEntry<T>*>(allocateRaw(newCapacity * sizeof(HashTableEntry<T>))) };
        m_count = 0;

        if (!old_entries.is_empty()) {
            // Migrate old entries over
            for (auto const& old_entry : old_entries) {
                if (old_entry.isOccupied)
                    put(old_entry.key, old_entry.value);
            }

            deallocateRaw(old_entries.raw_data());
        }

        ASSERT(old_count == m_count);
    }

    HashTableEntry<T>* find_entry(String key)
    {
        u32 hash = key.hash();
        u32 index = hash % capacity();
        HashTableEntry<T>* result = nullptr;

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

    HashTableEntry<T> const* find_entry(String key) const
    {
        return const_cast<HashTable*>(this)->find_entry(key);
    }

    HashTableEntry<T>* find_or_add_entry(String key)
    {
        auto expand_and_find_new_entry = [&] {
            ASSERT(!m_has_fixed_memory);

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

    Span<HashTableEntry<T>> m_entries {};
    size_t m_count { 0 };

    float m_max_load_factor { 0 };
    bool m_has_fixed_memory { false }; // Fixed-memory HashTables don't expand in size

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
        return &hashTable->m_entries[currentIndex];
    }
};
