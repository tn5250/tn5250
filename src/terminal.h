/* TN5250 - An implementation of the 5250 telnet protocol.
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
#ifndef TERMINAL_H
#define TERMINAL_H

#ifdef __cplusplus
extern "C" {
#endif

/* Flags */
#define TN5250_TERMINAL_HAS_COLOR	0x0001

/* Events */
#define TN5250_TERMINAL_EVENT_KEY	0x0001
#define TN5250_TERMINAL_EVENT_DATA	0x0002
#define TN5250_TERMINAL_EVENT_QUIT	0x0004

/* Key definitions
 * These are directly copied from <curses.h>, although the only real
 * requirement is that they be unique.  CursesTerminal is currently
 * broken and relies on them being the same, though. */

#define K_CTRL(k) ((k)-0x40)
/* e.g. K_CTRL('X') is curses/ascii key value for Ctrl+X */

#define K_FIRST_SPECIAL	0400	/* Coincides with first Curses key. */

#define K_ENTER		0x0d
#define K_NEWLINE	0x0a
#define K_TAB		0x09
#define K_BACKTAB	0541
#define K_F1		(0410+1)
#define K_F2		(0410+2)
#define K_F3		(0410+3)
#define K_F4		(0410+4)
#define K_F5		(0410+5)
#define K_F6		(0410+6)
#define K_F7		(0410+7)
#define K_F8		(0410+8)
#define K_F9		(0410+9)
#define K_F10		(0410+10)
#define K_F11		(0410+11)
#define K_F12		(0410+12)
#define K_F13		(0410+13)
#define K_F14		(0410+14)
#define K_F15		(0410+15)
#define K_F16		(0410+16)
#define K_F17		(0410+17)
#define K_F18		(0410+18)
#define K_F19		(0410+19)
#define K_F20		(0410+20)
#define K_F21		(0410+21)
#define K_F22		(0410+22)
#define K_F23		(0410+23)
#define K_F24		(0410+24)
#define K_LEFT		0404
#define K_RIGHT		0405
#define K_UP		0403
#define K_DOWN		0402
#define K_ROLLDN	0523
#define K_ROLLUP	0522
#define K_BACKSPACE	0407
#define K_HOME		0406
#define K_END		0550
#define K_INSERT	0513
#define K_DELETE	0512
#define K_RESET		0531
#define K_PRINT		0532
#define K_HELP		0553
#define K_SYSREQ	0401	/* curses KEY_BREAK */
#define K_CLEAR		0515	/* curses KEY_CLEAR */
#define K_REFRESH	0564	/* curses KEY_REFRESH */
#define K_FIELDEXIT	0517	/* curses KEY_EOL (clear to EOL) */
#define K_TESTREQ	0516	/* curses KEY_EOS (as good as any) */
#define K_TOGGLE	0533	/* curses KEY_LL (as good as any) */
#define K_ERASE		0514	/* curses KEY_EIC (as good as any) */
#define K_ATTENTION	0511	/* curses KEY_IL (as good as any) */
#define K_DUPLICATE	0524	/* curses KEY_STAB (set tab - good as any) */
#define K_FIELDMINUS	0526	/* curses KEY_CATAB (clear all tabs - g.a.a.) */
#define K_FIELDPLUS     0520    /* curses KEY_SF */
#define K_UNKNOW	0xffff

struct _Tn5250Display;

/****s* lib5250/Tn5250Terminal
 * NAME
 *    Tn5250Terminal
 * SYNOPSIS
 *    Tn5250Terminal *term = get_my_terminal_object ();
 *    int w,h,r,k,f;
 *    tn5250_terminal_init(term);
 *
 *    w = tn5250_terminal_width(term);
 *    h = tn5250_terminal_height(term);
 *    f = tn5250_terminal_flags(term);
 *
 *    r = tn5250_terminal_waitevent(term);
 *    if ((r & TN5250_TERMINAL_EVENT_KEY) != 0)
 *	 k = tn5250_terminal_getkey(term);
 *    if ((r & TN5250_TERMINAL_EVENT_QUIT) != 0)
 *	 exit(0);
 *    if ((r & TN5250_TERMINAL_EVENT_DATA) != 0)
 *	 read_data_from_fd ();
 *
 *    tn5250_terminal_update(term,display);
 *    tn5250_terminal_update_indicators(term,display);
 *    tn5250_terminal_beep(term);
 *
 *    tn5250_terminal_term(term);
 *    tn5250_terminal_destroy(term);
 * DESCRIPTION
 *    Manages a terminal interface, such as the Curses terminal interface,
 *    the S/Lang terminal interface, or an X Windows based terminal.  This
 *    is an abstract type, implementations are currently available in
 *    cursesterm.c, slangterm.c, and one should shortly be available in
 *    gtkterm.c.
 * SOURCE
 */
struct _Tn5250Terminal {
   SOCKET_TYPE conn_fd;
   struct _Tn5250TerminalPrivate *data;

   void (*init) (struct _Tn5250Terminal * This);
   void (*term) (struct _Tn5250Terminal * This);
   void (*destroy) (struct _Tn5250Terminal /*@only@*/ * This);
   int (*width) (struct _Tn5250Terminal * This);
   int (*height) (struct _Tn5250Terminal * This);
   int (*flags) (struct _Tn5250Terminal * This);
   void (*update) 			(struct _Tn5250Terminal * This,
				      struct _Tn5250Display *display);
   void (*update_indicators) 	(struct _Tn5250Terminal * This,
				      struct _Tn5250Display *display);
   int (*waitevent) (struct _Tn5250Terminal * This);
   int (*getkey) (struct _Tn5250Terminal * This);
   void (* beep) (struct _Tn5250Terminal * This);
   int (*config) (struct _Tn5250Terminal * This,
	          struct _Tn5250Config *config);
};

typedef struct _Tn5250Terminal Tn5250Terminal;
/*******/

#ifndef _TN5250_TERMINAL_PRIVATE_DEFINED
#define _TN5250_TERMINAL_PRIVATE_DEFINED
   struct _Tn5250TerminalPrivate {
      long dummy;
   };
#endif


/* Useful macros call ``methods'' on the terminal type. */
#define tn5250_terminal_init(This) (* ((This)->init)) ((This))
#define tn5250_terminal_term(This) (* ((This)->term)) ((This))
#define tn5250_terminal_destroy(This) (* ((This)->destroy)) ((This))
#define tn5250_terminal_width(This) (* ((This)->width)) ((This))
#define tn5250_terminal_height(This) (* ((This)->height)) ((This))
#define tn5250_terminal_flags(This) (* ((This)->flags)) ((This))
#define tn5250_terminal_update(This,b) (* ((This)->update)) ((This),(b))
#define tn5250_terminal_update_indicators(This,b) \
	(* ((This)->update_indicators)) ((This),(b))
#define tn5250_terminal_waitevent(This) \
	   (* ((This)->waitevent)) ((This))
#define tn5250_terminal_getkey(This) (* ((This)->getkey)) ((This))
#define tn5250_terminal_beep(This) (* ((This)->beep)) ((This))

#define tn5250_terminal_config(This,conf) \
   ((This)->config == NULL ? 0 : (* ((This)->config)) ((This),(conf)))

#ifdef __cplusplus
}

#endif
#endif				/* TERMINAL_H */

/* vi:set cindent sts=3 sw=3: */
