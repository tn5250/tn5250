/* tn5250 -- an implentation of the 5250 telnet protocol.
 * Copyright (C) 1997 Michael Madore
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
#ifndef DBUFFER_H
#define DBUFFER_H

#ifdef __cplusplus
extern "C" {
#endif

#define TN5250_DISPLAY_IND_INHIBIT	   	0x0001
#define TN5250_DISPLAY_IND_MESSAGE_WAITING	0x0002
#define TN5250_DISPLAY_IND_X_SYSTEM	   	0x0004
#define TN5250_DISPLAY_IND_X_CLOCK	   	0x0008
#define TN5250_DISPLAY_IND_INSERT	   	0x0010

   struct _Tn5250DBuffer {
 
      /* How we keep track of multiple saved display buffers */
      struct _Tn5250DBuffer *	next;
      struct _Tn5250DBuffer *	prev;
      unsigned char		id;	/* Saved buffer id */

      int w, h;
      int cx, cy;		/* Cursor Position */
      int tcx, tcy;		/* for set_new_ic */
      int disp_indicators;
      unsigned char /*@notnull@*/ *data;
   };

   typedef struct _Tn5250DBuffer Tn5250DBuffer;

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
   extern void tn5250_dbuffer_mvaddnstr(Tn5250DBuffer * This, int y, int x,
					const unsigned char *str, int n);
   extern void tn5250_dbuffer_del(Tn5250DBuffer * This, int shiftcount);
   extern void tn5250_dbuffer_ins(Tn5250DBuffer * This, unsigned char c, int shiftcount);
   extern void tn5250_dbuffer_set_temp_ic(Tn5250DBuffer * This, int y, int x);
   extern void tn5250_dbuffer_roll(Tn5250DBuffer * This, int top, int bot, int lines);

   extern void tn5250_dbuffer_indicator_set(Tn5250DBuffer * This, int inds);
   extern void tn5250_dbuffer_indicator_clear(Tn5250DBuffer * This, int inds);
   extern unsigned char tn5250_dbuffer_char_at(Tn5250DBuffer * This, int y, int x);

#define tn5250_dbuffer_width(This) ((This)->w)
#define tn5250_dbuffer_height(This) ((This)->h)
#define tn5250_dbuffer_cursor_x(This) ((This)->cx)
#define tn5250_dbuffer_cursor_y(This) ((This)->cy)
#define tn5250_dbuffer_indicators(This) ((This)->disp_indicators)

   /* Useful macros for testing/setting INHIBIT flag */
#define tn5250_dbuffer_inhibited(This) ((tn5250_dbuffer_indicators (This) & TN5250_DISPLAY_IND_INHIBIT) != 0)
#define tn5250_dbuffer_inhibit(This) (tn5250_dbuffer_indicator_set (This, TN5250_DISPLAY_IND_INHIBIT))
#define tn5250_dbuffer_uninhibit(This) \
   (tn5250_dbuffer_indicator_clear (This, TN5250_DISPLAY_IND_INHIBIT))

#ifdef __cplusplus
}

#endif
#endif				/* DBUFFER_H */
