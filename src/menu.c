/* TN5250 - An implementation of the 5250 telnet protocol.
 * Copyright (C) 2005 James Rich
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

Tn5250Menubar *
tn5250_menubar_new ()
{
  Tn5250Menubar *This = tn5250_new (Tn5250Menubar, 1);
  if (This == NULL)
    {
      return NULL;
    }
  memset (This, 0, sizeof (Tn5250Menubar));
  This->next = NULL;
  This->prev = NULL;
  This->table = NULL;
  This->id = -1;
  return (This);
}


Tn5250Menubar *
tn5250_menubar_copy (Tn5250Menubar * This)
{
  Tn5250Menubar *win = tn5250_new (Tn5250Menubar, 1);

  if (win == NULL)
    {
      return NULL;
    }
  memcpy (win, This, sizeof (Tn5250Menubar));
  win->next = NULL;
  win->prev = NULL;
  return win;
}


void
tn5250_menubar_destroy (Tn5250Menubar * This)
{
  free (This);
}


int
tn5250_menubar_start_row (Tn5250Menubar * This)
{
  return (This->row);
}


int
tn5250_menubar_start_col (Tn5250Menubar * This)
{
  return (This->column);
}


int
tn5250_menubar_height (Tn5250Menubar * This)
{
  return (This->height);
}


int
tn5250_menubar_size (Tn5250Menubar * This)
{
  return (This->size);
}


int
tn5250_menubar_items (Tn5250Menubar * This)
{
  return (This->items);
}


Tn5250Menubar *
tn5250_menubar_list_destroy (Tn5250Menubar * list)
{
  Tn5250Menubar *iter, *next;

  if ((iter = list) != NULL)
    {
      do
	{
	  next = iter->next;
	  tn5250_menubar_destroy (iter);
	  iter = next;
	}
      while (iter != list);
    }
  return NULL;
}


Tn5250Menubar *
tn5250_menubar_list_add (Tn5250Menubar * list, Tn5250Menubar * node)
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


Tn5250Menubar *
tn5250_menubar_list_remove (Tn5250Menubar * list, Tn5250Menubar * node)
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


Tn5250Menubar *
tn5250_menubar_list_find_by_id (Tn5250Menubar * list, int id)
{
  Tn5250Menubar *iter;

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


Tn5250Menubar *
tn5250_menubar_list_copy (Tn5250Menubar * This)
{
  Tn5250Menubar *new_list = NULL, *iter, *new_menubar;
  if ((iter = This) != NULL)
    {
      do
	{
	  new_menubar = tn5250_menubar_copy (iter);
	  if (new_menubar != NULL)
	    {
	      new_list = tn5250_menubar_list_add (new_list, new_menubar);
	    }
	  iter = iter->next;
	}
      while (iter != This);
    }
  return new_list;
}


Tn5250Menubar *
tn5250_menubar_hit_test (Tn5250Menubar * list, int x, int y)
{
  Tn5250Menubar *iter;

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
