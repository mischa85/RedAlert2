// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright 2026 ra2-decomp contributors
// Copyright 2025 Electronic Arts Inc.
// Derived from Electronic Arts source code; EA's GPL Section 7 terms apply (COPYING.md).
#include "audlist.h"
#include <stddef.h>

void __fastcall ListInit(ListHead * head)
{
	head->prev = head->next = head;
	head->pri = (Priority)head;
}

void __fastcall ListNodeInit(ListNode * node)
{
	node->prev = node->next = node;
	node->pri = 0;
}

int __fastcall ListAddNodeSortAscending(ListHead * head, ListNode * new_node)
{
	ListNode * node;
	Priority pri;
	int index;

	index = 0;
	pri = new_node->pri;
	node = (ListNode *)head;
	while ((node = ListNodeNext(node))) {
		if (pri <= node->pri) {
			ListNodeInsert(node, new_node);
			return index;
		}
		index++;
	}

	ListNodeInsert(head, new_node);
	return index;
}

void __fastcall ListAddNode(ListHead * head, ListNode * new_node)
{
	ListNode * node;
	Priority pri;

	pri = new_node->pri;
	node = (ListNode *)head;
	while ((node = ListNodeNext(node))) {
		if (node->pri <= pri) {
			ListNodeInsert(node, new_node);
			return;
		}
	}

	ListNodeInsert(head, new_node);
}

void __fastcall ListMerge(ListHead * from, ListHead * to)
{
	ListNode * first;
	ListNode * last;
	ListNode * node;

	first = from->next;
	last = from->prev;

	if (first == (ListNode *)from) {
		return;
	}

	node = to->prev;
	node->next = first;
	first->prev = node;
	last->next = (ListNode *)to;
	to->prev = last;

	ListInit(from);
}

int __fastcall ListCountItems(ListHead * head)
{
	ListNode * node;
	int count = 0;

	node = head->next;

	while (node != (ListNode *)head) {
		count++;
		node = node->next;
	}

	return count;
}

ListNode * __fastcall ListFirstItem(ListHead * head)
{
	return ListNextItem((ListNode *)head);
}

ListNode * __fastcall ListNextItem(ListNode * node)
{
	if (!node) {
		return NULL;
	}
	return (ListNodeNext(node));
}

ListNode * __fastcall ListGetItem(ListHead * head, int number)
{
	ListNode * node;

	node = head->next;

	while (node != (ListNode *)head) {
		if (number-- == 0) {
			return node;
		}
		node = node->next;
	}

	return NULL;
}

void __fastcall ListNodeInsert(ListNode * node, ListNode * new_node)
{
	new_node->prev = node->prev;
	new_node->next = node;
	node->prev = new_node;
	new_node->prev->next = new_node;
}

void __fastcall ListNodeAppend(ListNode * node, ListNode * new_node)
{
	new_node->prev = node;
	new_node->next = node->next;
	node->next = new_node;
	new_node->next->prev = new_node;
}

void __fastcall ListNodeRemove(ListNode * node)
{
	node->prev->next = node->next;
	node->next->prev = node->prev;
	node->prev = node->next = node;
}

ListNode * __fastcall ListNodeNext(ListNode * node)
{
	ListNode * next;

	next = node->next;

	if (next && ListNodeIsHead(next)) {
		return NULL;
	}

	return next;
}

ListNode * __fastcall ListNodePrev(ListNode * node)
{
	ListNode * next;

	next = node->prev;

	if (ListNodeIsHead(next)) {
		return NULL;
	}

	return next;
}

ListNode * __fastcall ListNodeLoopNext(ListNode * node)
{
	ListNode * next;

	next = node->next;

	if (ListNodeIsHead(next)) {
		next = next->next;
		if (ListNodeIsHead(next)) {
			return NULL;
		}
	}

	return next;
}

ListNode * __fastcall ListNodeLoopPrev(ListNode * node)
{
	ListNode * next;

	next = node->prev;

	if (ListNodeIsHead(next)) {
		next = next->prev;
		if (ListNodeIsHead(next)) {
			return NULL;
		}
	}

	return next;
}
