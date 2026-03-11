/*
 * Copyright (c) 2015-2025, Sam Atkins <sam@samatkins.co.uk>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <Util/Assert.h>
#include <Util/Basic.h>

/*****************************************************************
 * Somewhat less-hacky doubly-linked-list stuff using templates. *
 *****************************************************************/
template<typename T>
struct DeprecatedLinkedListNode {
    T* prevNode;
    T* nextNode;
};

template<typename T>
void initLinkedListSentinel(T* sentinel)
{
    sentinel->prevNode = sentinel;
    sentinel->nextNode = sentinel;
}

template<typename T>
void addToLinkedList(T* item, T* sentinel)
{
    item->prevNode = sentinel->prevNode;
    item->nextNode = sentinel;
    item->prevNode->nextNode = item;
    item->nextNode->prevNode = item;
}

template<typename T>
void removeFromLinkedList(T* item)
{
    item->nextNode->prevNode = item->prevNode;
    item->prevNode->nextNode = item->nextNode;

    item->nextNode = item->prevNode = item;
}

template<typename T>
bool linkedListIsEmpty(DeprecatedLinkedListNode<T>* sentinel)
{
    bool hasNext = (sentinel->nextNode != sentinel);
    bool hasPrev = (sentinel->prevNode != sentinel);
    ASSERT(hasNext == hasPrev); // Linked list is corrupted!
    return !hasNext;
}

template<typename T>
void moveAllNodes(T* srcSentinel, T* destSentinel)
{
    if (linkedListIsEmpty(srcSentinel))
        return;

    while (!linkedListIsEmpty(srcSentinel)) {
        T* node = srcSentinel->nextNode;
        removeFromLinkedList(node);
        addToLinkedList(node, destSentinel);
    }

    srcSentinel->nextNode = srcSentinel->prevNode = srcSentinel;
}

template<typename T>
s32 countNodes(T* sentinel)
{
    s32 result = 0;

    if (sentinel != nullptr) {
        T* cursor = sentinel->nextNode;

        while (cursor != sentinel) {
            ASSERT(cursor != nullptr); // null node in linked list
            cursor = cursor->nextNode;
            result++;
        }
    }

    return result;
}
