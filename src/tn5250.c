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

#include "config.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#ifdef HAVE_GETOPT_H
#include <getopt.h>
#endif
#include <errno.h>

#ifdef HAVE_LOCALE_H
#include <locale.h>
#endif

#include "buffer.h"
#include "record.h"
#include "stream.h"
#include "utility.h"
#include "dbuffer.h"
#include "field.h"
#include "formattable.h"
#include "codes5250.h"
#include "terminal.h"
#include "session.h"
#include "printsession.h"
#if USE_CURSES
#include "cursesterm.h"
#endif
#if USE_SLANG
#include "slangterm.h"
#endif
#include "debug.h"

extern char *version_string;

char *remotehost;
char *transmapname = "37";
char *sessionname = NULL;
#if USE_CURSES
int underscores = 0;
#endif
int printsession = 0;
char *transformname = NULL;
char *outputcommand = NULL;
char *termtype = NULL;
#ifndef NDEBUG
int debugpause = 1;
#endif

Tn5250PrintSession *printsess = NULL;
Tn5250Session *sess = NULL;
Tn5250Stream *stream = NULL;
Tn5250Terminal *term = NULL;

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
   tn5250_settransmap(transmapname);

   stream = tn5250_stream_open (remotehost);
   if (stream == NULL)
      goto bomb_out;

   if (printsession) {
      printsess = tn5250_print_session_new();
      tn5250_stream_setenv(stream, "TERM", "IBM-3812-1");
   } else {
#ifdef USE_CURSES
      term = tn5250_curses_terminal_new();
#endif
#ifdef USE_SLANG
      term = tn5250_slang_terminal_new();
#endif
      if (term == NULL)
	 goto bomb_out;
#ifdef USE_CURSES
      tn5250_curses_terminal_use_underscores(term, underscores);
#endif
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
      sess = tn5250_session_new();
      tn5250_session_set_terminal(sess, term);

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
   }

   tn5250_stream_setenv(stream, "DEVNAME", sessionname);
   tn5250_stream_setenv(stream, "IBMFONT", "12");
   if (transformname != NULL) {
      tn5250_stream_setenv(stream, "IBMTRANSFORM", "0");
      tn5250_stream_setenv(stream, "IBMMFRTYPMDL", transformname);
   } else
      tn5250_stream_setenv(stream, "IBMTRANSFORM", "1");

   if (printsession) {
      tn5250_print_session_set_fd(printsess, tn5250_stream_socket_handle(stream));
      tn5250_print_session_set_stream(printsess, stream);
      printf("-%s-\n", outputcommand);
      tn5250_print_session_set_output_command(printsess, outputcommand);
      tn5250_print_session_main_loop(printsess);
   } else {
      term->conn_fd = tn5250_stream_socket_handle(stream);
      tn5250_session_set_stream(sess, stream);
      tn5250_session_main_loop(sess);
   }

   errno = 0;

bomb_out:
   if (!printsession) {
      if (term != NULL)
	 tn5250_terminal_term(term);
      if (sess != NULL)
	 tn5250_session_destroy(sess);
      else if (stream != NULL)
	 tn5250_stream_destroy (stream);
   } else {
      if (printsess != NULL)
	 tn5250_print_session_destroy(printsess);
      if (stream != NULL)
	 tn5250_stream_destroy (stream);
   }
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
   while ((arg = getopt(argc, argv, "m:s:t:T:P:uVpwy:")) != EOF) {
      switch (arg) {
      case 'm':
	 transmapname = optarg;
	 break;

      case 'P':
	 outputcommand = optarg;
	 break;

      case 's':
	 sessionname = optarg;
	 break;

#if USE_CURSES
      case 'u':
	 underscores = 1;
	 break;
#endif

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

      case 'T':
	 transformname = optarg;
	 break;

      case 'V':
	 printf("tn5250 version %s\n\n", version_string);
	 exit(0);
	 break;

      case 'p':
	 printsession = 1;
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
   printf("Usage:  tn5250 [options] host[:port]\n"
	  "Options:\n"
	  "\t-m map      specify translation map\n"
	  "\t-s name     specify session name\n"
#ifndef NDEBUG
	  "\t-t file     specify trace file\n"
#endif
#if USE_CURSES
	  "\t-u          use underscores instead of underline attribute\n"
#endif
	  "\t-V          display version\n"
#ifndef NDEBUG
	  "\t-w          don't wait for input when debugging\n"
#endif
	  "\t-y type     specify IBM terminal type to emulate.\n");

   p = valid_terms;
   while (p->name) {
      printf ("\t\t%s\t%s\n", p->name, p->descr);
      p++;
   }
   printf ("\n");
   exit (255);
}

/* vi:set cindent sts=3 sw=3: */
