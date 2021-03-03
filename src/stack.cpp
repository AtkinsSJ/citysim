#pragma once

template <typename T>
void initStack(Stack<T> *stack, MemoryArena *arena, s32 itemsPerChunk)
{
	initChunkedArray(&stack->_array, arena, itemsPerChunk);
}

template <typename T>
inline bool isEmpty(Stack<T> *stack)
{
	return (stack->_array.isEmpty());
}

template <typename T>
T *push(Stack<T> *stack, T item)
{
	return stack->_array.append(item);
}

template <typename T>
T *peek(Stack<T> *stack)
{
	T *result = null;

	if (!isEmpty(stack))
	{
		result = stack->_array.get(stack->_array.count-1);
	}

	return result;
}

template <typename T>
Maybe<T> pop(Stack<T> *stack)
{
	Maybe<T> result = makeFailure<T>();

	if (!isEmpty(stack))
	{
		result = makeSuccess(*stack->_array.get(stack->_array.count-1));
		removeIndex(&stack->_array, stack->_array.count-1);
	}

	return result;
}
