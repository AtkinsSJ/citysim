#pragma once

/*****************************************************************
 * Somewhat less-hacky doubly-linked-list stuff using templates. *
 *****************************************************************/
template<typename T>
struct LinkedListNode {
    T* prevNode;
    T* nextNode;
};

template<typename T>
void initLinkedListSentinel(T* sentinel);

template<typename T>
void addToLinkedList(T* item, T* sentinel);

template<typename T>
void removeFromLinkedList(T* item);

template<typename T>
bool linkedListIsEmpty(LinkedListNode<T>* sentinel);

template<typename T>
void moveAllNodes(T* srcSentinel, T* destSentinel);

template<typename T>
s32 countNodes(T* sentinel);
