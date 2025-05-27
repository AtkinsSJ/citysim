#pragma once

template<typename T>
struct SetIterator;

template<typename T>
struct Set {
    // TODO: @Speed: This is a really dumb, linearly-compare-with-all-the-elements implementation!
    // Replace it with a binary tree or some kind of hash map or something, if we end up using this a lot.
    // For now, simple is better!
    // - Sam, 03/09/2019
    ChunkedArray<T> items;
    bool (*areItemsEqual)(T* a, T* b);

    // Methods

    // Returns true if the item got added (aka, it was not already in the set)
    bool add(T item);
    bool remove(T item);
    bool contains(T item);
    bool isEmpty();
    void clear();

    SetIterator<T> iterate();

    // The compare() function should return true if 'a' comes before 'b'
    Array<T> asSortedArray(bool (*compare)(T a, T b) = [](T a, T b) { return a < b; });
};

template<typename T>
void initSet(Set<T>* set, MemoryArena* arena, bool (*areItemsEqual)(T* a, T* b) = [](T* a, T* b) { return *a == *b; });

template<typename T>
struct SetIterator {
    // This is SUPER JANKY
    ChunkedArrayIterator<T> itemsIterator;

    // Methods
    bool hasNext();
    void next();
    T* get();
    T getValue();
};
