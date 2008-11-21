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
  This->menuitem_list = NULL;
  This->menuitem_count = 0;
  This->table = NULL;
  This->id = -1;
  return (This);
}


Tn5250Menubar *
tn5250_menubar_copy (Tn5250Menubar * This)
{
  Tn5250Menubar *menu = tn5250_new (Tn5250Menubar, 1);

  if (menu == NULL)
    {
      return NULL;
    }
  memcpy (menu, This, sizeof (Tn5250Menubar));
  menu->next = NULL;
  menu->prev = NULL;
  return menu;
}


void
tn5250_menubar_destroy (Tn5250Menubar * This)
{
  free (This);
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
	  /* We want to find a hit for any column in the menubar row */
	  /*
	   * if ((x >= iter->column)
	   *   && (x <= (iter->column + (iter->itemsize * iter->items)))
	   *   && (y >= iter->row) && (y <= (iter->row + iter->height - 1)))
	   */
	  if ((y >= iter->row) && (y <= (iter->row + iter->height - 1)))
	    {
	      return iter;
	    }
	  iter = iter->next;
	}
      while (iter != list);
    }
  return NULL;
}


void
tn5250_menubar_select_next (Tn5250Menubar * This, int *x, int *y)
{
  Tn5250Menuitem *menuitem =
    tn5250_menuitem_hit_test (This->menuitem_list, *x, *y);

  if (menuitem == NULL)
    {
      menuitem = This->menuitem_list->prev;
    }

  menuitem->selected = 0;
  menuitem = menuitem->next;
  menuitem->selected = 1;
  *y = tn5250_menuitem_start_row (menuitem);
  *x = tn5250_menuitem_start_col (menuitem);
  return;
}


void
tn5250_menubar_select_prev (Tn5250Menubar * This, int *x, int *y)
{
  Tn5250Menuitem *menuitem =
    tn5250_menuitem_hit_test (This->menuitem_list, *x, *y);

  if (menuitem == NULL)
    {
      menuitem = This->menuitem_list;
    }

  menuitem->selected = 0;
  menuitem = menuitem->prev;
  menuitem->selected = 1;
  *y = tn5250_menuitem_start_row (menuitem);
  *x = tn5250_menuitem_start_col (menuitem);
  return;
}


/***** lib5250/tn5250_menu_add_menuitem
 * NAME
 *    tn5250_menu_add_menuitem
 * SYNOPSIS
 *    tn5250_menu_add_menuitem (This, menuitem);
 * INPUTS
 *    Tn5250Menubar *      This       -
 *    Tn5250Menuitem *     menuitem   -
 * DESCRIPTION
 *    DOCUMENT ME!!!
 *****/
void
tn5250_menu_add_menuitem (Tn5250Menubar * This, Tn5250Menuitem * menuitem)
{
  menuitem->id = This->menuitem_count++;
  menuitem->menubar = This;
  This->menuitem_list = tn5250_menuitem_list_add (This->menuitem_list,
						  menuitem);

  TN5250_LOG (("adding menu item: menuitem->id: %d\n", menuitem->id));
  return;
}


Tn5250Menuitem *
tn5250_menuitem_new ()
{
  Tn5250Menuitem *This = tn5250_new (Tn5250Menuitem, 1);
  if (This == NULL)
    {
      return NULL;
    }
  memset (This, 0, sizeof (Tn5250Menuitem));
  This->next = NULL;
  This->prev = NULL;
  This->id = -1;
  This->size = 0;
  This->available = 0;
  This->selected = 0;
  return (This);
}


Tn5250Menuitem *
tn5250_menuitem_list_add (Tn5250Menuitem * list, Tn5250Menuitem * node)
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


Tn5250Menuitem *
tn5250_menuitem_list_remove (Tn5250Menuitem * list, Tn5250Menuitem * node)
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


int
tn5250_menuitem_new_row (Tn5250Menuitem * This)
{
  unsigned char menutype = tn5250_menubar_type (This->menubar);

  switch (menutype)
    {
    case MENU_TYPE_MENUBAR:
      if (This->prev == This)
	{
	  return (This->menubar->row);
	}
      else
	{
	  return (This->prev->row);
	}
      break;
    case MENU_TYPE_SINGLE_SELECT_FIELD:
    case MENU_TYPE_MULTIPLE_SELECT_FIELD:
    case MENU_TYPE_SINGLE_SELECT_LIST:
    case MENU_TYPE_MULTIPLE_SELECT_LIST:
    case MENU_TYPE_SINGLE_SELECT_FIELD_PULL_DOWN:
    case MENU_TYPE_MULTIPLE_SELECT_FIELD_PULL_DOWN:
    case MENU_TYPE_PUSH_BUTTONS:
    case MENU_TYPE_PUSH_BUTTONS_PULL_DOWN:
      if (This->prev == This)
	{
	  return (This->menubar->row);
	}
      else
	{
	  return (This->prev->row + 1);
	}
      break;
    default:
      TN5250_LOG (("Invalid selection field type!!\n"));
      break;
    }
  return 0;
}


int
tn5250_menuitem_new_col (Tn5250Menuitem * This)
{
  unsigned char menutype = tn5250_menubar_type (This->menubar);

  switch (menutype)
    {
    case MENU_TYPE_MENUBAR:
      if (This->prev == This)
	{
	  return (This->menubar->column + 1);
	}
      else
	{
	  return (This->prev->column + This->prev->size + 1);
	}
      break;
    case MENU_TYPE_SINGLE_SELECT_FIELD:
    case MENU_TYPE_MULTIPLE_SELECT_FIELD:
    case MENU_TYPE_SINGLE_SELECT_LIST:
    case MENU_TYPE_MULTIPLE_SELECT_LIST:
    case MENU_TYPE_SINGLE_SELECT_FIELD_PULL_DOWN:
    case MENU_TYPE_MULTIPLE_SELECT_FIELD_PULL_DOWN:
    case MENU_TYPE_PUSH_BUTTONS:
    case MENU_TYPE_PUSH_BUTTONS_PULL_DOWN:
      return (This->menubar->column + 1);
      break;
    default:
      TN5250_LOG (("Invalid selection field type!!\n"));
      break;
    }
  return 0;
}


Tn5250Menuitem *
tn5250_menuitem_list_find_by_id (Tn5250Menuitem * list, int id)
{
  Tn5250Menuitem *iter;

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


Tn5250Menuitem *
tn5250_menuitem_hit_test (Tn5250Menuitem * list, int x, int y)
{
  Tn5250Menuitem *iter;

  if ((iter = list) != NULL)
    {
      do
	{
	  if ((x >= iter->column) && (x <= (iter->column + iter->size))
	      && (iter->row == y))
	    {
	      return iter;
	    }
	  iter = iter->next;
	}
      while (iter != list);
    }
  return NULL;
}
