/* TN5250 - An implementation of the 5250 telnet protocol.
 * Copyright (C) 2000 Jay 'Eraserhead' Felice
 * 
 * This file is part of TN5250.
 *
 * TN5250 is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1, or (at your option)
 * any later version.
 * 
 * TN5250 is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public License
 * along with this software; see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 59 Temple Place, Suite 330,
 * Boston, MA 02111-1307 USA
 * 
 */
#include "tn5250-private.h"

Tn5250Menu *
tn5250_menu_new ()
{
  Tn5250Menu *This = tn5250_new (Tn5250Menu, 1);
  if (This == NULL)
    {
      return NULL;
    }
  memset (This, 0, sizeof (Tn5250Menu));
  This->next = NULL;
  This->prev = NULL;
  This->table = NULL;
  This->id = -1;
  return (This);
}


Tn5250Menu *
tn5250_menu_copy (Tn5250Menu * This)
{
  Tn5250Menu *win = tn5250_new (Tn5250Menu, 1);

  if (win == NULL)
    {
      return NULL;
    }
  memcpy (win, This, sizeof (Tn5250Menu));
  win->next = NULL;
  win->prev = NULL;
  return win;
}


void
tn5250_menu_destroy (Tn5250Menu * This)
{
  free (This);
}


int
tn5250_menu_start_row (Tn5250Menu * This)
{
  return (This->row);
}


int
tn5250_menu_start_col (Tn5250Menu * This)
{
  return (This->column);
}


int
tn5250_menu_height (Tn5250Menu * This)
{
  return (This->height);
}


int
tn5250_menu_size (Tn5250Menu * This)
{
  return (This->size);
}


int
tn5250_menu_items (Tn5250Menu * This)
{
  return (This->items);
}


Tn5250Menu *
tn5250_menu_list_destroy (Tn5250Menu * list)
{
  Tn5250Menu *iter, *next;

  if ((iter = list) != NULL)
    {
      do
	{
	  next = iter->next;
	  tn5250_menu_destroy (iter);
	  iter = next;
	}
      while (iter != list);
    }
  return NULL;
}


Tn5250Menu *
tn5250_menu_list_add (Tn5250Menu * list, Tn5250Menu * node)
{
  node->prev = node->next = NULL;

  if (list == NULL)
    {
      node->next = node->prev = node;
      return node;
    }
  node->next = list;
  node->prev = list->prev;
  node->prev->next = node;
  node->next->prev = node;
  return list;
}


Tn5250Menu *
tn5250_menu_list_remove (Tn5250Menu * list, Tn5250Menu * node)
{
  if (list == NULL)
    {
      return NULL;
    }
  if ((list->next == list) && (list == node))
    {
      node->next = node->prev = NULL;
      return NULL;
    }
  if (list == node)
    {
      list = list->next;
    }

  node->next->prev = node->prev;
  node->prev->next = node->next;
  node->prev = node->next = NULL;
  return list;
}


Tn5250Menu *
tn5250_menu_list_find_by_id (Tn5250Menu * list, int id)
{
  Tn5250Menu *iter;

  if ((iter = list) != NULL)
    {
      do
	{
	  if (iter->id == id)
	    {
	      return iter;
	    }
	  iter = iter->next;
	}
      while (iter != list);
    }
  return NULL;
}


Tn5250Menu *
tn5250_menu_list_copy (Tn5250Menu * This)
{
  Tn5250Menu *new_list = NULL, *iter, *new_menu;
  if ((iter = This) != NULL)
    {
      do
	{
	  new_menu = tn5250_menu_copy (iter);
	  if (new_menu != NULL)
	    {
	      new_list = tn5250_menu_list_add (new_list, new_menu);
	    }
	  iter = iter->next;
	}
      while (iter != This);
    }
  return new_list;
}


Tn5250Menu *
tn5250_menu_hit_test (Tn5250Menu * list, int x, int y)
{
  Tn5250Menu *iter;

  if ((iter = list) != NULL)
    {
      do
	{
	  if ((iter->column == x) && (iter->row == y))
	    {
	      return iter;
	    }
	  iter = iter->next;
	}
      while (iter != list);
    }
  return NULL;
}
