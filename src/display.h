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
#ifndef DISPLAY_H
#define DISPLAY_H

#ifdef __cplusplus
extern "C" {
#endif

#define TN5250_DISPLAY_IND_INHIBIT	   	0x0001
#define TN5250_DISPLAY_IND_MESSAGE_WAITING	0x0002
#define TN5250_DISPLAY_IND_X_SYSTEM	   	0x0004
#define TN5250_DISPLAY_IND_X_CLOCK	   	0x0008
#define TN5250_DISPLAY_IND_INSERT	   	0x0010

   struct _Tn5250Display {
      int w, h;
      int cx, cy;		/* Cursor Position */
      int tcx, tcy;		/* for set_new_ic */
      int disp_indicators;
      unsigned char /*@notnull@*/ **rows;
   };

   typedef struct _Tn5250Display Tn5250Display;

/* Displays */
   extern Tn5250Display /*@only@*/ /*@null@*/ *tn5250_display_new(int width, int height);
   extern Tn5250Display /*@only@*/ /*@null@*/ *tn5250_display_copy(Tn5250Display *);
   extern void tn5250_display_destroy(Tn5250Display /*@only@*/ * This);

   extern void tn5250_display_set_size(Tn5250Display * This, int rows, int cols);
   extern void tn5250_display_cursor_set(Tn5250Display * This, int y, int x);
   extern void tn5250_display_clear(Tn5250Display * This) /*@modifies This@*/;
   extern void tn5250_display_right(Tn5250Display * This, int n);
   extern void tn5250_display_left(Tn5250Display * This);
   extern void tn5250_display_up(Tn5250Display * This);
   extern void tn5250_display_down(Tn5250Display * This);
   extern void tn5250_display_goto_ic(Tn5250Display * This);

   extern void tn5250_display_addch(Tn5250Display * This, unsigned char c);
   extern void tn5250_display_mvaddnstr(Tn5250Display * This, int y, int x,
					const unsigned char *str, int n);
   extern void tn5250_display_del(Tn5250Display * This, int shiftcount);
   extern void tn5250_display_ins(Tn5250Display * This, unsigned char c, int shiftcount);
   extern void tn5250_display_set_temp_ic(Tn5250Display * This, int y, int x);
   extern void tn5250_display_roll(Tn5250Display * This, int top, int bot, int lines);

   extern void tn5250_display_indicator_set(Tn5250Display * This, int inds);
   extern void tn5250_display_indicator_clear(Tn5250Display * This, int inds);
   extern int tn5250_display_save(Tn5250Display * This);
   extern void tn5250_display_restore(Tn5250Display * This);

   extern unsigned char tn5250_display_char_at(Tn5250Display * This, int y, int x);

#define tn5250_display_width(This) ((This)->w)
#define tn5250_display_height(This) ((This)->h)
#define tn5250_display_cursor_x(This) ((This)->cx)
#define tn5250_display_cursor_y(This) ((This)->cy)
#define tn5250_display_indicators(This) ((This)->disp_indicators)

   /* Useful macros for testing/setting INHIBIT flag */
#define tn5250_display_inhibited(This) ((tn5250_display_indicators (This) & TN5250_DISPLAY_IND_INHIBIT) != 0)
#define tn5250_display_inhibit(This) (tn5250_display_indicator_set (This, TN5250_DISPLAY_IND_INHIBIT))
#define tn5250_display_uninhibit(This) \
   (tn5250_display_indicator_clear (This, TN5250_DISPLAY_IND_INHIBIT))

#ifdef __cplusplus
}

#endif
#endif				/* DISPLAY_H */
