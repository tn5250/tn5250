#ifndef SCROLLBAR_H
#define SCROLLBAR_H

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

  struct _Tn5250Scrollbar;
  struct _Tn5250DBuffer;

/***** lib5250/Tn5250Scrollbar
 * NAME
 *    Tn5250Scrollbar
 * SYNOPSIS
 *    Tn5250Scrollbar *scrollbar = tn5250_scrollbar_new ();
 * DESCRIPTION
 *    The Tn5250Scrollbar object manages a 5250 scrollbar on the display.
 * SOURCE
 */
  struct _Tn5250Scrollbar
  {
    struct _Tn5250Scrollbar *next;
    struct _Tn5250Scrollbar *prev;
    unsigned int id;		/* Numeric ID of this scrollbar */
    unsigned int direction;	/* 1=horizontal, 0=vertical */
    unsigned int rowscols;	/* number of scrollable rows/columns */
    unsigned int sliderpos;	/* position of slider */
    unsigned int size;		/* size (in characters) of scrollbar */
    struct _Tn5250DBuffer *table;
  };

  typedef struct _Tn5250Scrollbar Tn5250Scrollbar;
/*******/

/* Manipulate scrollbars */
  extern Tn5250Scrollbar *tn5250_scrollbar_new ();
  extern Tn5250Scrollbar *tn5250_scrollbar_copy (Tn5250Scrollbar * This);
  extern void tn5250_scrollbar_destroy (Tn5250Scrollbar * This);
  extern int tn5250_scrollbar_direction (Tn5250Scrollbar * This);
  extern int tn5250_scrollbar_rowscols (Tn5250Scrollbar * This);
  extern int tn5250_scrollbar_sliderpos (Tn5250Scrollbar * This);
  extern int tn5250_scrollbar_size (Tn5250Scrollbar * This);

/* Manipulate scrollbar lists */
  extern Tn5250Scrollbar *tn5250_scrollbar_list_destroy (Tn5250Scrollbar *
							 list);
  extern Tn5250Scrollbar *tn5250_scrollbar_list_add (Tn5250Scrollbar * list,
						     Tn5250Scrollbar * node);
  extern Tn5250Scrollbar *tn5250_scrollbar_list_remove (Tn5250Scrollbar *
							list,
							Tn5250Scrollbar *
							node);
  extern Tn5250Scrollbar *tn5250_scrollbar_list_find_by_id (Tn5250Scrollbar *
							    list, int id);
  extern Tn5250Scrollbar *tn5250_scrollbar_list_copy (Tn5250Scrollbar * list);

#ifdef __cplusplus
}

#endif
#endif				/* SCROLLBAR_H */
