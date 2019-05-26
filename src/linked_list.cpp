#pragma once

template<typename T>
void initLinkedListSentinel(T *sentinel)
{
	sentinel->prevNode = sentinel;
	sentinel->nextNode = sentinel;
}

template<typename T>
void addToLinkedList(T *item, T *sentinel)
{
	item->prevNode = sentinel->prevNode;
	item->nextNode = sentinel;
	item->prevNode->nextNode = item;
	item->nextNode->prevNode = item;
}

template<typename T>
void removeFromLinkedList(T *item)
{
	item->nextNode->prevNode = item->prevNode;
	item->prevNode->nextNode = item->nextNode;

	item->nextNode = item->prevNode = item;
}

template<typename T>
bool linkedListIsEmpty(LinkedListNode<T> *sentinel)
{
	bool hasNext = (sentinel->nextNode != sentinel);
	bool hasPrev = (sentinel->prevNode != sentinel);
	ASSERT(hasNext == hasPrev, "Linked list is corrupted!");
	return !hasNext;
}

template<typename T>
void moveAllNodes(T *srcSentinel, T *destSentinel)
{
	if (linkedListIsEmpty(srcSentinel)) return;

	while (!linkedListIsEmpty(srcSentinel))
	{
		T *node = srcSentinel->nextNode;
		removeFromLinkedList(node);
		addToLinkedList(node, destSentinel);
	}

	srcSentinel->nextNode = srcSentinel->prevNode = srcSentinel;
}

template<typename T>
s32 countNodes(T *sentinel)
{
	DEBUG_FUNCTION();
	
	s32 result = 0;

	if (sentinel != null)
	{
		T *cursor = sentinel->nextNode;

		while (cursor != sentinel)
		{
			ASSERT(cursor != null, "null node in linked list");
			cursor = cursor->nextNode;
			result++;
		}
	}

	return result;
}
