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
 * As a special exception, the copyright holder gives permission
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
#define _TN5250_TERMINAL_PRIVATE_DEFINED
#include "tn5250-private.h"

#ifdef USE_CURSES

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
				   Tn5250Display * display) /*@modifies This@*/;
static void curses_terminal_update_indicators(Tn5250Terminal * This,
					      Tn5250Display * display) /*@modifies This@*/;
static int curses_terminal_waitevent(Tn5250Terminal * This) /*@modifies This@*/;
static int curses_terminal_getkey(Tn5250Terminal * This) /*@modifies This@*/;
static int curses_terminal_get_esc_key(Tn5250Terminal * This, int is_esc) /*@modifies This@*/;
static void curses_terminal_beep(Tn5250Terminal * This);

#ifdef USE_OWN_KEY_PARSING
struct _Key {
   int	       k_code;
   char        k_str[10];
};

typedef struct _Key Key;
#endif

#define MAX_K_BUF_LEN 20

struct _Tn5250TerminalPrivate {
   int		  last_width, last_height;
#ifdef USE_OWN_KEY_PARSING
   unsigned char  k_buf[MAX_K_BUF_LEN];
   int		  k_buf_len;

   Key *	  k_map;
   int		  k_map_len;
#endif
   int		  quit_flag : 1;
   int		  have_underscores : 1;
   int		  underscores : 1;
   int		  is_xterm : 1;
};

#ifdef USE_OWN_KEY_PARSING
/* This is an array mapping our key code to a termcap capability
 * name. */
static Key curses_caps[] = {
   { K_ENTER, "@8" },
   { K_ENTER, "cr" },
   { K_BACKTAB, "kB" },
   { K_F1, "k1" },
   { K_F2, "k2" },
   { K_F3, "k3" },
   { K_F4, "k4" },
   { K_F5, "k5" },
   { K_F6, "k6" },
   { K_F7, "k7" },
   { K_F8, "k8" },
   { K_F9, "k9" },
   { K_F10, "k;" },
   { K_F11, "F1" },
   { K_F12, "F2" },
   { K_F13, "F3" },
   { K_F14, "F4" },
   { K_F15, "F5" },
   { K_F16, "F6" },
   { K_F17, "F7" },
   { K_F18, "F8" },
   { K_F19, "F9" },
   { K_F20, "FA" },
   { K_F21, "FB" },
   { K_F22, "FC" },
   { K_F23, "FD" },
   { K_F24, "FE" },
   { K_LEFT, "kl" },
   { K_RIGHT, "kr" },
   { K_UP, "ku" },
   { K_DOWN, "kd" },
   { K_ROLLDN, "kP" },
   { K_ROLLUP, "kN" },
   { K_BACKSPACE, "kb" },
   { K_HOME, "kh" },
   { K_END, "@7" },
   { K_INSERT, "kI" },
   { K_DELETE, "kD" },
   { K_PRINT, "%9" },
   { K_HELP, "%1" },
   { K_CLEAR, "kC" },
   { K_REFRESH, "&2" },
   { K_FIELDEXIT, "@9" },
};

/* This is an array mapping some of our vt100 sequences to our internal
 * key names. */
static Key curses_vt100[] = {
   /* CTRL strings */
   { K_ATTENTION,	"\001" }, /* CTRL A */
   { K_ROLLDN,		"\002" }, /* CTRL B */
   { K_SYSREQ,		"\003" }, /* CTRL C */
   { K_ROLLUP,		"\004" }, /* CTRL D */
   { K_ERASE,           "\005" }, /* CTRL E */
   { K_ROLLUP,		"\006" }, /* CTRL F */
   { K_FIELDEXIT,       "\013" }, /* CTRL K */
   { K_REFRESH,         "\014" }, /* CTRL L */
   { K_HOME,            "\017" }, /* CTRL O */
   { K_PRINT,           "\020" }, /* CTRL P */
   { K_RESET,           "\022" }, /* CTRL R */
   { K_TESTREQ,         "\024" }, /* CTRL T */
   { K_ROLLDN,          "\025" }, /* CTRL U */
   { K_FIELDPLUS,       "\030" }, /* CTRL X */

   /* ASCII DEL is not correctly reported as the DC key in some
    * termcaps */
   /* But it is backspace in some termcaps... */
   /* { K_DELETE,		"\177" }, /* ASCII DEL */
   { K_DELETE,		"\033\133\063\176" }, /* ASCII DEL Sequence: \E[3~ */
   

   /* ESC strings */
   { K_F1,		"\033\061" }, /* ESC 1 */
   { K_F2,		"\033\062" }, /* ESC 2 */
   { K_F3,		"\033\063" }, /* ESC 3 */
   { K_F4,		"\033\064" }, /* ESC 4 */
   { K_F5,		"\033\065" }, /* ESC 5 */
   { K_F6,		"\033\066" }, /* ESC 6 */
   { K_F7,		"\033\067" }, /* ESC 7 */
   { K_F8,		"\033\070" }, /* ESC 8 */
   { K_F9,		"\033\071" }, /* ESC 9 */
   { K_F10,		"\033\060" }, /* ESC 0 */
   { K_F11,		"\033\055" }, /* ESC - */
   { K_F12,		"\033\075" }, /* ESC = */
   { K_F13,		"\033\041" }, /* ESC ! */
   { K_F14,		"\033\100" }, /* ESC @ */
   { K_F15,		"\033\043" }, /* ESC # */
   { K_F16,		"\033\044" }, /* ESC $ */
   { K_F17,		"\033\045" }, /* ESC % */
   { K_F18,		"\033\136" }, /* ESC ^ */
   { K_F19,		"\033\046" }, /* ESC & */
   { K_F20,		"\033\052" }, /* ESC * */
   { K_F21,		"\033\050" }, /* ESC ( */
   { K_F22,		"\033\051" }, /* ESC ) */
   { K_F23,		"\033\137" }, /* ESC _ */
   { K_F24,		"\033\053" }, /* ESC + */
   { K_ATTENTION,	"\033\101" }, /* ESC A */
   { K_CLEAR,		"\033\103" }, /* ESC C */
   { K_DUPLICATE,	"\033\104" }, /* ESC D */
   { K_HELP,		"\033\110" }, /* ESC H */
   { K_INSERT,		"\033\111" }, /* ESC I */
   { K_REFRESH,		"\033\114" }, /* ESC L */
   { K_FIELDMINUS,	"\033\115" }, /* ESC M */
   { K_NEWLINE,		"\033\116" }, /* ESC N */ /* Our extension */
   { K_PRINT,		"\033\120" }, /* ESC P */
   { K_RESET,		"\033\122" }, /* ESC R */
   { K_SYSREQ,		"\033\123" }, /* ESC S */
   { K_TOGGLE,		"\033\124" }, /* ESC T */
   { K_FIELDEXIT,	"\033\130" }, /* ESC X */
   { K_NEWLINE,         "\033\012" }, /* ESC ^J */
   { K_NEWLINE,         "\033\015" }, /* ESC ^M */
   { K_INSERT,		"\033\177" }, /* ESC DEL */
   /* K_INSERT = ESC+DEL handled in code below. */
};
#endif

/****f* lib5250/tn5250_curses_terminal_new
 * NAME
 *    tn5250_curses_terminal_new
 * SYNOPSIS
 *    ret = tn5250_curses_terminal_new ();
 * INPUTS
 *    None
 * DESCRIPTION
 *    Create a new curses terminal object.
 *****/
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

   r->data->have_underscores = 0;
   r->data->underscores = 0;
   r->data->quit_flag = 0;
   r->data->last_width = 0;
   r->data->last_height = 0;
   r->data->is_xterm = 0;

#ifdef USE_OWN_KEY_PARSING
   r->data->k_buf_len = 0;
   r->data->k_map_len = 0;
   r->data->k_map = NULL;
#endif

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
   r->beep = curses_terminal_beep;
   r->config = NULL;
   return r;
}

/****i* lib5250/curses_terminal_init
 * NAME
 *    curses_terminal_init
 * SYNOPSIS
 *    curses_terminal_init (This);
 * INPUTS
 *    Tn5250Terminal *     This       - 
 * DESCRIPTION
 *    DOCUMENT ME!!!
 *****/
static void curses_terminal_init(Tn5250Terminal * This)
{
   char buf[MAX_K_BUF_LEN];
   int i = 0, c, s;
   struct timeval tv;
   char *str;

   (void)initscr();
   raw();

#ifdef USE_OWN_KEY_PARSING
   if ((str = (unsigned char *)tgetstr ("ks", NULL)) != NULL)
      tputs (str, 1, putchar);
   fflush (stdout);
#else
   keypad(stdscr, 1);
#endif

   nodelay(stdscr, 1);
   noecho();

   /* Determine if we're talking to an xterm ;) */
   if ((str = getenv("TERM")) != NULL &&
	 (!strcmp (str, "xterm") || !strcmp (str, "xterm-5250")
	  || !strcmp (str, "xterm-color"))) 
      This->data->is_xterm = 1;

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

   /* Determine if the terminal supports underlining. */
   if (This->data->have_underscores == 0) {
      if ((unsigned char *)tgetstr ("us", NULL) == NULL)
	 This->data->underscores = 1;
      else
	 This->data->underscores = 0;
   }

#ifdef USE_OWN_KEY_PARSING
   /* Allocate and populate an array of escape code => key code 
    * mappings. */
   This->data->k_map_len = (sizeof (curses_vt100) / sizeof (Key)) * 2
      + sizeof (curses_caps) / sizeof (Key) + 1;
   This->data->k_map = (Key*)malloc (sizeof (Key)*This->data->k_map_len);

   c = sizeof (curses_caps) / sizeof (Key);
   s = sizeof (curses_vt100) / sizeof (Key);
   for (i = 0; i < c; i++) {
      This->data->k_map[i].k_code = curses_caps[i].k_code;
      if ((str = (unsigned char *)tgetstr (curses_caps[i].k_str, NULL)) != NULL) {
	 TN5250_LOG(("Found string for cap '%s': '%s'.\n",
		  curses_caps[i].k_str, str));
	 strcpy (This->data->k_map[i].k_str, str);
      } else
	 This->data->k_map[i].k_str[0] = '\0';
   }

   /* Populate vt100 escape codes, both ESC+ and C-g+ forms. */
   for (i = 0; i < sizeof (curses_vt100) / sizeof (Key); i++) {
      This->data->k_map[i + c].k_code =
	 This->data->k_map[i + c + s].k_code =
	 curses_vt100[i].k_code;
      strcpy (This->data->k_map[i + c].k_str, curses_vt100[i].k_str);
      strcpy (This->data->k_map[i + c + s].k_str, curses_vt100[i].k_str);

      if (This->data->k_map[i + c + s].k_str[0] == '\033')
	 This->data->k_map[i + c + s].k_str[0] = K_CTRL('G');
      else 
	 This->data->k_map[i + c + s].k_str[0] = '\0';
   }

   /* Damn the exceptions to the rules. (ESC + DEL) */
   This->data->k_map[This->data->k_map_len-1].k_code = K_INSERT;
   This->data->k_map[This->data->k_map_len-s-1].k_code = K_INSERT;
   if ((str = (unsigned char *)tgetstr ("kD", NULL)) != NULL) {
      This->data->k_map[This->data->k_map_len-1].k_str[0] = '\033';
      This->data->k_map[This->data->k_map_len-s-1].k_str[0] = K_CTRL('G');
      strcpy (This->data->k_map[This->data->k_map_len-1].k_str + 1, str);
      strcpy (This->data->k_map[This->data->k_map_len-s-1].k_str + 1, str);
   } else
      This->data->k_map[This->data->k_map_len-1].k_str[0] =
	 This->data->k_map[This->data->k_map_len-s-1].k_str[0] = 0;
#endif
}

/****i* lib5250/tn5250_curses_terminal_use_underscores
 * NAME
 *    tn5250_curses_terminal_use_underscores
 * SYNOPSIS
 *    tn5250_curses_terminal_use_underscores (This, f);
 * INPUTS
 *    Tn5250Terminal  *    This       - The curses terminal object.
 *    int                  f          - Flag to use underscores
 * DESCRIPTION
 *    This function instructs the curses terminal to use underscore
 *    characters (`_') for blank cells in under-lined fields instead of
 *    using the curses underline attribute.  This is necessary on terminals
 *    which don't support the underline attribute.
 *
 *    If this is not explicitly set, the curses terminal will determine
 *    if it should use underscores by checking for the "us" termcap 
 *    capability.  This may not always produce the desired effect.
 *****/
void tn5250_curses_terminal_use_underscores (Tn5250Terminal *This, int u)
{
   This->data->have_underscores = 1;
   This->data->underscores = u;
}

/****i* lib5250/curses_terminal_term
 * NAME
 *    curses_terminal_term
 * SYNOPSIS
 *    curses_terminal_term (This);
 * INPUTS
 *    Tn5250Terminal  *    This       - 
 * DESCRIPTION
 *    DOCUMENT ME!!!
 *****/
static void curses_terminal_term(Tn5250Terminal /*@unused@*/ * This)
{
   endwin();
}

/****i* lib5250/curses_terminal_destroy
 * NAME
 *    curses_terminal_destroy
 * SYNOPSIS
 *    curses_terminal_destroy (This);
 * INPUTS
 *    Tn5250Terminal *     This       - 
 * DESCRIPTION
 *    DOCUMENT ME!!!
 *****/
static void curses_terminal_destroy(Tn5250Terminal * This)
{
#ifdef USE_OWN_KEY_PARSING
   if (This->data->k_map != NULL)
      free(This->data->k_map);
#endif
   if (This->data != NULL)
      free(This->data);
   free(This);
}

/****i* lib5250/curses_terminal_width
 * NAME
 *    curses_terminal_width
 * SYNOPSIS
 *    ret = curses_terminal_width (This);
 * INPUTS
 *    Tn5250Terminal  *    This       - 
 * DESCRIPTION
 *    DOCUMENT ME!!!
 *****/
static int curses_terminal_width(Tn5250Terminal /*@unused@*/ * This)
{
   int y, x;
   getmaxyx(stdscr, y, x);
   return x + 1;
}

/****i* lib5250/curses_terminal_height
 * NAME
 *    curses_terminal_height
 * SYNOPSIS
 *    ret = curses_terminal_height (This);
 * INPUTS
 *    Tn5250Terminal  *    This       - 
 * DESCRIPTION
 *    DOCUMENT ME!!!
 *****/
static int curses_terminal_height(Tn5250Terminal /*@unused@*/ * This)
{
   int y, x;
   getmaxyx(stdscr, y, x);
   return y + 1;
}

/****i* lib5250/curses_terminal_flags
 * NAME
 *    curses_terminal_flags
 * SYNOPSIS
 *    ret = curses_terminal_flags (This);
 * INPUTS
 *    Tn5250Terminal  *    This       - 
 * DESCRIPTION
 *    DOCUMENT ME!!!
 *****/
static int curses_terminal_flags(Tn5250Terminal /*@unused@*/ * This)
{
   int f = 0;
   if (has_colors() != 0)
      f |= TN5250_TERMINAL_HAS_COLOR;
   return f;
}

/****i* lib5250/curses_terminal_update
 * NAME
 *    curses_terminal_update
 * SYNOPSIS
 *    curses_terminal_update (This, display);
 * INPUTS
 *    Tn5250Terminal *     This       - 
 *    Tn5250Display *      display    - 
 * DESCRIPTION
 *    DOCUMENT ME!!!
 *****/
static void curses_terminal_update(Tn5250Terminal * This, Tn5250Display *display)
{
   int my, mx;
   int y, x;
   attr_t curs_attr;
   unsigned char a = 0x20, c;

   if (This->data->last_width != tn5250_display_width(display)
       || This->data->last_height != tn5250_display_height(display)) {
      clear();
      if(This->data->is_xterm) {
	 printf ("\x1b[8;%d;%dt", tn5250_display_height (display)+1,
	       tn5250_display_width (display));
	 fflush (stdout);
#ifdef HAVE_RESIZETERM
	 resizeterm(tn5250_display_height(display)+1, tn5250_display_width(display)+1);
#endif
 	 /* Make sure we get a SIGWINCH - We need curses to resize its
 	  * buffer. */
 	 raise (SIGWINCH);
	 refresh ();
      }
      This->data->last_width = tn5250_display_width(display);
      This->data->last_height = tn5250_display_height(display);
   }
   attrset(A_NORMAL);
   getmaxyx(stdscr, my, mx);
   for (y = 0; y < tn5250_display_height(display); y++) {
      if (y > my)
	 break;

      move(y, 0);
      for (x = 0; x < tn5250_display_width(display); x++) {
	 c = tn5250_display_char_at(display, y, x);
	 if ((c & 0xe0) == 0x20) {	/* ATTRIBUTE */
	    a = (c & 0xff);
	    addch(attribute_map[0] | ' ');
	 } else {		/* DATA */
	    curs_attr = attribute_map[a - 0x20];
	    if (curs_attr == 0x00) {	/* NONDISPLAY */
	       addch(attribute_map[0] | ' ');
	    } else {
               /* UNPRINTABLE -- print block */
               if ((c==0x1f) || (c==0x3F)) {
		  c = ' ';
		  curs_attr ^= A_REVERSE;
               }
               /* UNPRINTABLE -- print blank */
	       else if ((c < 0x40 && c > 0x00) || c == 0xff) { 
		  c = ' ';
	       } else {
		  c = tn5250_char_map_to_local (tn5250_display_char_map (display), c);
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

   move(tn5250_display_cursor_y(display), tn5250_display_cursor_x(display));

   /* This performs the refresh () */
   curses_terminal_update_indicators(This, display);
}

/****i* lib5250/curses_terminal_update_indicators
 * NAME
 *    curses_terminal_update_indicators
 * SYNOPSIS
 *    curses_terminal_update_indicators (This, display);
 * INPUTS
 *    Tn5250Terminal  *    This       - 
 *    Tn5250Display *      display    - 
 * DESCRIPTION
 *    DOCUMENT ME!!!
 *****/
static void curses_terminal_update_indicators(Tn5250Terminal /*@unused@*/ * This, Tn5250Display *display)
{
   int inds = tn5250_display_indicators(display);
   char ind_buf[80];

   memset(ind_buf, ' ', sizeof(ind_buf));
   memcpy(ind_buf, "5250", 4);
   if ((inds & TN5250_DISPLAY_IND_MESSAGE_WAITING) != 0)
      memcpy(ind_buf + 23, "MW", 2);
   if ((inds & TN5250_DISPLAY_IND_INHIBIT) != 0)
      memcpy(ind_buf + 9, "X II", 4);
   else if ((inds & TN5250_DISPLAY_IND_X_CLOCK) != 0)
      memcpy(ind_buf + 9, "X CLOCK", 7);
   else if ((inds & TN5250_DISPLAY_IND_X_SYSTEM) != 0)
      memcpy(ind_buf + 9, "X SYSTEM", 8);
   if ((inds & TN5250_DISPLAY_IND_INSERT) != 0)
      memcpy(ind_buf + 30, "IM", 2);
   if ((inds & TN5250_DISPLAY_IND_FER) != 0)
      memcpy(ind_buf + 33, "FER", 3);
   sprintf(ind_buf+72,"%03.3d/%03.3d",tn5250_display_cursor_x(display)+1,
      tn5250_display_cursor_y(display)+1);

   attrset( (attr_t)COLOR_PAIR(COLOR_WHITE) );
   mvaddnstr(tn5250_display_height(display), 0, ind_buf, 80);
   move(tn5250_display_cursor_y(display), tn5250_display_cursor_x(display));
   attrset(A_NORMAL);
   refresh();
}

/****i* lib5250/curses_terminal_waitevent
 * NAME
 *    curses_terminal_waitevent
 * SYNOPSIS
 *    ret = curses_terminal_waitevent (This);
 * INPUTS
 *    Tn5250Terminal *     This       - 
 * DESCRIPTION
 *    DOCUMENT ME!!!
 *****/
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

#ifndef USE_OWN_KEY_PARSING
/****i* lib5250/curses_terminal_getkey
 * NAME
 *    curses_terminal_getkey
 * SYNOPSIS
 *    ret = curses_terminal_getkey (This);
 * INPUTS
 *    Tn5250Terminal *     This       - 
 * DESCRIPTION
 *    DOCUMENT ME!!!
 *****/
static int curses_terminal_getkey(Tn5250Terminal * This)
{
   int key;
  
   key = getch();

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
	 return K_FIELDPLUS;

      case K_CTRL('Q'):
	 This->data->quit_flag = 1;
	 return -1;

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
	 return K_END;

      case KEY_C3:
	 return K_ROLLUP;

      case KEY_ENTER:
	 return K_FIELDEXIT;

      case KEY_END:
	 return K_END;

      default:
	 return key;
      }
   }
}
#endif

/****i* lib5250/curses_terminal_beep
 * NAME
 *    curses_terminal_beep
 * SYNOPSIS
 *    curses_terminal_beep (This);
 * INPUTS
 *    Tn5250Terminal *     This       - 
 * DESCRIPTION
 *    DOCUMENT ME!!!
 *****/
static void curses_terminal_beep (Tn5250Terminal *This)
{
   TN5250_LOG (("CURSES: beep\n"));
   beep ();
   refresh ();
}

#ifndef USE_OWN_KEY_PARSING
/****i* lib5250/curses_terminal_get_esc_key
 * NAME
 *    curses_terminal_get_esc_key
 * SYNOPSIS
 *    ret = curses_terminal_get_esc_key (This, is_esc);
 * INPUTS
 *    Tn5250Terminal *     This       - 
 *    int                  is_esc     - 
 * DESCRIPTION
 *    If a vt100 escape key sequence was introduced (using either
 *    <Esc> or <Ctrl+g>), handle the next key in the sequence.
 *****/
static int curses_terminal_get_esc_key(Tn5250Terminal * This, int is_esc)
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
      key = -1;
      break;

   case ERR:
   default:
      beep();
      key = -1;
      break;
   }

   if (key == -1)
      mvaddstr(24, 64, "???");
   else
      mvaddch(24, 64, (chtype)display_key);
   move(y, x);
   refresh();
   return key;
}
#endif

#ifdef USE_OWN_KEY_PARSING
static int curses_get_key (Tn5250Terminal *This, int rmflag)
{
   int i, j;
   int have_incomplete_match = -1;
   int have_complete_match = -1;
   int complete_match_len;

   /* Fast path */
   if (This->data->k_buf_len == 0)
      return -1;

   /* Look up escape codes. */
   for (i = 0; i < This->data->k_map_len; i++) {

      /* Skip empty entries. */
      if (This->data->k_map[i].k_str[0] == '\0')
	 continue;

      for (j = 0; j < MAX_K_BUF_LEN + 1; j++) {
	 if (This->data->k_map[i].k_str[j] == '\0') {
	    have_complete_match = i;
	    complete_match_len = j;
	    break;
	 }
	 if (j == This->data->k_buf_len) {
	    have_incomplete_match = i;
	    TN5250_LOG (("Have incomplete match ('%s')\n",
		     This->data->k_map[i].k_str));
	    break;
	 }
	 if (This->data->k_map[i].k_str[j] != This->data->k_buf[j])
	    break; /* No match */
      }

   }

   if (have_incomplete_match == -1 && have_complete_match == -1) {
      /* At this point, we know that we don't have an escape sequence,
       * so just return the next character. */
      i = This->data->k_buf[0];
      if (rmflag) {
	 memmove (This->data->k_buf, This->data->k_buf + 1, MAX_K_BUF_LEN - 1);
	 This->data->k_buf_len --;
      }
      return i;
   }

   if (have_incomplete_match != -1)
      return -1;

   if (have_complete_match != -1) {
      if (rmflag) {
	 if (This->data->k_buf_len - complete_match_len > 0) {
	    memmove (This->data->k_buf, This->data->k_buf + complete_match_len,
		  This->data->k_buf_len - complete_match_len);
	 }
	 This->data->k_buf_len -= complete_match_len;
      }
      return This->data->k_map[have_complete_match].k_code;
   }

   return -1;
}

static int curses_terminal_getkey (Tn5250Terminal *This)
{
   int ch;

   /* Retreive all keys from the keyboard buffer. */
   while (This->data->k_buf_len < MAX_K_BUF_LEN && (ch = getch ()) != ERR) {
      TN5250_LOG(("curses_getch: received 0x%02X.\n", ch));

      /* FIXME: Here would be the proper place to get mouse events :) */
     
      /* HACK! Why are we gettings these 0410s still?  ncurses bug? */
      if (ch < 0 || ch > 255)
	 continue;

      This->data->k_buf[This->data->k_buf_len++] = ch;
   }

   ch = curses_get_key (This, 1);
   switch (ch) {
   case K_CTRL('Q'):
      This->data->quit_flag = 1;
      return -1;
   case 0x0a:
      return 0x0d;
   }
   return ch;
}
#endif


#endif /* USE_CURSES */

/* vi:set cindent sts=3 sw=3: */
