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
};

template<typename T>
struct HashTableIterator;

template<typename T>
struct HashTable {

    static smm calculate_desired_count(s32 capacity, float max_load_factor)
    {
        return ceil_s32(static_cast<float>(capacity) / max_load_factor);
    }

    explicit HashTable(s32 initial_capacity, float max_load_factor = 0.75f)
        : maxLoadFactor(max_load_factor)
        , keyDataArena("HashTable"_s, KB(4), KB(4))
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

    HashTable(HashTable&& other)
        : count(other.count)
        , capacity(other.capacity)
        , maxLoadFactor(other.maxLoadFactor)
        , hasFixedMemory(other.hasFixedMemory)
        , entries(move(other.entries))
        , keyDataArena(move(other.keyDataArena))
    {
    }

    HashTable& operator=(HashTable&& other)
    {
        count = other.count;
        capacity = other.capacity;
        maxLoadFactor = other.maxLoadFactor;
        hasFixedMemory = other.hasFixedMemory;
        entries = move(other.entries);
        keyDataArena = move(other.keyDataArena);
        return *this;
    }

    static HashTable allocate_fixed_size(MemoryArena& arena, s32 capacity, float max_load_factor = 0.75f)
    {
        auto slot_count = calculate_desired_count(capacity, max_load_factor);
        auto* entries = arena.allocate_multiple<HashTableEntry<T>>(slot_count);

        return HashTable {
            { "FixedSizeHashTable"_s, KB(4), KB(4) },
            entries,
            ceil_s32(static_cast<float>(capacity) / max_load_factor),
            max_load_factor
        };
    }

    ~HashTable()
    {
        if (!hasFixedMemory && entries != nullptr) {
            deallocateRaw(entries);
        }
    }

    s32 count { 0 };
    s32 capacity { 0 };
    float maxLoadFactor { 0 };
    bool hasFixedMemory { false }; // Fixed-memory HashTables don't expand in size
    HashTableEntry<T>* entries {};

    // @Size: In a lot of cases, we already store the key in a separate StringTable, so having
    // it stored here too is redundant. But, keys are small so it's unlikely to cause any real
    // issues.
    // - Sam, 08/01/2020
    // FIXME: Make Strings own their memory!
    MemoryArena keyDataArena;

    // Methods
    bool isInitialised()
    {
        return (entries != nullptr || maxLoadFactor > 0.0f);
    }

    bool contains(String key)
    {
        if (entries == nullptr)
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

        u32 hash = hashString(&key);
        u32 index = hash % capacity;
        HashTableEntry<T>* result = nullptr;

        // "Linear probing" - on collision, just keep going until you find an empty slot
        s32 itemsChecked = 0;
        while (true) {
            HashTableEntry<T>* entry = entries + index;

            if (entry->isGravestone) {
                // Store the first gravestone we find, in case we fail to find the "real" option
                if (result == nullptr)
                    result = entry;
            } else if ((entry->isOccupied == false) || (hash == hashString(&entry->key) && key == entry->key)) {
                // If the entry is unoccupied, we'd rather re-use the gravestone we found above
                if (entry->isOccupied || result == nullptr) {
                    result = entry;
                }
                break;
            }

            index = (index + 1) % capacity;

            // Prevent the edge case infinite loop if all unoccupied spaces are gravestones
            itemsChecked++;
            if (itemsChecked >= capacity)
                break;
        }

        return result;
    }

    HashTableEntry<T>* findOrAddEntry(String key)
    {
        ASSERT(isInitialised());

        HashTableEntry<T>* result = nullptr;

        if (capacity > 0) {
            result = findEntry(key);

            if (!result->isOccupied) {
                // Expand if needed!
                if (count + 1 > (capacity * maxLoadFactor)) {
                    ASSERT(!hasFixedMemory);

                    s32 newCapacity = max(
                        ceil_s32((count + 1) / maxLoadFactor),
                        (capacity < 8) ? 8 : capacity * 2);
                    expand(newCapacity);

                    // We now have to search again, because the result we got before is now invalid
                    result = findEntry(key);
                }
            }
        } else {
            // We're at 0 capacity, so expand
            s32 newCapacity = max(
                ceil_s32((count + 1) / maxLoadFactor),
                (capacity < 8) ? 8 : capacity * 2);
            expand(newCapacity);

            // We now have to search again, because the result we got before is now invalid
            result = findEntry(key);
        }

        return result;
    }
    Maybe<T*> find(String key)
    {
        Maybe<T*> result = makeFailure<T*>();

        if (entries != nullptr) {
            HashTableEntry<T>* entry = findEntry(key);
            if (entry->isOccupied) {
                result = makeSuccess(&entry->value);
            }
        }

        return result;
    }

    Maybe<T> findValue(String key)
    {
        Maybe<T> result = makeFailure<T>();

        if (entries != nullptr) {
            HashTableEntry<T>* entry = findEntry(key);
            if (entry->isOccupied) {
                result = makeSuccess(entry->value);
            }
        }

        return result;
    }

    T* findOrAdd(String key)
    {
        HashTableEntry<T>* entry = findOrAddEntry(key);
        if (!entry->isOccupied) {
            count++;
            entry->isOccupied = true;
            entry->isGravestone = false;

            String theKey = pushString(&keyDataArena, key);
            hashString(&theKey);
            entry->key = theKey;
        }

        return &entry->value;
    }

    T* put(String key, T value = {})
    {
        HashTableEntry<T>* entry = findOrAddEntry(key);

        if (!entry->isOccupied) {
            count++;
            entry->isOccupied = true;
            entry->isGravestone = false;

            String theKey = pushString(&keyDataArena, key);
            hashString(&theKey);
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

    void removeKey(String key)
    {
        if (entries == nullptr)
            return;

        HashTableEntry<T>* entry = findEntry(key);
        if (entry->isOccupied) {
            entry->isGravestone = true;
            entry->isOccupied = false;
            count--;
        }
    }

    void clear()
    {
        ASSERT(!hasFixedMemory);

        if (count > 0) {
            count = 0;
            if (entries != nullptr) {
                for (s32 i = 0; i < capacity; i++) {
                    entries[i] = {};
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
        iterator.isDone = (count == 0);

        // If the first entry is unoccupied, we need to skip ahead
        if (!iterator.isDone && !iterator.getEntry()->isOccupied) {
            iterator.next();
        }

        return iterator;
    }

private:
    HashTable(MemoryArena&& arena, HashTableEntry<T>* entries, s32 capacity, float max_load_factor)
        : capacity(capacity)
        , maxLoadFactor(max_load_factor)
        , hasFixedMemory(true)
        , entries(entries)
        // TODO: Eliminate the keyDataArena somehow
        , keyDataArena(move(arena))
    {
    }

    void expand(s32 newCapacity)
    {
        ASSERT(!hasFixedMemory);
        ASSERT(newCapacity > 0);        //, "Attempted to resize a hash table to {0}", {formatInt(newCapacity)});
        ASSERT(newCapacity > capacity); //, "Attempted to shrink a hash table from {0} to {1}", {formatInt(capacity), formatInt(newCapacity)});

        s32 oldCount = count;
        s32 oldCapacity = capacity;
        HashTableEntry<T>* oldItems = entries;

        capacity = newCapacity;
        entries = (HashTableEntry<T>*)allocateRaw(newCapacity * sizeof(HashTableEntry<T>));
        count = 0;

        if (oldItems != nullptr) {
            // Migrate old entries over
            for (s32 i = 0; i < oldCapacity; i++) {
                HashTableEntry<T>* oldEntry = oldItems + i;

                if (oldEntry->isOccupied) {
                    put(oldEntry->key, oldEntry->value);
                }
            }

            deallocateRaw(oldItems);
        }

        ASSERT(oldCount == count); //, "Hash table item count changed while expanding it! Old: {0}, new: {1}", {formatInt(oldCount), formatInt(table->count)});
    }
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

            if (currentIndex >= hashTable->capacity) {
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
        return hashTable->entries + currentIndex;
    }
};
