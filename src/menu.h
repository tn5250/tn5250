#ifndef MENU_H
#define MENU_H

/* TN5250 - An implementation of the 5250 telnet protocol.
 * Copyright (C) 1997 Michael Madore
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

#ifdef __cplusplus
extern "C"
{
#endif

#define MENU_TYPE_MENUBAR 0x01 /* Menu bar */
#define MENU_TYPE_SINGLE_SELECT_FIELD 0x11 /* Single choice selection field */
#define MENU_TYPE_MULTIPLE_SELECT_FIELD 0x12 /* Multiple choice selection field */
#define MENU_TYPE_SINGLE_SELECT_LIST 0x21 /* Single choice selection list */
#define MENU_TYPE_MULTIPLE_SELECT_LIST 0x22 /* Multiple choice selection list */
#define MENU_TYPE_SINGLE_SELECT_FIELD_PULL_DOWN 0x31 /* Single choice selection field and a pull-down list */
#define MENU_TYPE_MULTIPLE_SELECT_FIELD_PULL_DOWN 0x32 /* Multiple choice selection field and a pull-down list */
#define MENU_TYPE_PUSH_BUTTONS 0x41 /* Push buttons */
#define MENU_TYPE_PUSH_BUTTONS_PULL_DOWN 0x51 /* Push buttons in a pull-down menu */


  struct _Tn5250Menu;
  struct _Tn5250DBuffer;

/***** lib5250/Tn5250Menu
 * NAME
 *    Tn5250Menu
 * SYNOPSIS
 *    Tn5250Menu *menu = tn5250_menu_new ();
 * DESCRIPTION
 *    The Tn5250Menu object manages a 5250 menu on the display.
 * SOURCE
 */
  struct _Tn5250Menu
  {
    struct _Tn5250Menu *next;
    struct _Tn5250Menu *prev;
    unsigned int id;		/* Numeric ID of this menu */
    unsigned char mdt;
    short use_scrollbar;
    short num_sep_blank;
    short asterisk;
    short inputonly;
    short fieldadvischaradv;
    short nocursormove;
    unsigned char type;
    unsigned int row;		/* Row menu starts on */
    unsigned int column;	/* Column menu starts on */
    unsigned int size;		/* max size (in characters) of menu item */
    unsigned int height;	/* height (in rows) of menu */
    unsigned int items;		/* number of items on this menu */
    struct _Tn5250DBuffer *table;
  };

  typedef struct _Tn5250Menu Tn5250Menu;

/* Manipulate menus */
  extern Tn5250Menu *tn5250_menu_new ();
  extern Tn5250Menu *tn5250_menu_copy (Tn5250Menu * This);
  extern void tn5250_menu_destroy (Tn5250Menu * This);
  extern int tn5250_menu_start_row (Tn5250Menu * This);
  extern int tn5250_menu_start_col (Tn5250Menu * This);
  extern int tn5250_menu_size (Tn5250Menu * This);
  extern int tn5250_menu_height (Tn5250Menu * This);
  extern int tn5250_menu_items (Tn5250Menu * This);

/* Manipulate menu lists */
  extern Tn5250Menu *tn5250_menu_list_destroy (Tn5250Menu * list);
  extern Tn5250Menu *tn5250_menu_list_add (Tn5250Menu * list,
					   Tn5250Menu * node);
  extern Tn5250Menu *tn5250_menu_list_remove (Tn5250Menu * list,
					      Tn5250Menu * node);
  extern Tn5250Menu *tn5250_menu_list_find_by_id (Tn5250Menu * list, int id);
  extern Tn5250Menu *tn5250_menu_list_copy (Tn5250Menu * list);
  extern Tn5250Menu *tn5250_menu_hit_test (Tn5250Menu * list, int x, int y);


  struct _Tn5250Menuitem;
  struct _Tn5250DBuffer;

/***** lib5250/Tn5250Menuitem
 * NAME
 *    Tn5250Menuitem
 * SYNOPSIS
 *    Tn5250Menuitem *menuitem = tn5250_menuitem_new ();
 * DESCRIPTION
 *    The Tn5250Menuitem object manages a 5250 menuitem on the display.
 * SOURCE
 */
  struct _Tn5250Menuitem
  {
    struct _Tn5250Menuitem *next;
    struct _Tn5250Menuitem *prev;
    unsigned int id;		/* Numeric ID of this menuitem */
    unsigned int row;		/* Row menuitem starts on */
    unsigned int column;	/* Column menuitem starts on */
    unsigned int height;	/* height (in characters) of menuitem */
    unsigned int width;		/* width (in characters) of menuitem */
    unsigned int border[4];	/* Characters used to create borders
				 * Uses the same masks as buf5250 */
    struct _Tn5250DBuffer *table;
  };

  typedef struct _Tn5250Menuitem Tn5250Menuitem;

/* Manipulate menuitems */
  extern Tn5250Menuitem *tn5250_menuitem_new ();
  extern Tn5250Menuitem *tn5250_menuitem_copy (Tn5250Menuitem * This);
  extern void tn5250_menuitem_destroy (Tn5250Menuitem * This);
  extern int tn5250_menuitem_start_row (Tn5250Menuitem * This);
  extern int tn5250_menuitem_start_col (Tn5250Menuitem * This);
  extern int tn5250_menuitem_height (Tn5250Menuitem * This);
  extern int tn5250_menuitem_width (Tn5250Menuitem * This);

/* Manipulate menuitem lists */
  extern Tn5250Menuitem *tn5250_menuitem_list_destroy (Tn5250Menuitem * list);
  extern Tn5250Menuitem *tn5250_menuitem_list_add (Tn5250Menuitem * list,
						   Tn5250Menuitem * node);
  extern Tn5250Menuitem *tn5250_menuitem_list_remove (Tn5250Menuitem * list,
						      Tn5250Menuitem * node);
  extern Tn5250Menuitem *tn5250_menuitem_list_find_by_id (Tn5250Menuitem *
							  list, int id);
  extern Tn5250Menuitem *tn5250_menuitem_list_copy (Tn5250Menuitem * list);
  extern Tn5250Menuitem *tn5250_menuitem_hit_test (Tn5250Menuitem * list,
						   int x, int y);

#ifdef __cplusplus
}

#endif
#endif				/* MENU_H */
