//extern "C" by erno aug04
#include "..\Syslogserver\common_stdafx.h" 
extern "C" {

/*-----------------------------------------------------------------------------
 *
 * list.c - Dynamic list management package
 *
 *    Copyright (c) 1997-1998, SaberNet.net - All rights reserved
 *
 *    This program is free software; you can redistribute it and/or
 *    modify it under the terms of the GNU General Public License
 *    as published by the Free Software Foundation; either version 2
 *    of the License, or (at your option) any later version.
 *
 *    This program is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with this program; if not, write to the Free Software
 *    Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307
 *
 *  $Id: list.c,v 1.10 2002/07/23 06:36:51 jason Exp $
 *
 *  Revision History:
 *		15-Aug-98  JRR  Code restructured - added ListIterator
 *      04-May-97  JRR  Module completed
 *
 *----------------------------------------------------------------------------*/
#include <malloc.h>
#include "list.h"


/*------------------------------[private methods]-----------------------------*/
static int list_append_(List *list, void *item);
static int list_insert_(List *list, void *item, int (*comp)());
static int list_insert_at_(List *list, void *item, int index);


/*---------------------------------[list_new]---------------------------------
 * Inits a new list 
 *
 * Returns:
 *		success		pointer to a list
 *		failure		null
 *----------------------------------------------------------------------------*/
List *list_new()
{
	List *list = (List*)malloc(sizeof(List));

	if (list != NULL)
	{
		list->car  = NULL;
		list->cdr  = NULL;
	}

	return(list);
}

/*-----------------------------------[list_delete]------------------------------
 * Deletes a list 
 *
 *----------------------------------------------------------------------------*/
List* list_delete(List* list)
{
	list_flush(list, 1);
	return NULL;
}


/*--------------------------------[list_empty]--------------------------------
 * Check if the list is empty
 *
 *	Paramaters:
 *		list		pointer to a list struct
 *
 *  Returns:
 *		empty		(0)
 *		not empty	(!0)
 *----------------------------------------------------------------------------*/
int list_empty(List *list)
{
	return ( (list == NULL) || (list->car == NULL) );
}

/*---------------------------------[list_size]---------------------------------
 * Returns the number of items in the list
 *
 *	Paramaters:
 *		list		pointer to a list struct
 *
 *  Returns:
 *		success		number of items
 *		error		(0)
 *----------------------------------------------------------------------------*/
int list_size(List *list)
{
	int   size = 0;
	List *temp = list;

	while(temp != NULL) 
	{
		if (temp->car != NULL)
			size++;
		temp = temp->cdr;
	}

	return(size);
}

/*--------------------------------[list_flush]--------------------------------
 * Removes all items from the list
 *
 *	Paramaters:
 *		list		pointer to a list
 * 		destory		1 = free item, 0 = leave item intact
 *
 *	Returns:
 *		success		(0)
 *		failure		(-1)
 *----------------------------------------------------------------------------*/
int list_flush(List *list, int destroy)
{
	if (list == NULL)
		return(-1);

	while (!list_empty(list))
		list_remove_first(list, destroy);

	if (destroy)
		free(list);

	return(0);
}	


/*---------------------------------[list_add]---------------------------------
 * Adds an item at the top of the list
 *
 *	Paramaters:
 *		list		pointer to a list
 *      item		pointer to the item to add
 *
 *	Returns:
 *		success		(0)
 *		failure		(-1)
 *----------------------------------------------------------------------------*/
int list_add(List *list, void *item)
{  
	List *temp;

	if ( (list == NULL) || 
	     (item == NULL) || ((temp = list_new()) == NULL) )
		return(-1);

	temp->car = list->car;
	temp->cdr = list->cdr;
	list->cdr = temp;
	list->car = item;
	
	return(0);
} 

/*--------------------------------[list_append]--------------------------------
 * Appends an item to the end of a list
 *
 *	Paramaters:
 *		list		pointer to a list
 *      item		pointer to the item to add
 *
 *	Returns:
 *		success		(0)
 *		failure		(-1)
 *----------------------------------------------------------------------------*/
int list_append(List *list, void *item)
{
	int rc = -1;

	if ( (list != NULL) && (item != NULL) )
		rc = list_append_(list, item);

	return(rc);
}

/*--------------------------------[list_append_]-------------------------------
 * Recursive append operation
 *
 *	Returns:
 *		success		(0)
 *		failure		(-1)
 *----------------------------------------------------------------------------*/
static int list_append_(List *list, void *item)
{
	int rc;

	if (list_empty(list))
		rc = list_add(list, item);
	else
		rc = list_append_(list->cdr, item);

	return(rc);
}
	
/*-------------------------------[list_insert]--------------------------------
 * Inserts an item based on its sorted order.  Existing items will be shuffled 
 * down to make room for the item.
 *
 *	Paramaters:
 *		list		pointer to a list
 * 		item		item to insert into list
 *		comp		comparison function in the form: int comp(x, y)
 *                  which returns an integer greater than, equal to, or
 *                  less than 0, if the item x is greater than, equal
 *                  to, or less than item y.
 *	Returns:
 *		success		(0)
 *		failure		(-1)
 *----------------------------------------------------------------------------*/
/* erno not used, caused compiler error when moving from C to C++
/*int list_insert(List *list, void *item, int (*comp)())
{
	int rc = -1;

	if ( (list != NULL) && (item != NULL) && (comp != NULL) )
		rc  = list_insert_(list, item, comp);

	return(rc);
}
*/

/*-------------------------------[list_insert_]-------------------------------
 * Recursive insert method
 *
 *	Returns:
 *		success		(0)
 *		failure		(-1)
 *----------------------------------------------------------------------------*/
/* erno not used, caused compiler error when moving from C to C++
static int list_insert_(List *list, void *item, int (*comp)())
{


	int	rc;

	if (list_empty(list) || (comp(item, list->car) < 0))
		rc = list_add(list, item);
	else
		rc = list_insert_(list->cdr, item, comp);

	return(rc);
}
*/
/*------------------------------[list_insert_at]-------------------------------
 * Inserts an item at the specified index.  Existing items will be shuffled 
 * down to make room for the item.
 *
 *	Paramaters:
 *		list		pointer to a list
 * 		item		item to insert into list
 *      index       index to insert item at
 *
 *	Returns:
 *		success		(0)
 *		failure		(-1)
 *----------------------------------------------------------------------------*/
int list_insert_at(List *list, void *item, int index)
{
	int rc = -1;

	if ( (list != NULL) && (item != NULL) && (index >= 0) )
		rc = list_insert_at_(list, item, index);

	return(rc);
}

/*-----------------------------[list_insert_at_]------------------------------
 * Recursive insert method
 *
 *	Returns:
 *		success		(0)
 *		failure		(-1)
 *----------------------------------------------------------------------------*/
static int list_insert_at_(List *list, void *item, int index)
{
	int	rc;

	if ( (list_empty(list)) || (index == 0) )
		rc = list_add(list, item);
	else
		rc = list_insert_at_(list->cdr, item, index - 1);

	return(rc);
}

/*-----------------------------[list_remove_first]-----------------------------
 * Removes the first item in the list 
 *
 *	Paramaters:
 *		list		pointer to a list
 * 		destory		1 = free item, 0 = leave item intact
 *
 *	Returns:
 *		success		(0)
 *		failure		(-1)
 *----------------------------------------------------------------------------*/
int list_remove_first(List *list, int destroy)
{
	void *data;
	void *node;

	if (list_empty(list))
		return(-1);

	data = list->car;
	node = list->cdr;

	list->car = list->cdr->car;
	list->cdr = list->cdr->cdr;

	if (destroy)
		free(data);
	free(node);

	return(0);
}


/*--------------------------------[list_remove]--------------------------------
 * Removes the first matching item from the list
 *
 *	Paramaters:
 *		list		pointer to a list
 *		item		item to remove
 *		comp		comparison function in the form: int comp(x, y)
 *                  which returns an integer greater than, equal to, or
 *                  less than 0, if the item x is greater than, equal
 *                  to, or less than item y.
 * 		destory		1 = free item, 0 = leave item intact
 *
 *	Returns:
 *		success		(0)
 *		failure		(-1)
 *----------------------------------------------------------------------------*/
/* erno not used, caused compiler error when moving from C to C++
int list_remove(List *list, void *item, int (*comp)(), int destroy)
{
	List *templist = list;
	int	  rc       = -1;

	if ( (list == NULL) || (item == NULL) || (comp == NULL) )
		return(rc);

	while (templist->car != NULL)
	{
		if ((*comp)(item, templist->car) == 0)
			rc = list_remove_first(templist, destroy);
		if (templist->cdr != NULL)
			templist = templist->cdr;
		else
			break;
	}		

	return(rc);
}
*/

/*------------------------------[list_remove_at]------------------------------
 * Removes the item at the specified index
 *
 *	Paramaters:
 *		list		pointer to a list
 *		index		index of item to remove
 * 		destory		1 = free item, 0 = leave item intact
 *
 *	Returns:
 *		success		(0)
 *		failure		(-1)
 *----------------------------------------------------------------------------*/
int list_remove_at(List *list, int index, int destroy)
{
	int rc;

	if (list_empty(list))
		return(-1);

	if (index == 0)
		rc = list_remove_first(list, destroy);	
	else
		rc = list_remove_at(list->cdr, index - 1, destroy);

	return(rc);
}

/*---------------------------------[list_at]----------------------------------
 * Removes the item at the specified index
 *
 *	Paramaters:
 *		list		pointer to a list
 *		index		index of item to return
 *
 *	Returns:
 *		success		pointer to an object	
 *		failure		null
 *----------------------------------------------------------------------------*/
void *list_at(List *list, int index)
{
	if (list_empty(list))
		return(NULL);

	if (index == 0)
		return(list->car);
	else
		return(list_at(list->cdr, index - 1));
}


/*-------------------------------[list_print_on]-------------------------------
 * Print the contents of the list on a stream
 *
 *	Paramaters:
 *		list		pointer to a list
 *		fp			pointer to an open file descriptor
 *		func		print function in the form: void func(FILE*, void*)
 *
 *	Returns:
 *		success		(0)
 *		failure		(-1)
 *----------------------------------------------------------------------------*/
/* erno not used, caused compiler error when moving from C to C++
int list_print_on(List *list, FILE *fp, void (*func)())
{
	ListIterator	*iter;
	void 			*item;

	if ( (fp == NULL) || (func == NULL) |
         ((iter = list_iterator(list)) == NULL) )
		return(-1);

	item = list_first(iter);
	while (item != NULL)
	{
		(*func)(fp, item);
		item = list_next(iter);
	}

	free(iter);
	return(0);
}
*/
/*------------------------------[ list_iterator ]-----------------------------
 * Inits a new list iterator
 *
 *  Parameters:
 *		list		list to iterate
 *
 *	Returns:
 *		success		pointer to an iterator
 *		failure		null
 *----------------------------------------------------------------------------*/
ListIterator *list_iterator(List *list)
{
	ListIterator *iter = NULL;

	if (list != NULL)
	{
		iter = (ListIterator*)malloc(sizeof(ListIterator));
		iter->list = list;
		list_first(iter);
	}

	return(iter);
}

/*-----------------------------[ list_iterator_delete ]-------------------------
 * Deletes a list iterator
 *----------------------------------------------------------------------------*/
ListIterator* list_iterator_delete(ListIterator* iterator)
{
	free(iterator);
	return NULL;
}


/*-----------------------------------[ list_first ]---------------------------
 *  Returns the first object in the list
 *
 *  Parameters:
 *		iter		pointer to a list iterator
 *
 *  Return value:
 *		success		pointer to an object
 *		failure		null
 *----------------------------------------------------------------------------*/
void *list_first(ListIterator *iter)
{
	if (iter == NULL)
		return(NULL);

	iter->node  = iter->list;
	iter->index = 0;

	if (iter->node == NULL)
		return(NULL);

	return(iter->node->car);
}

/*------------------------------------[ list_next ]---------------------------
 *  Returns the next object in the list
 *
 *  Parameters:
 *		iter		pointer to a list iterator
 *
 *  Return value:
 *		success		pointer to an object
 *		failure		null
 *----------------------------------------------------------------------------*/
void *list_next(ListIterator *iter)
{
	if (iter == NULL)
		return(NULL);

	iter->node  = iter->node->cdr;
	iter->index++;

	if (iter->node == NULL)
		return(NULL);

	return(iter->node->car);
}
/*------------------------------[ list_current ]------------------------------
 * Returns the object at the current index
 *
 *  Parameters:
 *		iter		pointer to a list iterator
 *
 *  Return value:
 *		success		pointer to an object
 *		failure		null
 *----------------------------------------------------------------------------*/
void *list_current(ListIterator *iter)
{
	if (iter == NULL)
		return(NULL);

	return(iter->node->car);
}

/*-------------------------------[ list_index ]-------------------------------
 * Returns the object at the current index
 *
 *  Parameters:
 *      iter		pointer to a list iterator
 *
 *  Return value:
 *      success     current index
 *      failure    	(-1) 
 *----------------------------------------------------------------------------*/
int list_index(ListIterator *iter)
{
	if (iter == NULL)
		return(-1);

	return(iter->index);
}


/*---------------------------------[list_find]--------------------------------
 * Returns the index of the specified object
 *
 *	Paramaters:
 *		iter		pointer to a list iterator
 *      item		pointer to item to search for
 *		comp		comparison function in the form: int comp(x, y)
 *                  which returns an integer greater than, equal to, or
 *                  less than 0, if the item x is greater than, equal
 *                  to, or less than item y.
 *
 *  Returns:
 *		success		index of the requested item
 *		failure		(-1)
 *----------------------------------------------------------------------------*/
/* erno not used, caused compiler error when moving from C to C++
int list_find(ListIterator *iter, void *item, int (*comp)())
{
	if ( (iter != NULL) && (item != NULL) && (comp != NULL) )
	{
		void *tempitem = list_first(iter);

		while (tempitem != NULL)
		{
			if ((*comp)(item, tempitem) == 0)
				return(iter->index);
			else
				tempitem = list_next(iter);
		}
	}

	return(-1);
}
*/

} //end extern "C" by erno aug04

