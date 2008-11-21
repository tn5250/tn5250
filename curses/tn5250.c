/*
 * Copyright (C) 1997-2008 Michael Madore
 *
 * This file is part of TN5250.
 *
 * TN5250 is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 *
 * TN5250 is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include "tn5250-private.h"

Tn5250Session *sess = NULL;
Tn5250Stream *stream = NULL;
Tn5250Terminal *term = NULL;
Tn5250Display *display = NULL;
Tn5250Config *config = NULL;
Tn5250Macro *macro = NULL;

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

int main(int argc, char *argv[])
{
#ifndef NDEBUG
   const char *tracefile;
#endif

#ifdef HAVE_SETLOCALE
   setlocale (LC_ALL, "");
#endif

   config = tn5250_config_new ();
   if (tn5250_config_load_default (config) == -1) {
      tn5250_config_unref (config);
      exit (1);
   }

   if (tn5250_config_parse_argv (config, argc, argv) == -1) {
      tn5250_config_unref (config);
      syntax ();
   }

   if (tn5250_config_get (config, "help"))
      syntax ();
   else if (tn5250_config_get (config, "version")) {
      printf ("tn5250 %s\n", VERSION);
      exit (0);
   } else if (!tn5250_config_get (config, "host")) {
      syntax ();
   }

#ifndef NDEBUG
   tracefile = tn5250_config_get (config, "trace");
   if (tracefile != NULL)
      tn5250_log_open (tn5250_config_get (config, "trace"));
#endif

   stream = tn5250_stream_open (tn5250_config_get (config, "host"), config);
   if (stream == NULL)
      goto bomb_out;

   display = tn5250_display_new ();
   if (tn5250_display_config (display, config) == -1)
      goto bomb_out;

#ifdef USE_CURSES
   term = tn5250_curses_terminal_new();
   if (tn5250_config_get (config, "underscores")) {
      tn5250_curses_terminal_use_underscores(term,
	    tn5250_config_get_bool (config, "underscores")
	    );
   }
   if (tn5250_config_get (config, "ruler")) {
      tn5250_curses_terminal_display_ruler(term,
	    tn5250_config_get_bool (config, "ruler")
	    );
   }
   if ((tn5250_config_get (config, "font_80")) 
       && (tn5250_config_get (config, "font_132"))) {
      tn5250_curses_terminal_set_xterm_font (term,   
            tn5250_config_get (config, "font_80"),
            tn5250_config_get (config, "font_132")
            );
   }
   tn5250_curses_terminal_load_colorlist(config);
#endif
#ifdef USE_SLANG
   term = tn5250_slang_terminal_new();
#endif
   if (term == NULL)
      goto bomb_out;
   if (tn5250_terminal_config (term, config) == -1)
      goto bomb_out;
#ifndef NDEBUG
   /* Shrink-wrap the terminal with the debug terminal, if appropriate. */
   {
      const char *remotehost = tn5250_config_get (config, "host");
      if (strlen (remotehost) >= 6
	    && !memcmp (remotehost, "debug:", 6)) {
	 Tn5250Terminal *dbgterm = tn5250_debug_terminal_new (term, stream);
	 if (dbgterm == NULL) {
	    tn5250_terminal_destroy (term);
	    goto bomb_out;
	 }
	 term = dbgterm;
	 if (tn5250_terminal_config (term, config) == -1)
	    goto bomb_out;
      }
   }
#endif
   tn5250_terminal_init(term);
   tn5250_display_set_terminal(display, term);

   sess = tn5250_session_new();
   tn5250_display_set_session (display, sess);

   term->conn_fd = tn5250_stream_socket_handle(stream);
   tn5250_session_set_stream(sess, stream);
   if (tn5250_session_config (sess, config) == -1)
      goto bomb_out;

   macro = tn5250_macro_init() ;
   tn5250_macro_attach (display, macro) ;

   tn5250_session_main_loop(sess);
   errno = 0;

bomb_out:
   if (macro != NULL)
      tn5250_macro_exit(macro);
   if (term != NULL)
      tn5250_terminal_term(term);
   if (sess != NULL)
      tn5250_session_destroy(sess);
   else if (stream != NULL)
      tn5250_stream_destroy (stream);
   if (config != NULL)
      tn5250_config_unref (config);

   if (errno != 0)
      printf("Could not start session: %s\n", strerror(errno));
#ifndef NDEBUG
   tn5250_log_close();
#endif
   exit(0);
}

static void syntax()
{
   struct valid_term *p;
   Tn5250CharMap *m;
   int i = 0;

   printf ("tn5250 - TCP/IP 5250 emulator\n\
Syntax:\n\
  tn5250 [options] HOST[:PORT]\n");
#ifdef HAVE_LIBSSL
   printf ("\
   To connect using ssl prefix HOST with 'ssl:'.  Example:\
      tn5250 +ssl_verify_server ssl:as400.example.com\n");
#endif
   printf ("\n\
Options:\n\
   map=NAME                Character map (default is '37'):");
   m = tn5250_transmaps;
   while (m->name != NULL) {
      if (i % 5 == 0)
	 printf ("\n                             "); 
      printf ("%s, ", m->name);
      m++; i++;
   }
   printf ("\n\
   env.DEVNAME=NAME         Use NAME as session name (default: none).\n");
#ifndef NDEBUG
   printf ("\
   trace=FILE              Log session to FILE.\n");
#endif
#ifdef HAVE_LIBSSL
   printf ("\
   +/-ssl_verify_server    Verify/don't verify the server's SSL certificate\n\
   ssl_ca_file=FILE        Use certificate authority (CA) certs from FILE\n\
   ssl_cert_file=FILE      File containing SSL certificate in PEM format to\n\
                           use if the AS/400 requires client authentication.\n\
   ssl_pem_pass=PHRASE     Passphrase to use when decrypting a PEM private\n\
                           key.  Used in conjunction with ssl_cert_file\n\
   ssl_check_exp[=SECS]    Check if SSL certificate is expired, or if it\n\
                           will be expired in SECS seconds.\n");
#endif
   printf ("\
   +/-underscores          Use/don't use underscores instead of underline\n\
                           attribute.\n\
   +/-ruler		   Draw a ruler pointing to the cursor position\n\
   +/-version              Show emulator version and exit.\n\
   env.NAME=VALUE          Set telnet environment string NAME to VALUE.\n\
   env.TERM=TYPE           Emulate IBM terminal type (default: depends)");
   p = valid_terms;
   while (p->name) {
      printf ("\n                             %s (%s)", p->name, p->descr); 
      p++;
   }
   printf ("\n\n");
   exit (255);
}

/* vi:set cindent sts=3 sw=3: */
