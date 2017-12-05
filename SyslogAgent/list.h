//extern "C" by erno aug04
extern "C" {

/*-----------------------------------------------------------------------------
 *
 *  list.h - Dynamic list management package
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
 *  $Id: list.h,v 1.10 2002/07/23 06:36:51 jason Exp $
 *
 *----------------------------------------------------------------------------*/
#include <stdio.h>

#ifndef _LIST_H_
#define _LIST_H_


struct _List
{
	void 				*car;		/* pointer to data                        */
	struct _List		*cdr;		/* pointer to next node                   */
};
typedef struct _List List;


typedef struct
{
	List				*list;		/* pointer to the list                    */
	List				*node;		/* current node                           */
	int					index;		/* current index                          */

} ListIterator;


List* list_new();
List* list_delete(List* list);

int   list_empty(List *list);
int   list_size(List *list);
int   list_flush(List *list, int destroy);
int   list_add(List *list, void *item);
int   list_append(List *list, void *item);
int   list_insert(List *list, void *item, int (*comp)());
int   list_insert_at(List *list, void *item, int index);
int   list_remove_first(List *list, int destroy);
int   list_remove(List *list, void *item, int (*comp)(), int destroy);
int   list_remove_at(List *list, int index, int destroy);
void *list_at(List *list, int index);

int   list_print_on(List *list, FILE *fp, void (*func)());


ListIterator* list_iterator(List *list);
ListIterator* list_iterator_delete(ListIterator* iterator);

void *list_first(ListIterator *iterator);
void *list_next(ListIterator *iterator);
void *list_current(ListIterator *iterator);
int   list_index(ListIterator *iterator);
int   list_find(ListIterator *iterator, void *item, int (*comp)());

#endif

} //end extern "C" by erno aug04

