#pragma once

//
// A stack! What more is there to say?
//
// For now (04/02/2021) this just wraps a ChunkedArray and limits what you can do with it.
// That saves a lot of effort and duplication. But it's a bit inelegant, so maybe this should
// be rewritten later. IDK! We basically are only making this a thing, instead of using a
// ChunkedArray directly, because we don't want user code to accidentally do operations on
// it that might leave a gap, as that would break the stack logic completely. Well, rather,
// it would make the stack logic a lot more complicated to check for gaps.
//

template<typename T>
struct Stack {
    ChunkedArray<T> _array;
};

template<typename T>
void initStack(Stack<T>* stack, MemoryArena* arena, s32 itemsPerChunk = 32);

template<typename T>
bool isEmpty(Stack<T>* stack);

template<typename T>
T* push(Stack<T>* stack, T item);

template<typename T>
T* peek(Stack<T>* stack);

template<typename T>
Maybe<T> pop(Stack<T>* stack);
