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

#include "config.h"

#ifdef USE_CURSES

#define _TN5250_TERMINAL_PRIVATE_DEFINED

#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <curses.h>
#include <signal.h>
#include <string.h>
#include <ctype.h>
#include <malloc.h>

#include "utility.h"
#include "display.h"
#include "buffer.h"
#include "record.h"
#include "stream.h"
#include "terminal.h"
#include "cursesterm.h"

/* Some versions of ncurses don't have this defined. */
#ifndef A_VERTICAL
#define A_VERTICAL ((1UL) << ((22) + 8))
#endif				/* A_VERTICAL */

/* Mapping of 5250 colors to curses colors */
#define A_5250_GREEN    ((attr_t)COLOR_PAIR(COLOR_GREEN))
#define A_5250_WHITE    ((attr_t)COLOR_PAIR(COLOR_WHITE) | A_BOLD)
#define A_5250_RED      ((attr_t)COLOR_PAIR(COLOR_RED))
#define A_5250_TURQ     ((attr_t)COLOR_PAIR(COLOR_CYAN))
#define A_5250_YELLOW   ((attr_t)COLOR_PAIR(COLOR_YELLOW) | A_BOLD)
#define A_5250_PINK     ((attr_t)COLOR_PAIR(COLOR_MAGENTA))
#define A_5250_BLUE     ((attr_t)COLOR_PAIR(COLOR_CYAN) | A_BOLD)

/*@-globstate -nullpass@*/  /* lclint incorrectly assumes stdscr may be NULL */

static attr_t attribute_map[] =
{A_5250_GREEN,
 A_5250_GREEN | A_REVERSE,
 A_5250_WHITE,
 A_5250_WHITE | A_REVERSE,
 A_5250_GREEN | A_UNDERLINE,
 A_5250_GREEN | A_UNDERLINE | A_REVERSE,
 A_5250_WHITE | A_UNDERLINE,
 0x00,
 A_5250_RED,
 A_5250_RED | A_REVERSE,
 A_5250_RED | A_BLINK,
 A_5250_RED | A_BLINK | A_REVERSE,
 A_5250_RED | A_UNDERLINE,
 A_5250_RED | A_UNDERLINE | A_REVERSE,
 A_5250_RED | A_UNDERLINE | A_BLINK,
 0x00,
 A_5250_TURQ | A_VERTICAL,
 A_5250_TURQ | A_VERTICAL | A_REVERSE,
 A_5250_YELLOW | A_VERTICAL,
 A_5250_YELLOW | A_VERTICAL | A_REVERSE,
 A_5250_TURQ | A_UNDERLINE | A_VERTICAL,
 A_5250_TURQ | A_UNDERLINE | A_REVERSE | A_VERTICAL,
 A_5250_YELLOW | A_UNDERLINE | A_VERTICAL,
 0x00,
 A_5250_PINK,
 A_5250_PINK | A_REVERSE,
 A_5250_BLUE,
 A_5250_BLUE | A_REVERSE,
 A_5250_PINK | A_UNDERLINE,
 A_5250_PINK | A_UNDERLINE | A_REVERSE,
 A_5250_BLUE | A_UNDERLINE,
 0x00};

static void curses_terminal_init(Tn5250Terminal * This) /*@modifies This@*/;
static void curses_terminal_term(Tn5250Terminal * This) /*@modifies This@*/;
static void curses_terminal_destroy(Tn5250Terminal /*@only@*/ * This);
static int curses_terminal_width(Tn5250Terminal * This);
static int curses_terminal_height(Tn5250Terminal * This);
static int curses_terminal_flags(Tn5250Terminal * This);
static void curses_terminal_update(Tn5250Terminal * This,
				   Tn5250Display * dsp) /*@modifies This@*/;
static void curses_terminal_update_indicators(Tn5250Terminal * This,
					      Tn5250Display * dsp) /*@modifies This@*/;
static int curses_terminal_waitevent(Tn5250Terminal * This) /*@modifies This@*/;
static int curses_terminal_getkey(Tn5250Terminal * This) /*@modifies This@*/;
static int curses_terminal_get_esc_key(Tn5250Terminal * This, int is_esc) /*@modifies This@*/;

struct _Tn5250TerminalPrivate {
   int quit_flag;
   int underscores;
   int last_width, last_height;
   int is_xterm;
};

/*
 *    Create a new curses terminal object.
 */
Tn5250Terminal *tn5250_curses_terminal_new()
{
   Tn5250Terminal *r = tn5250_new(Tn5250Terminal, 1);
   if (r == NULL)
      return NULL;

   r->data = tn5250_new(struct _Tn5250TerminalPrivate, 1);
   if (r->data == NULL) {
      free(r);
      return NULL;
   }

   r->data->underscores = 0;
   r->data->quit_flag = 0;
   r->data->last_width = 0;
   r->data->last_height = 0;
   r->data->is_xterm = 0;

   r->conn_fd = -1;
   r->init = curses_terminal_init;
   r->term = curses_terminal_term;
   r->destroy = curses_terminal_destroy;
   r->width = curses_terminal_width;
   r->height = curses_terminal_height;
   r->flags = curses_terminal_flags;
   r->update = curses_terminal_update;
   r->update_indicators = curses_terminal_update_indicators;
   r->waitevent = curses_terminal_waitevent;
   r->getkey = curses_terminal_getkey;

   return r;
}

static void curses_terminal_init(Tn5250Terminal * This)
{
   char buf[6];
   int i = 0, c;
   struct timeval tv;

   (void)initscr();
   raw();
   keypad(stdscr, 1);
   nodelay(stdscr, 1);
   noecho();

   /* Determine if we're talking to an xterm ;) */
   printf ("\x5");
   fflush (stdout);

   tv.tv_usec = 100;
   tv.tv_sec = 0;
   select (0, NULL, NULL, NULL, &tv);

   while ((i < 5) && (c = getch ()) != -1) {
      buf[i++] = (char)c;
   }
   buf[i] = '\0';
   if (c != -1) {
      while (getch () != -1)
	 ;
   }
   This->data->is_xterm = !strcmp (buf, "xterm");

   /* Initialize colors if the terminal supports it. */
   if (has_colors()) {
      start_color();
      init_pair(COLOR_BLACK, COLOR_BLACK, COLOR_BLACK);
      init_pair(COLOR_GREEN, COLOR_GREEN, COLOR_BLACK);
      init_pair(COLOR_RED, COLOR_RED, COLOR_BLACK);
      init_pair(COLOR_CYAN, COLOR_CYAN, COLOR_BLACK);
      init_pair(COLOR_WHITE, COLOR_WHITE, COLOR_BLACK);
      init_pair(COLOR_MAGENTA, COLOR_MAGENTA, COLOR_BLACK);
      init_pair(COLOR_BLUE, COLOR_BLUE, COLOR_BLACK);
      init_pair(COLOR_YELLOW, COLOR_YELLOW, COLOR_BLACK);
   }
   This->data->quit_flag = 0;
}

static void curses_terminal_term(Tn5250Terminal /*@unused@*/ * This)
{
   endwin();
}

static void curses_terminal_destroy(Tn5250Terminal * This)
{
   if (This->data != NULL)
      free(This->data);
   free(This);
}

static int curses_terminal_width(Tn5250Terminal /*@unused@*/ * This)
{
   int y, x;
   getmaxyx(stdscr, y, x);
   return x + 1;
}

static int curses_terminal_height(Tn5250Terminal /*@unused@*/ * This)
{
   int y, x;
   getmaxyx(stdscr, y, x);
   return y + 1;
}

static int curses_terminal_flags(Tn5250Terminal /*@unused@*/ * This)
{
   int f = 0;
   if (has_colors() != 0)
      f |= TN5250_TERMINAL_HAS_COLOR;
   return f;
}

static void curses_terminal_update(Tn5250Terminal * This, Tn5250Display * dsp)
{
   int my, mx;
   int y, x;
   attr_t curs_attr;
   unsigned char a = 0x20, c;

   if (This->data->last_width != tn5250_display_width(dsp)
       || This->data->last_height != tn5250_display_height(dsp)) {
      clear();
        if(1) {
/*      if (This->data->is_xterm) {   */
	 refresh ();
	 printf ("\x1b[8;%d;%dt", tn5250_display_height (dsp)+1,
	       tn5250_display_width (dsp));
	 fflush (stdout);
	 resizeterm(tn5250_display_height(dsp)+1, tn5250_display_width(dsp)+1);

	 /* Make sure we get a SIGWINCH - We need curses to resize its
	  * buffer. */
	 raise (SIGWINCH);
      }
      This->data->last_width = tn5250_display_width(dsp);
      This->data->last_height = tn5250_display_height(dsp);
      curses_terminal_update_indicators(This, dsp);
   }
   attrset(A_NORMAL);
   getmaxyx(stdscr, my, mx);
   for (y = 0; y < tn5250_display_height(dsp); y++) {
      if (y > my)
	 break;

      move(y, 0);
      for (x = 0; x < tn5250_display_width(dsp); x++) {
	 c = tn5250_display_char_at(dsp, y, x);
	 if ((c & 0xe0) == 0x20) {	/* ATTRIBUTE */
	    a = (c & 0xff);
	    addch(attribute_map[0] | ' ');
	 } else {		/* DATA */
	    curs_attr = attribute_map[a - 0x20];
	    if (curs_attr == 0x00) {	/* NONDISPLAY */
	       addch(attribute_map[0] | ' ');
	    } else {
	       if ((c < 0x40 && c > 0x00) || c == 0xff) { /* UNPRINTABLE */
		  c = ' ';
		  curs_attr ^= A_REVERSE;
	       } else {
		  c = tn5250_ebcdic2ascii(c);
	       }
	       if ((curs_attr & A_VERTICAL) != 0) {
		  curs_attr |= A_UNDERLINE;
		  curs_attr &= ~A_VERTICAL;
	       }
	       /* This is a kludge since vga hardware doesn't support under-
	        * lining characters.  It's pretty ugly. */
	       if (This->data->underscores) {
		  if ((curs_attr & A_UNDERLINE) != 0) {
		     curs_attr &= ~A_UNDERLINE;
		     if (c == ' ')
			c = '_';
		  }
	       }
	       addch((chtype)(c | curs_attr));
	    }
	 }			/* if ((c & 0xe0) ... */
      }				/* for (int x ... */
   }				/* for (int y ... */

   move(tn5250_display_cursor_y(dsp), tn5250_display_cursor_x(dsp));
   refresh();
}

static void curses_terminal_update_indicators(Tn5250Terminal /*@unused@*/ * This, Tn5250Display * dsp)
{
   int inds = tn5250_display_indicators(dsp);
   char ind_buf[80];

   memset(ind_buf, ' ', sizeof(ind_buf));
   memcpy(ind_buf, "5250", 4);
   if ((inds & TN5250_DISPLAY_IND_MESSAGE_WAITING) != 0) {
      memcpy(ind_buf + 23, "MW", 2);
   }
   if ((inds & TN5250_DISPLAY_IND_INHIBIT) != 0) {
      memcpy(ind_buf + 9, "X II", 4);
   } else if ((inds & TN5250_DISPLAY_IND_X_CLOCK) != 0) {
      memcpy(ind_buf + 9, "X CLOCK", 7);
   } else if ((inds & TN5250_DISPLAY_IND_X_SYSTEM) != 0) {
      memcpy(ind_buf + 9, "X SYSTEM", 8);
   }
   if ((inds & TN5250_DISPLAY_IND_INSERT) != 0) {
      memcpy(ind_buf + 30, "IM", 2);
   }
   attrset( (attr_t)COLOR_PAIR(COLOR_WHITE) );
   mvaddnstr(tn5250_display_height(dsp), 0, ind_buf, 80);
   move(tn5250_display_cursor_y(dsp), tn5250_display_cursor_x(dsp));
   attrset(A_NORMAL);
   refresh();
}

static int curses_terminal_waitevent(Tn5250Terminal * This)
{
   fd_set fdr;
   int result = 0;
   int sm;

   if (This->data->quit_flag)
      return TN5250_TERMINAL_EVENT_QUIT;

   FD_ZERO(&fdr);

   FD_SET(0, &fdr);
   sm = 1;
   if (This->conn_fd >= 0) {
      FD_SET(This->conn_fd, &fdr);
      sm = This->conn_fd + 1;
   }

   select(sm, &fdr, NULL, NULL, NULL);

   if (FD_ISSET(0, &fdr))
      result |= TN5250_TERMINAL_EVENT_KEY;

   if (This->conn_fd >= 0 && FD_ISSET(This->conn_fd, &fdr))
      result |= TN5250_TERMINAL_EVENT_DATA;

   return result;
}

static int curses_terminal_getkey(Tn5250Terminal * This)
{
   int key = getch();

   while (1) {
      switch (key) {

      case 0x0d:
      case 0x0a:
	 return K_ENTER;

      case 0x1b:
	 if ((key = curses_terminal_get_esc_key(This, 1)) != ERR)
	    return key;
	 break;

      case K_CTRL('A'):
	 return K_ATTENTION;
      case K_CTRL('B'):
	 return K_ROLLDN;
      case K_CTRL('C'):
	 return K_SYSREQ;
      case K_CTRL('D'):
	 return K_ROLLUP;
      case K_CTRL('E'):
	 return K_ERASE;
      case K_CTRL('F'):
	 return K_ROLLUP;
      case K_CTRL('K'):
	 return K_FIELDEXIT;
      case K_CTRL('L'):
	 return K_REFRESH;
      case K_CTRL('O'):
	 return K_HOME;
      case K_CTRL('P'):
	 return K_PRINT;
      case K_CTRL('R'):
	 return K_RESET;	/* Error Reset */
      case K_CTRL('T'):
	 return K_TESTREQ;
      case K_CTRL('U'):
	 return K_ROLLDN;
      case K_CTRL('X'):
	 return K_FIELDEXIT;

      case K_CTRL('Q'):
	 This->data->quit_flag = 1;
	 return ERR;

      case K_CTRL('G'):	/* C-g <function-key-shortcut> */
	 if ((key = curses_terminal_get_esc_key(This, 0)) != ERR)
	    return key;
	 break;

      case ERR:
	 return -1;

      case 127:
	 return K_DELETE;

      case KEY_A1:
	 return K_HOME;

      case KEY_A3:
	 return K_ROLLDN;

      case KEY_C1:
	 return KEY_END;

      case KEY_C3:
	 return K_ROLLUP;

      case KEY_ENTER:
	 return K_FIELDEXIT;

      default:
	 return key;
      }
   }
}

/*
 *    If a vt100 escape key sequence was introduced (using either
 *      <Esc> or <Ctrl+g>), handle the next key in the sequence.
 */
static int curses_terminal_get_esc_key(Tn5250Terminal * This,
				       int is_esc)
{
   int y, x, key, display_key;
   fd_set fdr;

   getyx(stdscr, y, x);
   attrset(COLOR_PAIR(COLOR_WHITE));
   if (is_esc)
      mvaddstr(24, 60, "Esc ");
   else
      mvaddstr(24, 60, "C-g ");
   move(y, x);
   refresh();

   FD_ZERO(&fdr);
   FD_SET(0, &fdr);
   select(1, &fdr, NULL, NULL, NULL);
   key = getch();

   if (isalpha(key))
      key = toupper(key);

   display_key = key;
   switch (key) {

      /* Function keys */
   case '1':
      key = K_F1;
      break;
   case '2':
      key = K_F2;
      break;
   case '3':
      key = K_F3;
      break;
   case '4':
      key = K_F4;
      break;
   case '5':
      key = K_F5;
      break;
   case '6':
      key = K_F6;
      break;
   case '7':
      key = K_F7;
      break;
   case '8':
      key = K_F8;
      break;
   case '9':
      key = K_F9;
      break;
   case '0':
      key = K_F10;
      break;
   case '-':
      key = K_F11;
      break;
   case '=':
      key = K_F12;
      break;
   case '!':
      key = K_F13;
      break;
   case '@':
      key = K_F14;
      break;
   case '#':
      key = K_F15;
      break;
   case '$':
      key = K_F16;
      break;
   case '%':
      key = K_F17;
      break;
   case '^':
      key = K_F18;
      break;
   case '&':
      key = K_F19;
      break;
   case '*':
      key = K_F20;
      break;
   case '(':
      key = K_F21;
      break;
   case ')':
      key = K_F22;
      break;
   case '_':
      key = K_F23;
      break;
   case '+':
      key = K_F24;
      break;

      /* AS/400 strangeness */
   case 'A':
      key = K_ATTENTION;
      break;
   case 'C':
      key = K_CLEAR;
      break;
   case 'D':
      key = K_DUPLICATE;
      break;
   case 'H':
      key = K_HELP;
      break;
   case 'I':
      key = K_INSERT;
      break;
   case 'L':
      key = K_REFRESH;
      break;
   case 'M':
      key = K_FIELDMINUS;
      break;
   case 'P':
      key = K_PRINT;
      break;
   case 'R':
      key = K_RESET;
      break;
   case 'S':
      key = K_SYSREQ;
      break;
   case 'T':
      key = K_TOGGLE;
      break;
   case 'X':
      key = K_FIELDEXIT;
      break;

   case 127:
      key = K_INSERT;
      break;			/* ESC DEL */
   case KEY_DC:
      key = K_INSERT;
      break;			/* ESC DEL, also */
   case K_CTRL('J'):
      key = K_NEWLINE;
      break;

   case 'Q':
      This->data->quit_flag = 1;
      key = ERR;
      break;

   default:
      beep();
      key = ERR;
      break;
   }

   if (key == ERR)
      mvaddstr(24, 64, "???");
   else
      mvaddch(24, 64, (chtype)display_key);
   move(y, x);
   refresh();
   return key;
}

/*
 *    Called by tn5250.c to determine whether we should use underscores or
 *      the terminal's underline attribute.
 */
int tn5250_curses_terminal_use_underscores(Tn5250Terminal * This, int v)
{
   int oldval = This->data->underscores;
   This->data->underscores = v;
   return oldval;
}

#endif /* USE_CURSES */

/* vi:set cindent sts=3 sw=3: */
