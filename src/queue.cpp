#pragma once


template <typename T>
bool Queue<T>::isEmpty()
{
	return (count == 0);
}

template <typename T>
T *Queue<T>::push(T item)
{
	return null;
}

template <typename T>
Maybe<T> Queue<T>::pop()
{
	return makeFailure<T>();
}
