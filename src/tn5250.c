/*
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

#include "tn5250-private.h"

char *remotehost;
char *mapname = "37";
char *sessionname = NULL;
char *termtype = NULL;
#ifndef NDEBUG
int debugpause = 1;
#endif

Tn5250Session *sess = NULL;
Tn5250Stream *stream = NULL;
Tn5250Terminal *term = NULL;
Tn5250Display *display = NULL;

static char sopts[] = "m:s:t:Vwy:";

/* FIXME: This should be moved into session.[ch] or something. */
static struct valid_term {
   char *name;
   char *descr;
} valid_terms[] = {
   /* DBCS Terminals not yet supported.
    * { "IBM-5555-C01", "DBCS color" },
    * { "IBM-5555-B01", "DBCS monocrome" }, */
   { "IBM-3477-FC",  "27x132 color" },
   { "IBM-3477-FG",  "27x132 monochrome" },
   { "IBM-3180-2",   "27x132 monochrome" },
   { "IBM-3179-2",   "24x80 color" },
   { "IBM-3196-A1",  "24x80 monochrome" },
   { "IBM-5292-2",   "24x80 color" },
   { "IBM-5291-1",   "24x80 monochrome" },
   { "IBM-5251-11",  "24x80 monochrome" },
   { NULL, NULL }
};

static void syntax(void);
static int parse_options(int argc, char *argv[]);

int main(int argc, char *argv[])
{
#ifdef HAVE_SETLOCALE
   setlocale (LC_ALL, "");
#endif

   if (parse_options(argc, argv) < 0)
      syntax();

   stream = tn5250_stream_open (remotehost);
   if (stream == NULL)
      goto bomb_out;

   display = tn5250_display_new ();
   if (mapname != NULL)
      tn5250_display_set_char_map (display, mapname);

#ifdef USE_CURSES
      term = tn5250_curses_terminal_new();
#endif
#ifdef USE_SLANG
      term = tn5250_slang_terminal_new();
#endif
      if (term == NULL)
	 goto bomb_out;
#ifndef NDEBUG
      /* Shrink-wrap the terminal with the debug terminal, if appropriate. */
      if (strlen (remotehost) >= 6
	    && !memcmp (remotehost, "debug:", 6)) {
	 Tn5250Terminal *dbgterm = tn5250_debug_terminal_new (term, stream);
	 if (dbgterm == NULL) {
	    tn5250_terminal_destroy (term);
	    goto bomb_out;
	 }
	 term = dbgterm;
	 tn5250_debug_terminal_set_pause (term, debugpause);
      }
#endif
      tn5250_terminal_init(term);
      tn5250_display_set_terminal(display, term);

      sess = tn5250_session_new();
      tn5250_display_set_session (display, sess);

      /* Set emulation type to the user-supplied terminal/emulation type,
       * if it exists; otherwise, make a guess based on the size of the
       * physical tty and whether the terminal supports color. */
      if (termtype == NULL) {
	 if (tn5250_terminal_width(term) >= 132 && tn5250_terminal_height(term) >= 27) {
	    if ((tn5250_terminal_flags(term) & TN5250_TERMINAL_HAS_COLOR) != 0)
	       tn5250_stream_setenv(stream, "TERM", "IBM-3477-FC");
	    else
	       tn5250_stream_setenv(stream, "TERM", "IBM-3477-FG");
	 } else {
	    if ((tn5250_terminal_flags(term) & TN5250_TERMINAL_HAS_COLOR) != 0)
	       tn5250_stream_setenv(stream, "TERM", "IBM-3179-2");
	    else
	       tn5250_stream_setenv(stream, "TERM", "IBM-5251-11");
	 }
      } else
	 tn5250_stream_setenv(stream, "TERM", termtype);

   tn5250_stream_setenv(stream, "DEVNAME", sessionname);

   term->conn_fd = tn5250_stream_socket_handle(stream);
   tn5250_session_set_stream(sess, stream);
   tn5250_session_main_loop(sess);

   errno = 0;

bomb_out:
   if (term != NULL)
      tn5250_terminal_term(term);
   if (sess != NULL)
      tn5250_session_destroy(sess);
   else if (stream != NULL)
      tn5250_stream_destroy (stream);

   if (errno != 0)
      printf("Could not start session: %s\n", strerror(errno));
#ifndef NDEBUG
   tn5250_log_close();
#endif
   exit(0);
}

static int parse_options(int argc, char *argv[])
{
   int arg;
   while ((arg = getopt (argc, argv, sopts)) != EOF) {
      switch (arg) {
      case 'm':
	 mapname = optarg;
	 break;

      case 's':
	 sessionname = optarg;
	 break;

#ifndef NDEBUG
      case 't':
	 { 
	    FILE *p;
	    int n;

	    tn5250_log_open(optarg);
	    /* Log some useful information to the tracefile. */
	    tn5250_log_printf("tn5250 version %s, built on %s\n", version_string,
		  __DATE__);

	    /* Get uname -a */
	    if ((p = popen ("uname -a", "r")) != NULL) {
	       char buf[256];
	       while ((n = fread (buf, 1, sizeof (buf)-2, p)) > 0) {
		  fwrite (buf, 1, n, tn5250_logfile);
	       }
	       pclose (p);
	    }

	    /* Get TERM. */
	    tn5250_log_printf("TERM=%s\n\n", getenv("TERM") ? getenv("TERM") : "(not set)");

	    for (n = 0; n < argc; n ++)
	       tn5250_log_printf("argv[%d] = %s\n", n, argv[n]); 

	    tn5250_log_printf("\n"); 
	 }
	 break;
#endif

      case 'V':
	 printf("tn5250 version %s\n\n", version_string);
	 exit(0);
	 break;

#ifndef NDEBUG
      case 'w':
	 debugpause = 0;
	 break;
#endif

      case 'y':
	 {
	    struct valid_term *p = valid_terms;
	    while (p->name != NULL && strcmp (p->name, optarg))
	       p++;
	    if (p->name == NULL)
	       syntax ();
	    termtype = optarg;
	 }
	 break;

      case '?':
      case ':':
	 return -1;
      }
   }

   if (optind >= argc)
      return -1;
   remotehost = argv[optind++];
   if (optind != argc)
      return -1;
   return 0;
}

static void syntax()
{
   struct valid_term *p;
   Tn5250CharMap *m;
   int i = 0;

   printf ("tn5250 - TCP/IP 5250 emulator\n\
Syntax:\n\
  tn5250 [options] HOST[:PORT]\n\
\n\
Options:\n\
   -m NAME                 Character map (default is '37'):");
   m = tn5250_transmaps;
   while (m->name != NULL) {
      if (i % 5 == 0)
	 printf ("\n                             "); 
      printf ("%s, ", m->name);
      m++; i++;
   }
   printf ("\n\
   -s NAME                 Use NAME as session name (default: none).\n"
#ifndef NDEBUG
"   -t FILE                 Log session to FILE.\n"
#endif
"   -V                      Show emulator version and exit.\n\
   -y TYPE                 Emulate IBM terminal type (default: depends)");
   p = valid_terms;
   while (p->name) {
      printf ("\n                             %s (%s)", p->name, p->descr); 
      p++;
   }
   printf ("\n\n");
   exit (255);
}

/* vi:set cindent sts=3 sw=3: */
