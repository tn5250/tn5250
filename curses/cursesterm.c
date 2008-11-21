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
#define _TN5250_TERMINAL_PRIVATE_DEFINED
#include "tn5250-private.h"
#include "cursesterm.h"

#ifdef USE_CURSES

/* Some versions of ncurses don't have this defined. */
#ifndef A_VERTICAL
#define A_VERTICAL ((1UL) << ((22) + 8))
#endif				/* A_VERTICAL */

/* Some older versions of ncurses don't define NCURSES_COLOR_T */
#ifndef NCURSES_COLOR_T
#define NCURSES_COLOR_T short
#endif

/* Mapping of 5250 colors to curses colors */
struct _curses_color_map {
   char *name;
   NCURSES_COLOR_T ref;
   attr_t bld;
};
typedef struct _curses_color_map curses_color_map;

static curses_color_map colorlist[] =
{
  { "black",     COLOR_BLACK         },
  { "red",       COLOR_RED,   A_BOLD },
  { "green",     COLOR_GREEN         },
  { "yellow",    COLOR_YELLOW,A_BOLD },
  { "blue",      COLOR_CYAN,  A_BOLD },
  { "pink",      COLOR_MAGENTA       },
  { "turquoise", COLOR_CYAN          },
  { "white",     COLOR_WHITE, A_BOLD },
  { NULL, -1 }
};

#define A_5250_GREEN    ((attr_t)COLOR_PAIR(COLOR_GREEN)|colorlist[COLOR_GREEN].bld)
#define A_5250_WHITE    ((attr_t)COLOR_PAIR(COLOR_WHITE)|colorlist[COLOR_WHITE].bld)
#define A_5250_RED      ((attr_t)COLOR_PAIR(COLOR_RED)|colorlist[COLOR_RED].bld)
#define A_5250_TURQ     ((attr_t)COLOR_PAIR(COLOR_CYAN)|colorlist[COLOR_CYAN].bld)
#define A_5250_YELLOW   ((attr_t)COLOR_PAIR(COLOR_YELLOW)|colorlist[COLOR_YELLOW].bld)
#define A_5250_PINK     ((attr_t)COLOR_PAIR(COLOR_MAGENTA)|colorlist[COLOR_MAGENTA].bld)
#define A_5250_BLUE     ((attr_t)COLOR_PAIR(COLOR_BLUE)|colorlist[COLOR_BLUE].bld)

/*@-globstate -nullpass@*/  /* lclint incorrectly assumes stdscr may be NULL */

static attr_t attribute_map[33];

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
static int curses_terminal_enhanced (Tn5250Terminal * This);
static int curses_terminal_is_ruler(Tn5250Terminal *This, Tn5250Display *display, int x, int y);
int curses_rgb_to_color(int r, int g, int b, int *rclr, int *rbold);
int curses_terminal_config(Tn5250Terminal *This, Tn5250Config *config);
void curses_terminal_print_screen(Tn5250Terminal *This, Tn5250Display *display);
void curses_postscript_print(FILE *out, int x, int y, char *string, attr_t attr);

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
   char *         font_80;
   char *         font_132;
   Tn5250Display  *display;
   Tn5250Config   *config;
   int		  quit_flag : 1;
   int		  have_underscores : 1;
   int		  underscores : 1;
   int		  is_xterm : 1;
   int		  display_ruler : 1;
   int            local_print : 1;
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
   { K_MEMO,            "\023" }, /* CTRL S */
   { K_TESTREQ,         "\024" }, /* CTRL T */
   { K_ROLLDN,          "\025" }, /* CTRL U */
   { K_EXEC,            "\027" }, /* CTRL W */
   { K_FIELDPLUS,       "\030" }, /* CTRL X */

   /* ASCII DEL is not correctly reported as the DC key in some
    * termcaps */
   /* But it is backspace in some termcaps... */
   /* { K_DELETE,		"\177" }, */ /* ASCII DEL */
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
   { K_NEXTFLD,		"\033\025" }, /* ESC Ctrl-U */
   { K_PREVFLD,		"\033\010" }, /* ESC Ctrl-H */
   { K_FIELDHOME,	"\033\006" }, /* ESC Ctrl-F */
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
   r->data->font_80 = NULL;
   r->data->font_132 = NULL;
   r->data->display_ruler = 0;
   r->data->local_print = 0;
   r->data->display = NULL;
   r->data->config = NULL;

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
   r->putkey = NULL;
   r->beep = curses_terminal_beep;
   r->enhanced = curses_terminal_enhanced;
   r->config = curses_terminal_config;
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
   int i = 0, c, s;
   char *str;
   int x;

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
      init_pair(COLOR_BLACK, colorlist[COLOR_BLACK].ref, 
                             colorlist[COLOR_BLACK].ref);
      init_pair(COLOR_GREEN, colorlist[COLOR_GREEN].ref, 
                             colorlist[COLOR_BLACK].ref);
      init_pair(COLOR_RED,   colorlist[COLOR_RED  ].ref, 
                             colorlist[COLOR_BLACK].ref);
      init_pair(COLOR_CYAN,  colorlist[COLOR_CYAN ].ref, 
                             colorlist[COLOR_BLACK].ref);
      init_pair(COLOR_WHITE, colorlist[COLOR_WHITE].ref, 
                             colorlist[COLOR_BLACK].ref);
      init_pair(COLOR_MAGENTA,colorlist[COLOR_MAGENTA].ref, 
                             colorlist[COLOR_BLACK].ref);
      init_pair(COLOR_BLUE  ,colorlist[COLOR_BLUE ].ref, 
                             colorlist[COLOR_BLACK].ref);
      init_pair(COLOR_YELLOW,colorlist[COLOR_YELLOW].ref, 
                             colorlist[COLOR_BLACK].ref);
   }

   x=-1;
   attribute_map[++x] = A_5250_GREEN;
   attribute_map[++x] = A_5250_GREEN | A_REVERSE;
   attribute_map[++x] = A_5250_WHITE;
   attribute_map[++x] = A_5250_WHITE | A_REVERSE;
   attribute_map[++x] = A_5250_GREEN | A_UNDERLINE;
   attribute_map[++x] = A_5250_GREEN | A_UNDERLINE | A_REVERSE;
   attribute_map[++x] = A_5250_WHITE | A_UNDERLINE;
   attribute_map[++x] = 0x00;
   attribute_map[++x] = A_5250_RED;
   attribute_map[++x] = A_5250_RED | A_REVERSE;
   attribute_map[++x] = A_5250_RED | A_BLINK;
   attribute_map[++x] = A_5250_RED | A_BLINK | A_REVERSE;
   attribute_map[++x] = A_5250_RED | A_UNDERLINE;
   attribute_map[++x] = A_5250_RED | A_UNDERLINE | A_REVERSE;
   attribute_map[++x] = A_5250_RED | A_UNDERLINE | A_BLINK;
   attribute_map[++x] = 0x00;
   attribute_map[++x] = A_5250_TURQ | A_VERTICAL;
   attribute_map[++x] = A_5250_TURQ | A_VERTICAL | A_REVERSE;
   attribute_map[++x] = A_5250_YELLOW | A_VERTICAL;
   attribute_map[++x] = A_5250_YELLOW | A_VERTICAL | A_REVERSE;
   attribute_map[++x] = A_5250_TURQ | A_UNDERLINE | A_VERTICAL;
   attribute_map[++x] = A_5250_TURQ | A_UNDERLINE | A_REVERSE | A_VERTICAL;
   attribute_map[++x] = A_5250_YELLOW | A_UNDERLINE | A_VERTICAL;
   attribute_map[++x] = 0x00;
   attribute_map[++x] = A_5250_PINK;
   attribute_map[++x] = A_5250_PINK | A_REVERSE;
   attribute_map[++x] = A_5250_BLUE;
   attribute_map[++x] = A_5250_BLUE | A_REVERSE;
   attribute_map[++x] = A_5250_PINK | A_UNDERLINE;
   attribute_map[++x] = A_5250_PINK | A_UNDERLINE | A_REVERSE;
   attribute_map[++x] = A_5250_BLUE | A_UNDERLINE;
   attribute_map[++x] = 0x00;

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

/****i* lib5250/tn5250_curses_terminal_display_ruler
 * NAME
 *    tn5250_curses_terminal_display_ruler
 * SYNOPSIS
 *    tn5250_curses_terminal_display_ruler (This, f);
 * INPUTS
 *    Tn5250Terminal  *    This       - The curses terminal object.
 *    int                  f          - Flag, set to 1 to show ruler
 * DESCRIPTION
 *    Call this function to tell a curses terminal to display a 
 *    "ruler" that pinpoints where the cursor is on a given screen.
 *****/
void tn5250_curses_terminal_display_ruler (Tn5250Terminal *This, int f)
{
   This->data->display_ruler = f;
}

/****i* lib5250/tn5250_curses_terminal_set_xterm_font
 * NAME
 *  tn5250_curses_terminal_set_xterm_font  
 * SYNOPSIS
 *    tn5250_curses_terminal_set_xterm_font (This, font80, font132);
 * INPUTS
 *    Tn5250Terminal  *    This       - curses terminal object
 *    const char  *	   font80     - string to send when using 80 col font
 *    const char  *	   font132    - string to send when using 132 col font
 * DESCRIPTION
 *    When using an xterm, it is sometimes desirable to change fonts when
 *    switching from 80 to 132 col mode.  If this is not explicitly set,
 *    no font-change will be sent to the xterm.
 *
 *    Font changes consist of "\x1b]50;<font string>\x07".  You only need
 *    specify the "<font string>" portion as an argument to this function.
 *****/
void tn5250_curses_terminal_set_xterm_font (Tn5250Terminal *This, 
                                 const char *font80, const char *font132)
{
   This->data->font_80 = malloc(strlen(font80) + 6);
   This->data->font_132 = malloc(strlen(font132) + 6);
   sprintf(This->data->font_80, "\x1b]50;%s\x07", font80);
   sprintf(This->data->font_132, "\x1b]50;%s\x07", font132);
   TN5250_LOG(("font_80 = %s.\n",This->data->font_80));
   TN5250_LOG(("font_132 = %s.\n",This->data->font_132));
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
   if (This->data->font_80 !=NULL) 
      free(This->data->font_80);
   if (This->data->font_132 !=NULL) 
      free(This->data->font_132);
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

   This->data->display = display;

   if (This->data->last_width != tn5250_display_width(display)
       || This->data->last_height != tn5250_display_height(display)) {
      clear();
      if(This->data->is_xterm) {
         if (This->data->font_132!=NULL) {
               if (tn5250_display_width (display)>100)
                    printf(This->data->font_132);
               else
                    printf(This->data->font_80);
         }
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

/* XXX: this is somewhat of a hack.  For some reason the change to
      132 col lags a bit, causing our update to fail, so this just waits
      for the update to finish...  (is there a better way?) */

      for (x=0; x<10; x++) {
          refresh ();
          if (tn5250_display_width(display)==curses_terminal_width(This)-1)
              break;
          usleep(10000);
      }
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
               if (curses_terminal_is_ruler(This, display, x, y)) {
                  addch( A_REVERSE | attribute_map[0] | ' ');
               } else {
         	  addch(attribute_map[0] | ' ');
               }
	 } else {		/* DATA */
	    curs_attr = attribute_map[a - 0x20];
	    if (curs_attr == 0x00) {	/* NONDISPLAY */
               if (curses_terminal_is_ruler(This, display, x, y)) {
                  addch( A_REVERSE | attribute_map[0] | ' ');
               } else {
         	  addch(attribute_map[0] | ' ');
               }
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
               if (curses_terminal_is_ruler(This, display, x, y)) {
		  curs_attr |= A_REVERSE;
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


/****i* lib5250/curses_terminal_is_ruler
 * NAME
 *    curses_terminal_is_ruler
 * SYNOPSIS
 *    curses_terminal_is_ruler (display, x, y);
 * INPUTS
 *    Tn5250Terminal *     This       - 
 *    Tn5250Display *      display    - 
 *    int 		   x          - position of char on X axis
 *    int		   y          - position of char on Y axis
 * DESCRIPTION
 *    Returns 1 if a char on the screen should be displayed as part
 *    of the ruler, or 0 if it should not.
 *****/
static int curses_terminal_is_ruler(Tn5250Terminal *This,
               Tn5250Display *display, int x, int y) {

   if (!(This->data->display_ruler)) return 0;

   if (x==tn5250_display_cursor_x(display)) { 
        return 1;
   }
   if (y==tn5250_display_cursor_y(display)) { 
        return 1;
   }

   return 0;
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
   if ((inds & TN5250_DISPLAY_IND_MACRO) != 0)
      memcpy(ind_buf + 54, tn5250_macro_printstate (display), 11);
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
      case K_CTRL('S'):
	 return K_MEMO;
      case K_CTRL('T'):
	 return K_TESTREQ;
      case K_CTRL('U'):
	 return K_ROLLDN;
      case K_CTRL('W'):
	 return K_EXEC;
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


/***** lib5250/curses_terminal_enhanced
 * NAME
 *    curses_terminal_enhanced
 * SYNOPSIS
 *    ret = curses_terminal_enhanced (This);
 * INPUTS
 *    Tn5250Terminal  *    This       - 
 * DESCRIPTION
 *    Return 1 if we support the enhanced 5250 protocol, 0 otherwise.
 *****/
static int
curses_terminal_enhanced (Tn5250Terminal * This)
{
  return (0);
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

   case K_CTRL('U'):
      key = K_NEXTFLD;
      break;
   case K_CTRL('H'):
      key = K_PREVFLD;
      break;
   case K_CTRL('F'):
      key = K_FIELDHOME;
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
   case K_PRINT:
      if (This->data->local_print) {
         curses_terminal_print_screen(This, This->data->display);
         ch = K_RESET;
      }
      break;
   }
   return ch;
}
#endif

/****i* lib5250/curses_rgb_to_color
 * NAME
 *    curses_rgb_to_color
 * SYNOPSIS
 *    curses_rgb_to_color (r, g, b, &clr, &bld);
 * INPUTS
 *    int                  r          -    
 *    int                  g          -    
 *    int                  b          -    
 *    int            *     rclr       -    
 *    int            *     rbold      -    
 * DESCRIPTION
 *    DOCUMENT ME!!!
 *****/
int curses_rgb_to_color(int r, int g, int b, int *rclr, int *rbold) {

    int clr;
    
    clr = ((r & 0xff) << 16) | ((g & 0xff) << 8) | (b & 0xff);
    *rbold = A_NORMAL;

    switch (clr) {
       case 0xFFFFFF:
	 *rclr = COLOR_WHITE;   *rbold = A_BOLD;
         break;
       case 0xFFFF00:
	 *rclr = COLOR_YELLOW;  *rbold = A_BOLD;
         break;
       case 0xFF00FF:
	 *rclr = COLOR_MAGENTA; *rbold = A_BOLD;
         break;
       case 0xFF0000:
	 *rclr = COLOR_RED;     *rbold = A_BOLD;
         break;
       case 0x00FFFF:
	 *rclr = COLOR_CYAN;    *rbold = A_BOLD;
         break;
       case 0x00FF00:
	 *rclr = COLOR_GREEN;   *rbold = A_BOLD;
         break;
       case 0x0000FF:
	 *rclr = COLOR_BLUE;    *rbold = A_BOLD;
         break;
       case 0x808080:
	 *rclr = COLOR_WHITE;
         break;
       case 0xC0C0C0:
	 *rclr = COLOR_WHITE;
         break;
       case 0x808000:
	 *rclr = COLOR_YELLOW;
         break;
       case 0x800080:
	 *rclr = COLOR_MAGENTA;
         break;
       case 0x800000:
	 *rclr = COLOR_RED;
         break;
       case 0x008080:
	 *rclr = COLOR_CYAN;
         break;
       case 0x008000:
	 *rclr = COLOR_GREEN;
         break;
       case 0x000080:
	 *rclr = COLOR_BLUE;
         break;
       case 0x000000:
	 *rclr = COLOR_BLACK;
         break;
       default:
         return -1;
    }

    return 0;
}

/****i* lib5250/tn5250_curses_terminal_load_colorlist
 * NAME
 *    tn5250_curses_terminal_load_colorlist
 * SYNOPSIS
 *    tn5250_curses_terminal_load_colorlist(config);
 * INPUTS
 *    Tn5250Config   *     config     -    
 * DESCRIPTION
 *    DOCUMENT ME!!!
 *****/
void tn5250_curses_terminal_load_colorlist(Tn5250Config *config) {

   int r, g, b, x, clr, bld;

   if (tn5250_config_get_bool(config, "black_on_white")) {
       for (x=COLOR_BLACK+1; x<=COLOR_WHITE; x++) {
           colorlist[x].ref = COLOR_BLACK;
           colorlist[x].bld = A_NORMAL;
       }
       colorlist[COLOR_BLACK].ref = COLOR_WHITE;
       colorlist[COLOR_BLACK].bld = A_BOLD;
   }

   if (tn5250_config_get_bool(config, "white_on_black")) {
       for (x=COLOR_BLACK+1; x<=COLOR_WHITE; x++) {
           colorlist[x].ref = COLOR_WHITE;
           colorlist[x].bld = A_BOLD;
       }
       colorlist[COLOR_BLACK].ref = COLOR_BLACK;
       colorlist[COLOR_BLACK].bld = A_NORMAL;
   }

   x=0;
   while (colorlist[x].name != NULL) {
       if (tn5250_parse_color(config, colorlist[x].name, &r, &g, &b)!=-1) {
           if (curses_rgb_to_color(r,g,b, &clr, &bld) != -1) {
                colorlist[x].ref = clr;
                colorlist[x].bld = bld;
           }
       }
       x++;
   }

}


/****i* lib5250/curses_terminal_config
 * NAME
 *    curses_terminal_config
 * SYNOPSIS
 *    curses_terminal_config(This, config);
 * INPUTS
 *    Tn5250Terminal *     This       -
 *    Tn5250Display  *     config     -
 * DESCRIPTION
 *    Assign a set of configuration data to the terminal
 *****/
int curses_terminal_config(Tn5250Terminal *This, Tn5250Config *config) {
    This->data->config = config;
    if (tn5250_config_get_bool(config, "local_print_key"))
          This->data->local_print = 1;
    return 0;
}


/****i* lib5250/curses_terminal_print_screen
 * NAME
 *    curses_terminal_print_screen
 * SYNOPSIS
 *    curses_terminal_print_screen(This, This->data->display);
 * INPUTS
 *    Tn5250Terminal *     This       -
 *    Tn5250Display  *     display    -
 * DESCRIPTION
 *    Generate PostScript output of current screen image.
 *****/
void curses_terminal_print_screen(Tn5250Terminal *This, Tn5250Display *display) {

   int x, y, c, a;
   attr_t curs_attr;
   int px, py;
   int leftmar, topmar;
   FILE *out;
   const char *outcmd;
   double pgwid, pglen;
   double colwidth, rowheight, fontsize;
   int textlen;
   char *prttext;

   if (display==NULL)
       return;

   /* default values for printing screens are: */

   outcmd = "lpr";
   pglen = 11 * 72;
   pgwid = 8.5 * 72;
   leftmar = 18;
   topmar = 36;
   if (tn5250_display_width(display) == 132)
        fontsize = 7.0;
   else
        fontsize = 10.0;

   /* override defaults with values from config if available */
       
   if (This->data->config != NULL) {
       int fs80=0, fs132=0;
       if (tn5250_config_get(This->data->config, "outputcommand"))
           outcmd = tn5250_config_get(This->data->config, "outputcommand");
       if (tn5250_config_get(This->data->config, "pagewidth"))
           pgwid = atoi(tn5250_config_get(This->data->config, "pagewidth"));
       if (tn5250_config_get(This->data->config, "pagelength"))
           pglen = atoi(tn5250_config_get(This->data->config, "pagelength"));
       if (tn5250_config_get(This->data->config, "leftmargin"))
           leftmar = atoi(tn5250_config_get(This->data->config, "leftmargin"));
       if (tn5250_config_get(This->data->config, "topmargin"))
           topmar = atoi(tn5250_config_get(This->data->config, "topmargin"));
       if (tn5250_config_get(This->data->config, "psfontsize_80"))
           fs80 = atoi(tn5250_config_get(This->data->config, "psfontsize_80"));
       if (tn5250_config_get(This->data->config, "psfontsize_80"))
           fs132 =atoi(tn5250_config_get(This->data->config, "psfontsize_132"));
       if (tn5250_display_width(display)==132 && fs132!=0)
           fontsize = fs132;
       if (tn5250_display_width(display)==80 && fs80!=0)
           fontsize = fs80;
   }
        
   colwidth  = (pgwid - leftmar*2) / tn5250_display_width(display);
   rowheight = (pglen  - topmar*2) / 66;

 
   /* allocate enough memory to store the largest possible string that we
      could output.   Note that it could be twice the size of the screen
      if every single character needs to be escaped... */
   prttext = malloc((2 * tn5250_display_width(display) *
		     tn5250_display_height(display)) + 1);

   out = popen(outcmd, "w");
   if (out == NULL)
       return;

   fprintf(out, "%%!PS-Adobe-3.0\n");
   fprintf(out, "%%%%Pages: 1\n");
   fprintf(out, "%%%%Title: TN5250 Print Screen\n");
   fprintf(out, "%%%%BoundingBox: 0 0 %.0f %.0f\n", pgwid, pglen);
   fprintf(out, "%%%%LanguageLevel: 2\n");
   fprintf(out, "%%%%EndComments\n\n");
   fprintf(out, "%%%%BeginProlog\n");
   fprintf(out, "%%%%BeginResource: procset general 1.0.0\n");
   fprintf(out, "%%%%Title: (General Procedures)\n");
   fprintf(out, "%%%%Version: 1.0\n");
   fprintf(out, "%% Courier is a fixed-pitch font, so one character is as\n");
   fprintf(out, "%%   good as another for determining the height/width\n");
   fprintf(out, "/Courier %.2f selectfont\n", fontsize);
   fprintf(out, "/chrwid (W) stringwidth pop def\n");
   fprintf(out, "/pglen %.2f def\n", pglen);
   fprintf(out, "/pgwid %.2f def\n", pgwid);
   fprintf(out, "/chrhgt %.2f def\n", rowheight);
   fprintf(out, "/leftmar %d def\n", leftmar + 2);
   fprintf(out, "/topmar %d def\n", topmar);
   fprintf(out, "/exploc {           %% expand x y to dot positions\n"
                "   chrhgt mul\n"
                "   topmar add\n"
                "   3 add\n"
                "   pglen exch sub\n"
                "   exch\n"
                "   chrwid mul\n"
                "   leftmar add\n"
                "   3 add\n"
                "   exch\n"
                "} bind def\n");
   fprintf(out, "/prtnorm {          %% print text normally (text) x y color\n"
                "   setgray\n"
                "   exploc moveto\n"
                "   show\n"
                "} bind def\n");
   fprintf(out, "/drawunderline  { %% draw underline: (string) x y color\n"
                "   gsave\n"
                "   0 setlinewidth\n"
                "   setgray\n"
                "   exploc\n"
                "   2 sub\n"
                "   moveto\n"
                "   stringwidth pop 0\n"
                "   rlineto\n"
                "   stroke\n"
                "   grestore\n"
                "} bind def\n");
   fprintf(out, "/blkbox {       %% draw a black box behind the text\n"
                "   gsave\n"
                "   newpath\n"
                "   0 setgray\n"
                "   exploc\n"
                "   3 sub\n"
                "   moveto\n"
                "   0 chrhgt rlineto\n"
                "   stringwidth pop 0 rlineto\n"
                "   0 0 chrhgt sub rlineto\n"
                "   closepath\n"
                "   fill\n"
                "   grestore\n"
                "} bind def\n");
   fprintf(out, "/borderbox { %% Print a border around screen dump\n"
                "   gsave\n"
                "   newpath\n"
                "   0 setlinewidth\n"
                "   0 setgray\n"
                "   leftmar\n"
                "   topmar chrhgt sub pglen exch sub\n"
                "   moveto\n"
                "   chrwid %d mul 6 add 0 rlineto\n"
                "   0 0 chrhgt %d mul 6 add sub rlineto\n"
                "   0 chrwid %d mul 6 add sub 0 rlineto\n"
                "   closepath\n"
                "   stroke\n"
                "   grestore\n"
                "} bind def\n", 
                tn5250_display_width(display),
                tn5250_display_height(display)+1,
                tn5250_display_width(display));
   fprintf(out, "%%%%EndResource\n");
   fprintf(out, "%%%%EndProlog\n\n");
   fprintf(out, "%%%%Page 1 1\n");
   fprintf(out, "%%%%BeginPageSetup\n");
   fprintf(out, "/pgsave save def\n");
   fprintf(out, "%%%%EndPageSetup\n");
   
   curs_attr =  0x00;
   textlen = 0;
   px = -1;

   for (y = 0; y < tn5250_display_height(display); y++) {

      for (x = 0; x < tn5250_display_width(display); x++) {

	 c = tn5250_display_char_at(display, y, x);
	 if ((c & 0xe0) == 0x20) {
            if (textlen > 0) {
                curses_postscript_print(out, px, py, prttext, curs_attr);
                textlen = 0;
            }
	    a = (c & 0xff);
            curs_attr = attribute_map[a - 0x20];
            px = -1;
         } else { 
            if (px == -1) {
                px = x;
                py = y;
            }
	    if ((c < 0x40 && c > 0x00) || c == 0xff) { 
	       c = ' ';
	    } else {
	       c = tn5250_char_map_to_local (
                      tn5250_display_char_map (display), c);
	    }
            if (c == '\\' || c == '(' || c == ')') {
                 prttext[textlen] = '\\';
                 textlen++;
            }
            prttext[textlen] = c;
            textlen++;
            prttext[textlen] = '\0';
         }
      }
      if (textlen > 0) {
          curses_postscript_print(out, px, py, prttext, curs_attr);
          textlen = 0;
      }
      px = -1;
   }

   fprintf(out, "borderbox\n");
   fprintf(out, "pgsave restore\n");
   fprintf(out, "showpage\n");
   fprintf(out, "%%%%PageTrailer\n");
   fprintf(out, "%%%%Trailer\n");
   fprintf(out, "%%%%Pages: 1\n");
   fprintf(out, "%%%%EOF\n");

   pclose(out);

   free(prttext);

   attrset(attribute_map[0]);
   clear();
   mvprintw(0, 0, "Print Screen Successful!");
   mvprintw(1, 0, "Press ENTER to continue.");
   refresh();

   while (curses_terminal_getkey(This)!=K_ENTER) { /* wait */ }

   curses_terminal_update(This, display);
}


/****i* lib5250/curses_postscript_print
 * NAME
 *    curses_postscript_print
 * SYNOPSIS
 *    curses_postscript_print(out, px, py, "Print this", A_NORMAL);
 * INPUTS
 *    FILE           *     out        -
 *    int                  x          -
 *    int                  y          -
 *    char           *     string     -
 *    attr_t               attr       -
 * DESCRIPTION
 *    Adds a printed string to the postscript output generated by 
 *    curses_terminal_print_screen.   Converts the curses attributes
 *    to attributes understood by postscript (using the procedures
 *    we put in the prolog of the ps document)
 *****/

void curses_postscript_print(FILE *out, int x, int y, char *string, attr_t attr) {
    int color;

    if (attr == 0x00)    /* NONDISPLAY */
        return;

    color = 0;
    if (attr & A_REVERSE) {   /* Print white text on black background */
        color = 1;
        fprintf(out, "(%s) %d %d blkbox\n", string, x, y);
    } 

    fprintf(out, "(%s) %d %d %d prtnorm\n", string, x, y, color);

    if (attr & A_UNDERLINE)    /* draw underline below text */
        fprintf(out, "(%s) %d %d %d drawunderline\n", string, x, y, color);

}


#endif /* USE_CURSES */

/* vi:set cindent sts=3 sw=3: */
