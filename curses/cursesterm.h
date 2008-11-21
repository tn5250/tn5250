/* TN5250
 * Copyright (C) 1997-2008 Michael Madore
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
#ifndef CURSESTERM_H
#define CURSESTERM_H

#ifdef __cplusplus
extern "C" {
#endif

#if USE_CURSES
#ifdef HAVE_NCURSES_H
#include <ncurses.h>
#else
#include <curses.h>
#endif
#ifdef HAVE_TERMCAP_H
#include <termcap.h>
#endif
#include "cursesterm.h"
#endif

#ifdef USE_CURSES
   extern Tn5250Terminal /*@null@*/ /*@only@*/ *tn5250_curses_terminal_new(void);
   extern void tn5250_curses_terminal_use_underscores (Tn5250Terminal *This,
						       int use_underscores);
   extern void tn5250_curses_terminal_set_xterm_font (Tn5250Terminal *This,
                                                       const char *font80,
                                                       const char *font132);
   extern void tn5250_curses_terminal_display_ruler (Tn5250Terminal *This,
                                                       int display_ruler);
   extern void tn5250_curses_terminal_load_colorlist (Tn5250Config *config);
#endif

#ifdef __cplusplus
}
#endif

#endif				/* CURSESTERM_H */
