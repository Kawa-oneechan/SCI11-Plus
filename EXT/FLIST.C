/*
 *	flist.c
 *
 * List manager for far lists.
 */

//KAWA WAS HERE
//-------------
// > General cleanup -- no ancient-style loose parameter type lists, // comments

#include "flist.h"
#include "memmgr.h"
#include "intrpt.h"


//Return true if this node is first in list (no previous node).
bool FIsFirstNode(word node)
{
	word prev;
	_cli();
	prev = (*W2H(node))->prev;
	_sti();
	return  prev == NULL;
}


// Returns first node in list (value in list.lHead will be zero for empty list).
word FFirstNode(FList *list)
{
	return list->head;
}


//Returns last node in list (value in list.lTail will be zero for empty list).
word FLastNode(FList *list)
{
	return list->tail;
}


//Return handle to next node in list.
word FNextNode(word node)
{
	word next;
	_cli();
	next = (*W2H(node))->next;
	_sti();
	return next;
}


//Return handle to previous node in list.
word FPrevNode(word node)
{
	word prev;
	_cli();
	prev = (*W2H(node))->prev;
	_sti();
	return prev;
}


//Add this element to front of list and return the node added.
word FAddToFront(FList *list, word node)
{
	FNodePtr np;
	_cli();
	np = *W2H(node);
	np->next = list->head;
	np->prev = NULL;
	if (list->tail == NULL)
		// An empty list.  The node is the new tail.
		list->tail = node;
	if (list->head != NULL)
		(*W2H(list->head))->prev = node;
	list->head = node;
	_sti();
	return node;
}


//Delete 'node' from the list.  Return TRUE if the list still has elements.
bool FDeleteNode(FList *list, word node)
{
	FNodePtr np;
	_cli();
	np = *W2H(node);
	if (node == list->head)
		list->head = np->next;
	if (node == list->tail)
		list->tail = np->prev;
	if (np->next)
		(*W2H(np->next))->prev = np->prev;
	if (np->prev)
		(*W2H(np->prev))->next = np->next;
	_sti();
	return list->head != NULL;
}


//Move this node to the front of the list.
word FMoveToFront(FList *list, word node)
{
	if (list->head != node)
	{
		FDeleteNode(list, node);
		FAddToFront(list, node);
	}
	return node;
}

