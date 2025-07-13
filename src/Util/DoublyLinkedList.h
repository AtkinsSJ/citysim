/*
 * Copyright (c) 2015-2025, Sam Atkins <sam@ladybird.org>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <Util/Assert.h>
#include <Util/Basic.h>

template<typename T>
class DoublyLinkedList;

template<typename T>
class DoublyLinkedListNode {
    friend DoublyLinkedList<T>;

private:
    DoublyLinkedListNode* m_previous_node { this };
    DoublyLinkedListNode* m_next_node { this };
};

template<typename T>
class DoublyLinkedList {
public:
    u32 count() const
    {
        u32 count = 1;

        for (auto* node = m_sentinel.m_next_node; node != &m_sentinel; node = node->m_next_node) {
            count++;
        }

        return count;
    }

    bool is_empty() const
    {
        return m_sentinel.m_next_node == &m_sentinel;
    }

    void add(DoublyLinkedListNode<T>& node)
    {
        auto* previous_last_node = m_sentinel.m_previous_node;

        node.m_previous_node = previous_last_node;
        previous_last_node->m_next_node = &node;

        node.m_next_node = &m_sentinel;
        m_sentinel.m_previous_node = &node;
    }

    void remove(DoublyLinkedListNode<T>& node)
    {
        auto* previous_node = node.m_previous_node;
        auto* next_node = node.m_next_node;

        previous_node->m_next_node = next_node;
        next_node->m_previous_node = previous_node;

        node.m_previous_node = &node;
        node.m_next_node = &node;
    }

    T& remove_last()
    {
        ASSERT(!is_empty());
        auto& node = *m_sentinel.m_previous_node;
        remove(node);
        return static_cast<T&>(node);
    }

    void take_all(DoublyLinkedList& other)
    {
        if (other.is_empty())
            return;

        auto* previous_last_node = m_sentinel.m_previous_node;
        auto* other_first_node = other.m_sentinel.m_next_node;
        auto* other_last_node = other.m_sentinel.m_previous_node;

        // Insert other's items between our last node and our sentinel.
        other_first_node->m_previous_node = previous_last_node;
        previous_last_node->m_next_node = other_first_node;

        other_last_node->m_next_node = &m_sentinel;
        m_sentinel.m_previous_node = other_last_node;

        // Clear other
        other.m_sentinel.m_previous_node = &other.m_sentinel;
        other.m_sentinel.m_next_node = &other.m_sentinel;
    }

private:
    DoublyLinkedListNode<T> m_sentinel;
};
