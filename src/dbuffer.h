/* TN5250
 * Copyright (C) 1997 Michael Madore
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this software; see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 59 Temple Place, Suite 330,
 * Boston, MA 02111-1307 USA
 * 
 * As a special exception, the Free Software Foundation gives permission
 * for additional uses of the text contained in its release of TN5250.
 * 
 * The exception is that, if you link the TN5250 library with other files
 * to produce an executable, this does not by itself cause the
 * resulting executable to be covered by the GNU General Public License.
 * Your use of that executable is in no way restricted on account of
 * linking the TN5250 library code into it.
 * 
 * This exception does not however invalidate any other reasons why
 * the executable file might be covered by the GNU General Public License.
 * 
 * If you write modifications of your own for TN5250, it is your choice
 * whether to permit this exception to apply to your modifications.
 * If you do not wish that, delete this exception notice. */
#ifndef DBUFFER_H
#define DBUFFER_H

#ifdef __cplusplus
extern "C" {
#endif

/****s* lib5250/Tn5250DBuffer
 * NAME
 *    Tn5250DBuffer
 * SYNOPSIS
 *    Should only be accessed via the Tn5250Display object.
 * DESCRIPTION
 *    The display buffer keeps track of the current state of the display,
 *    including the field list, the format table header, the current
 *    cursor position, the home position of the cursor, and the master
 *    Modified Data Tag (MDT).
 * SOURCE
 */
struct _Tn5250DBuffer {

   /* How we keep track of multiple saved display buffers */
   struct _Tn5250DBuffer *	next;
   struct _Tn5250DBuffer *	prev;

   int w, h;
   int cx, cy;		/* Cursor Position */
   int tcx, tcy;		/* for set_new_ic */
   unsigned char /*@notnull@*/ *	  data;

   /* Stuff from the old Tn5250Table structure. */
   struct _Tn5250Field /*@null@*/ *	  field_list;
   int				  field_count;
   int				  master_mdt;

   /* Header data (from SOH order) is saved here.  We even save data that
    * we don't understand here so we can insert that into our generated
    * WTD orders for save/restore screen. */
   unsigned char *			  header_data;
   int				  header_length;
};

typedef struct _Tn5250DBuffer Tn5250DBuffer;
/*******/

/* Displays */
   extern Tn5250DBuffer /*@only@*/ /*@null@*/ *tn5250_dbuffer_new(int width, int height);
   extern Tn5250DBuffer /*@only@*/ /*@null@*/ *tn5250_dbuffer_copy(Tn5250DBuffer *);
   extern void tn5250_dbuffer_destroy(Tn5250DBuffer /*@only@*/ * This);

   extern void tn5250_dbuffer_set_size(Tn5250DBuffer * This, int rows, int cols);
   extern void tn5250_dbuffer_cursor_set(Tn5250DBuffer * This, int y, int x);
   extern void tn5250_dbuffer_clear(Tn5250DBuffer * This) /*@modifies This@*/;
   extern void tn5250_dbuffer_right(Tn5250DBuffer * This, int n);
   extern void tn5250_dbuffer_left(Tn5250DBuffer * This);
   extern void tn5250_dbuffer_up(Tn5250DBuffer * This);
   extern void tn5250_dbuffer_down(Tn5250DBuffer * This);
   extern void tn5250_dbuffer_goto_ic(Tn5250DBuffer * This);

   extern void tn5250_dbuffer_addch(Tn5250DBuffer * This, unsigned char c);
   extern void tn5250_dbuffer_del(Tn5250DBuffer * This, int shiftcount);
   extern void tn5250_dbuffer_ins(Tn5250DBuffer * This, unsigned char c, int shiftcount);
   extern void tn5250_dbuffer_set_ic(Tn5250DBuffer * This, int y, int x);
   extern void tn5250_dbuffer_roll(Tn5250DBuffer * This, int top, int bot, int lines);

   extern unsigned char tn5250_dbuffer_char_at(Tn5250DBuffer * This, int y, int x);

#define tn5250_dbuffer_width(This) ((This)->w)
#define tn5250_dbuffer_height(This) ((This)->h)
#define tn5250_dbuffer_cursor_x(This) ((This)->cx)
#define tn5250_dbuffer_cursor_y(This) ((This)->cy)

   /* Format table manipulation. */
   extern void tn5250_dbuffer_add_field(Tn5250DBuffer *This, struct _Tn5250Field *field);
   extern void tn5250_dbuffer_clear_table(Tn5250DBuffer *This);
   extern struct _Tn5250Field *tn5250_dbuffer_field_yx(Tn5250DBuffer *This, int y, int x);
   extern void tn5250_dbuffer_set_header_data(Tn5250DBuffer *This, unsigned char *data, int len);
   extern int tn5250_dbuffer_send_data_for_aid_key(Tn5250DBuffer *This, int k);
   extern unsigned char *tn5250_dbuffer_field_data(Tn5250DBuffer *This, struct _Tn5250Field *field);
   extern int tn5250_dbuffer_msg_line(Tn5250DBuffer *This);
   extern struct _Tn5250Field *tn5250_dbuffer_first_non_bypass(Tn5250DBuffer *This);

#define tn5250_dbuffer_field_count(This) ((This)->field_count)
#define tn5250_dbuffer_mdt(This) ((This)->master_mdt)
#define tn5250_dbuffer_set_mdt(This) ((This)->master_mdt = 1)

#ifdef __cplusplus
}

#endif
#endif				/* DBUFFER_H */

/* vi:set sts=3 sw=3: */
