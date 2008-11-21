#ifndef WINDOW_H
#define WINDOW_H

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

#ifdef __cplusplus
extern "C"
{
#endif

  struct _Tn5250Window;
  struct _Tn5250DBuffer;

/***** lib5250/Tn5250Window
 * NAME
 *    Tn5250Window
 * SYNOPSIS
 *    Tn5250Window *window = tn5250_window_new ();
 * DESCRIPTION
 *    The Tn5250Window object manages a 5250 window on the display.
 * SOURCE
 */
  struct _Tn5250Window
  {
    struct _Tn5250WindowPrivate *data;
    struct _Tn5250Window *next;
    struct _Tn5250Window *prev;
    unsigned int id;		/* Numeric ID of this window */
    unsigned int row;		/* Row window starts on */
    unsigned int column;	/* Column window starts on */
    unsigned int height;	/* height (in characters) of window */
    unsigned int width;		/* width (in characters) of window */
    unsigned int border[4];	/* Characters used to create borders
				 * Uses the same masks as buf5250 */
    struct _Tn5250DBuffer *table;
  };

  typedef struct _Tn5250Window Tn5250Window;
/*******/

/* Manipulate windows */
  extern Tn5250Window *tn5250_window_new ();
  extern Tn5250Window *tn5250_window_copy (Tn5250Window * This);
  extern void tn5250_window_destroy (Tn5250Window * This);
  extern int tn5250_window_start_row (Tn5250Window * This);
  extern int tn5250_window_start_col (Tn5250Window * This);
  extern int tn5250_window_height (Tn5250Window * This);
  extern int tn5250_window_width (Tn5250Window * This);

/* Manipulate window lists */
  extern Tn5250Window *tn5250_window_list_destroy (Tn5250Window * list);
  extern Tn5250Window *tn5250_window_list_add (Tn5250Window * list,
					       Tn5250Window * node);
  extern Tn5250Window *tn5250_window_list_remove (Tn5250Window * list,
						  Tn5250Window * node);
  extern Tn5250Window *tn5250_window_list_find_by_id (Tn5250Window * list,
						      int id);
  extern Tn5250Window *tn5250_window_list_copy (Tn5250Window * list);
  extern Tn5250Window *tn5250_window_match_test (Tn5250Window * list, int x,
						 int y, int columns,
						 int rows);
  extern Tn5250Window *tn5250_window_hit_test (Tn5250Window * list, int x,
					       int y);


#ifndef _TN5250_WINDOW_PRIVATE_DEFINED
#define _TN5250_WINDOW_PRIVATE_DEFINED
  struct _Tn5250WindowPrivate
  {
    long dummy;
  };
#endif

#ifdef __cplusplus
}

#endif
#endif				/* WINDOW_H */
