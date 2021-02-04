#pragma once

template <typename T>
void initStack(Stack<T> *stack, MemoryArena *arena, s32 itemsPerChunk)
{
	initChunkedArray(&stack->_array, arena, itemsPerChunk);
}

template <typename T>
inline bool isEmpty(Stack<T> *stack)
{
	return (stack->_array.count == 0);
}

template <typename T>
T *push(Stack<T> *stack, T item)
{
	return append(&stack->_array, item);
}

template <typename T>
T *peek(Stack<T> *stack)
{
	T *result = null;

	if (!isEmpty(stack))
	{
		result = get(&stack->_array, stack->_array.count-1);
	}

	return result;
}

template <typename T>
Maybe<T> pop(Stack<T> *stack)
{
	Maybe<T> result = makeFailure<T>();

	if (!isEmpty(stack))
	{
		result = makeSuccess(*get(&stack->_array, stack->_array.count-1));
		removeIndex(&stack->_array, stack->_array.count-1);
	}

	return result;
}
