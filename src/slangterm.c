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

#if USE_SLANG
#define _TN5250_TERMINAL_PRIVATE_DEFINED

#include <stdio.h>
#include <sys/time.h>
#include <signal.h>
#include <string.h>
#include <ctype.h>
#include <malloc.h>
#ifdef HAVE_SLANG_H
#include <slang.h>
#else
#ifdef HAVE_SLANG_SLANG_H
#include <slang/slang.h>
#endif
#endif
#include "utility.h"
#include "dbuffer.h"
#include "buffer.h"
#include "record.h"
#include "stream.h"
#include "terminal.h"
#include "slangterm.h"

/* Mapping of 5250 colors to curses colors */
#define A_5250_WHITE    0x100
#define A_5250_RED      0x200
#define A_5250_TURQ     0x300
#define A_5250_YELLOW   0x400
#define A_5250_PINK     0x500
#define A_5250_BLUE     0x600
#define A_5250_BLACK	0x700
#define A_5250_GREEN    0x800

#define A_COLOR_MASK	0xf00

#define A_REVERSE	0x1000
#define A_UNDERLINE	0x2000
#define A_BLINK		0x4000
#define A_VERTICAL	0x8000

static int attribute_map[] =
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

static void slang_terminal_init(Tn5250Terminal * This);
static void slang_terminal_term(Tn5250Terminal * This);
static void slang_terminal_destroy(Tn5250Terminal /*@only@*/ * This);
static int slang_terminal_width(Tn5250Terminal * This);
static int slang_terminal_height(Tn5250Terminal * This);
static int slang_terminal_flags(Tn5250Terminal * This);
static void slang_terminal_update(Tn5250Terminal * This,
				   Tn5250DBuffer * dsp);
static void slang_terminal_update_indicators(Tn5250Terminal * This,
					      Tn5250DBuffer * dsp);
static int slang_terminal_waitevent(Tn5250Terminal * This);
static int slang_terminal_getkey(Tn5250Terminal * This);
static int slang_terminal_get_esc_key(Tn5250Terminal * This, int is_esc);
static void slang_terminal_set_attrs (Tn5250Terminal * This, int attrs);

struct _Tn5250TerminalPrivate {
   int quit_flag;
   int last_width, last_height;
   int attrs;
};

/*
 *    Create a new curses terminal object.
 */
Tn5250Terminal *tn5250_slang_terminal_new()
{
   Tn5250Terminal *r = tn5250_new(Tn5250Terminal, 1);
   if (r != NULL) {
      r->data = tn5250_new(struct _Tn5250TerminalPrivate, 1);
      if (r->data == NULL) {
	 free(r);
	 return NULL;
      }
      r->data->quit_flag = 0;
      r->data->last_width = 0;
      r->data->last_height = 0;
      r->data->attrs = 0;

      r->conn_fd = -1;
      r->init = slang_terminal_init;
      r->term = slang_terminal_term;
      r->destroy = slang_terminal_destroy;
      r->width = slang_terminal_width;
      r->height = slang_terminal_height;
      r->flags = slang_terminal_flags;
      r->update = slang_terminal_update;
      r->update_indicators = slang_terminal_update_indicators;
      r->waitevent = slang_terminal_waitevent;
      r->getkey = slang_terminal_getkey;
   }
   return r;
}

static void slang_terminal_init(Tn5250Terminal * This)
{
   SLtt_get_terminfo ();
   if (-1 == SLkp_init ()) {
      SLang_doerror ("SLkp_init failed.");
      exit (255);
   }
   if (-1 == SLang_init_tty (K_CTRL('Q'), 1, 0)) {
      SLang_doerror ("SLang_init_tty failed.");
      exit (255);
   }
   SLang_set_abort_signal (NULL);
   if (-1 == SLsmg_init_smg ()) {
      SLang_doerror ("SLsmg_init_smg failed.");
      exit (255);
   }

   SLtt_set_color_fgbg (1, SLSMG_COLOR_BRIGHT_WHITE, SLSMG_COLOR_BLACK);
   SLtt_set_color_fgbg (2, SLSMG_COLOR_RED, SLSMG_COLOR_BLACK);
   SLtt_set_color_fgbg (3, SLSMG_COLOR_BLUE, SLSMG_COLOR_BLACK);
   SLtt_set_color_fgbg (4, SLSMG_COLOR_BRIGHT_BROWN, SLSMG_COLOR_BLACK);
   SLtt_set_color_fgbg (5, SLSMG_COLOR_BRIGHT_MAGENTA, SLSMG_COLOR_BLACK);
   SLtt_set_color_fgbg (6, SLSMG_COLOR_BRIGHT_BLUE, SLSMG_COLOR_BLACK);
   SLtt_set_color_fgbg (7, SLSMG_COLOR_BLACK, SLSMG_COLOR_BLACK);
   SLtt_set_color_fgbg (8, SLSMG_COLOR_GREEN, SLSMG_COLOR_BLACK);

   SLsmg_cls ();
   SLsmg_refresh ();
   This->data->quit_flag = 0;
}

static void slang_terminal_term(Tn5250Terminal * This)
{
   SLsmg_reset_smg ();
   SLang_reset_tty ();
}

static void slang_terminal_destroy(Tn5250Terminal * This)
{
   if (This->data != NULL)
      free(This->data);
   free(This);
}

static int slang_terminal_width(Tn5250Terminal /*@unused@*/ * This)
{
   SLtt_get_screen_size();
   return SLtt_Screen_Cols;
}

static int slang_terminal_height(Tn5250Terminal /*@unused@*/ * This)
{
   SLtt_get_screen_size();
   return SLtt_Screen_Rows;
}

static int slang_terminal_flags(Tn5250Terminal /*@unused@*/ * This)
{
   int f = 0;
   if (SLtt_Use_Ansi_Colors)
      f |= TN5250_TERMINAL_HAS_COLOR;
   return f;
}

static void slang_terminal_update(Tn5250Terminal * This, Tn5250DBuffer * dsp)
{
   int my, mx;
   int y, x;
   int curs_attr, temp_attr;
   unsigned char a = 0x20, c;

   if (This->data->last_width != tn5250_dbuffer_width(dsp)
       || This->data->last_height != tn5250_dbuffer_height(dsp)) {
      SLsmg_cls ();
      This->data->last_width = tn5250_dbuffer_width(dsp);
      This->data->last_height = tn5250_dbuffer_height(dsp);
      slang_terminal_update_indicators(This, dsp);
   }
   SLsmg_normal_video ();
   my = SLtt_Screen_Rows - 1;
   mx = SLtt_Screen_Cols - 1;
   for (y = 0; y < tn5250_dbuffer_height(dsp); y++) {
      if (y > my)
	 break;

      SLsmg_gotorc (y, 0);
      for (x = 0; x < tn5250_dbuffer_width(dsp); x++) {
	 c = tn5250_dbuffer_char_at(dsp, y, x);
	 if ((c & 0xe0) == 0x20) {	/* ATTRIBUTE */
	    a = (c & 0xff);
	    temp_attr = This->data->attrs;
	    slang_terminal_set_attrs (This, attribute_map[0]);
	    SLsmg_write_char (' ');
	    slang_terminal_set_attrs (This, temp_attr);
	 } else {		/* DATA */
	    curs_attr = attribute_map[a - 0x20];
	    if (curs_attr == 0x00) {	/* NONDISPLAY */
	       temp_attr = This->data->attrs;
	       slang_terminal_set_attrs (This, attribute_map[0]);
	       SLsmg_write_char (' ');
	       slang_terminal_set_attrs (This, temp_attr);
	    } else {
	       c = tn5250_ebcdic2ascii(c);
	       if ((c < 0x20 && c > 0x00) || c >= 0x7f) { /* UNPRINTABLE */
		  c = ' ';
		  curs_attr ^= A_REVERSE;
	       }
	       if ((curs_attr & A_VERTICAL) != 0) {
		  curs_attr |= A_UNDERLINE;
		  curs_attr &= ~A_VERTICAL;
	       }
	       /* This is a kludge since vga hardware doesn't support under-
	        * lining characters.  It's pretty ugly. */
	       if ((curs_attr & A_UNDERLINE) != 0) {
		  curs_attr &= ~A_UNDERLINE;
		  if (c == ' ')
		     c = '_';
	       }
	       slang_terminal_set_attrs (This, curs_attr);
	       SLsmg_write_char (c);
	    }
	 }			/* if ((c & 0xe0) ... */
      }				/* for (int x ... */
   }				/* for (int y ... */

   SLsmg_gotorc (tn5250_dbuffer_cursor_y(dsp), tn5250_dbuffer_cursor_x(dsp));
   SLsmg_refresh();
}

static void slang_terminal_update_indicators(Tn5250Terminal * This, Tn5250DBuffer * dsp)
{
   int inds = tn5250_dbuffer_indicators(dsp);
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
   SLsmg_normal_video ();
   SLsmg_gotorc (tn5250_dbuffer_height (dsp), 0);
   SLsmg_write_nchars (ind_buf, 80);
   SLsmg_gotorc (tn5250_dbuffer_cursor_y(dsp), tn5250_dbuffer_cursor_x(dsp));
   SLsmg_refresh();
}

static int slang_terminal_waitevent(Tn5250Terminal * This)
{
   fd_set fdr;
   int result = 0;
   int sm;

   if (SLang_Error == USER_BREAK)
      This->data->quit_flag = 1;
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

static int slang_terminal_getkey(Tn5250Terminal * This)
{
   unsigned int key;

   if (!SLang_input_pending (0))
      return -1;

   key = SLkp_getkey ();
   while (1) {
      switch (key) {
      case 0x0d:
      case 0x0a:
	 return K_ENTER;

      case 0x1b:
	 if ((key = slang_terminal_get_esc_key(This, 1)) != SL_KEY_ERR)
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
	 return -1;

      case K_CTRL('G'):	/* C-g <function-key-shortcut> */
	 if ((key = slang_terminal_get_esc_key(This, 0)) != SL_KEY_ERR)
	    return key;
	 break;

      case SL_KEY_ERR:
	 if (SLang_Error == USER_BREAK)
	    This->data->quit_flag = 1;
	 return -1;

      case 127:
	 return K_DELETE;

      case SL_KEY_F(1):
	 return K_F1;
      case SL_KEY_F(2):
	 return K_F2;
      case SL_KEY_F(3):
	 return K_F3;
      case SL_KEY_F(4):
	 return K_F4;
      case SL_KEY_F(5):
	 return K_F5;
      case SL_KEY_F(6):
	 return K_F6;
      case SL_KEY_F(7):
	 return K_F7;
      case SL_KEY_F(8):
	 return K_F8;
      case SL_KEY_F(9):
	 return K_F9;
      case SL_KEY_F(10):
	 return K_F10;
      case SL_KEY_F(11):
	 return K_F11;
      case SL_KEY_F(12):
	 return K_F12;
      case SL_KEY_F(13):
	 return K_F13;
      case SL_KEY_F(14):
	 return K_F14;
      case SL_KEY_F(15):
	 return K_F15;
      case SL_KEY_F(16):
	 return K_F16;
      case SL_KEY_F(17):
	 return K_F17;
      case SL_KEY_F(18):
	 return K_F18;
      case SL_KEY_F(19):
	 return K_F19;
      case SL_KEY_F(20):
	 return K_F20;
      case SL_KEY_F(21):
	 return K_F21;
      case SL_KEY_F(22):
	 return K_F22;
      case SL_KEY_F(23):
	 return K_F23;
      case SL_KEY_F(24):
	 return K_F24;
      case SL_KEY_BACKSPACE:
	 return K_BACKSPACE;
      case SL_KEY_IC:
	 return K_INSERT;

      case SL_KEY_HOME:
      case SL_KEY_A1:
	 return K_HOME;

      case SL_KEY_PPAGE:
      case SL_KEY_A3:
	 return K_ROLLDN;

      case SL_KEY_END:
      case SL_KEY_C1:
	 return K_END;

      case SL_KEY_NPAGE:
      case SL_KEY_C3:
	 return K_ROLLUP;

      case SL_KEY_ENTER:
	 return K_FIELDEXIT;

      case SL_KEY_UP:
	 return K_UP;

      case SL_KEY_DOWN:
	 return K_DOWN;

      case SL_KEY_LEFT:
	 return K_LEFT;

      case SL_KEY_RIGHT:
	 return K_RIGHT;

      default:
	 return key;
      }
   }
}

/*
 *    If a vt100 escape key sequence was introduced (using either
 *      <Esc> or <Ctrl+g>), handle the next key in the sequence.
 */
static int slang_terminal_get_esc_key(Tn5250Terminal * This,
				       int is_esc)
{
   int y, x, key, display_key;
   fd_set fdr;

   y = SLsmg_get_row ();
   x = SLsmg_get_column ();
   SLsmg_normal_video ();
   /* FIXME: This might need to be row 28 */
   SLsmg_gotorc (24, 60);
   if (is_esc)
      SLsmg_write_string ("Esc ");
   else
      SLsmg_write_string ("C-g ");
   SLsmg_gotorc (y, x);
   SLsmg_refresh();

   FD_ZERO(&fdr);
   FD_SET(0, &fdr);
   select(1, &fdr, NULL, NULL, NULL);
   key = SLkp_getkey ();

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
   case SL_KEY_DELETE:
      key = K_INSERT;
      break;			/* ESC DEL, also */
   case K_CTRL('J'):
      key = K_NEWLINE;
      break;

   case 'Q':
      This->data->quit_flag = 1;
      key = SL_KEY_ERR;
      break;

   default:
      SLtt_beep();
      key = SL_KEY_ERR;
      break;
   }

   /* FIXME: This might need to be row 27 */
   SLsmg_gotorc (24, 64);
   if (key == SL_KEY_ERR)
      SLsmg_write_string ("???");
   else
      SLsmg_write_char (display_key);
   SLsmg_gotorc (y, x);
   SLsmg_refresh ();
   return key;
}

static void slang_terminal_set_attrs (Tn5250Terminal * This, int attrs)
{
   if (attrs == This->data->attrs)
      return;
   SLsmg_normal_video ();
   SLsmg_set_color ((attrs & A_COLOR_MASK) >> 8);
   if ((attrs & A_REVERSE) != 0)
      SLsmg_reverse_video ();
   /* FIXME: Handle A_BLINK */
   This->data->attrs = attrs;
}

#else /* USE_SLANG */

/* When compiled with -Wall -pedantic: ANSI C forbids empty source file. */
struct NoSlang {
   long dummy;
};

#endif /* USE_SLANG */

/* vi:set cindent sts=3 sw=3: */
