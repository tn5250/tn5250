/* TN5250 - An implementation of the 5250 telnet protocol.
 * Copyright (C) 2005-2008 James Rich
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

Tn5250Window *
tn5250_window_new ()
{
  Tn5250Window *This = tn5250_new (Tn5250Window, 1);
  if (This == NULL)
    {
      return NULL;
    }
  memset (This, 0, sizeof (Tn5250Window));
  This->next = NULL;
  This->prev = NULL;
  This->table = NULL;
  This->id = -1;
  return (This);
}


Tn5250Window *
tn5250_window_copy (Tn5250Window * This)
{
  Tn5250Window *win = tn5250_new (Tn5250Window, 1);

  if (win == NULL)
    {
      return NULL;
    }
  memcpy (win, This, sizeof (Tn5250Window));
  win->next = NULL;
  win->prev = NULL;
  return win;
}


void
tn5250_window_destroy (Tn5250Window * This)
{
  free (This);
}


int
tn5250_window_start_row (Tn5250Window * This)
{
  return (This->row);
}


int
tn5250_window_start_col (Tn5250Window * This)
{
  return (This->column);
}


int
tn5250_window_height (Tn5250Window * This)
{
  return (This->height);
}


int
tn5250_window_width (Tn5250Window * This)
{
  return (This->width);
}


Tn5250Window *
tn5250_window_list_destroy (Tn5250Window * list)
{
  Tn5250Window *iter, *next;

  if ((iter = list) != NULL)
    {
      do
	{
	  next = iter->next;
	  tn5250_window_destroy (iter);
	  iter = next;
	}
      while (iter != list);
    }
  return NULL;
}


Tn5250Window *
tn5250_window_list_add (Tn5250Window * list, Tn5250Window * node)
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


Tn5250Window *
tn5250_window_list_remove (Tn5250Window * list, Tn5250Window * node)
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


Tn5250Window *
tn5250_window_list_find_by_id (Tn5250Window * list, int id)
{
  Tn5250Window *iter;

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


Tn5250Window *
tn5250_window_list_copy (Tn5250Window * This)
{
  Tn5250Window *new_list = NULL, *iter, *new_window;
  if ((iter = This) != NULL)
    {
      do
	{
	  new_window = tn5250_window_copy (iter);
	  if (new_window != NULL)
	    {
	      new_list = tn5250_window_list_add (new_list, new_window);
	    }
	  iter = iter->next;
	}
      while (iter != This);
    }
  return new_list;
}


Tn5250Window *
tn5250_window_match_test (Tn5250Window * list, int x, int y, int columns,
			int rows)
{
  Tn5250Window *iter;

  if ((iter = list) != NULL)
    {
      do
	{
	  if ((iter->column == x) && (iter->row == y) &&
	      (iter->width == columns) && (iter->height == rows))
	    {
	      return iter;
	    }
	  iter = iter->next;
	}
      while (iter != list);
    }
  return NULL;
}


Tn5250Window *
tn5250_window_hit_test (Tn5250Window * list, int x, int y)
{
  Tn5250Window *iter;

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
