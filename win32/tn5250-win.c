/* 
 * Copyright (C) 2001 Scott Klement
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

#include "tn5250-private.h"

Tn5250Terminal *tn5250_win32_terminal_new(HINSTANCE hInst, 
    HINSTANCE hPrev, int show);

Tn5250Session *sess = NULL;
Tn5250Stream *stream = NULL;
Tn5250Terminal *term = NULL;
Tn5250Display *display = NULL;
Tn5250Config *config = NULL;

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

static void clean_up_and_exit(int error);
static void usage(void);
void msgboxf(const char *fmt, ...);
int parse_cmdline(char *cmdline, char **my_argv);

int WINAPI WinMain(HINSTANCE hInst, HINSTANCE prev, LPSTR cmdline, int show)
{
    WORD winsock_ver;
    WSADATA wsadata;
    char **argv = NULL;
    int argc, x;

    argc = parse_cmdline(cmdline, NULL);
    if (argc>0) {
          argv = (char **)malloc(argc * sizeof(char *));
          for (x=0; x<argc; x++) argv[x]=NULL;
          parse_cmdline(cmdline, argv);
    }

    winsock_ver = MAKEWORD(1, 1);
    if (WSAStartup(winsock_ver, &wsadata)) {
	MessageBox(NULL, "Unable to initialize WinSock", "TN5250",
		   MB_OK | MB_ICONEXCLAMATION);
	return 1;
    }
    if (LOBYTE(wsadata.wVersion) != 1 || HIBYTE(wsadata.wVersion) != 1) {
	MessageBox(NULL, "WinSock version is incompatible with 1.1",
		   "TN5250", MB_OK | MB_ICONEXCLAMATION);
	WSACleanup();
	return 1;
    }


    /*
     *  Load configuration file & command-line args 
     *
     *  FIXME: we are only handling _one_ command line arg.
     *    we should really parse it into potentially several.
     */

    config = tn5250_config_new ();
    if (tn5250_config_load_default (config) == -1) {
         tn5250_config_unref(config);
         exit (1);
    }
    if (tn5250_config_parse_argv (config, argc, argv) == -1) {
         tn5250_config_unref(config);
         usage ();
    }

    if (tn5250_config_get (config, "help") != NULL)
         usage ();
    else if (tn5250_config_get (config, "version") != NULL) {
         msgboxf("tn5250 %s", VERSION);
         exit (0);
    }
/* FIXME: We probably want to prompt for the host if not supplied... */
    else if (tn5250_config_get (config, "host") == NULL) {
          usage ();
    }

#ifndef NDEBUG
    if (tn5250_config_get (config, "trace")) 
         tn5250_log_open (tn5250_config_get (config, "trace"));
#endif

/* create connection to host */
    stream = tn5250_stream_open (tn5250_config_get (config, "host"), config);
    if (stream == NULL) {
         msgboxf("Unable to open communications stream!");
         clean_up_and_exit(errno);
    }

/* set up display terminal */

    display = tn5250_display_new ();
    if (tn5250_display_config (display, config) == -1)
         clean_up_and_exit(errno);

    term = tn5250_win32_terminal_new(hInst, prev, show);
    if (term == NULL)
        clean_up_and_exit(errno);
    if (tn5250_terminal_config (term, config) == -1) 
        clean_up_and_exit(errno);

#ifndef NDEBUG
    /* wrap the terminal with the debug terminal if a debug
       session is required. */
    {
       const char *remotehost = tn5250_config_get (config, "host");
       if (strlen(remotehost) >= 6
              && !memcmp (remotehost, "debug:", 6)) {
            Tn5250Terminal *dbgterm = tn5250_debug_terminal_new (term, stream);
            if (dbgterm == NULL) {
                tn5250_terminal_destroy (term);
                clean_up_and_exit(errno);
            }
            term = dbgterm;
            if (tn5250_terminal_config (term, config) == -1) 
                clean_up_and_exit(errno);
       }
    }
#endif
    tn5250_terminal_init(term);
    tn5250_display_set_terminal(display, term);

/* Create a new TN5250 session! */

    sess = tn5250_session_new();
    tn5250_display_set_session(display, sess);
    term->conn_fd = tn5250_stream_socket_handle(stream);
    tn5250_session_set_stream(sess, stream);

    if (tn5250_session_config (sess, config) == -1) 
        clean_up_and_exit(errno);

    tn5250_session_main_loop(sess);

    clean_up_and_exit(0);
}

/** 
 *  This is called when the program terminates.  It cleans up the 
 *  misc objects & reports an error if one exists.
 **/

static void clean_up_and_exit(int error) {

     if (term != NULL)
           tn5250_terminal_term(term);
     if (sess != NULL)
           tn5250_session_destroy(sess);
     if (stream != NULL)
           tn5250_stream_destroy(stream);
     if (config != NULL)
           tn5250_config_unref (config);
 
     if (error != 0) {
         char *error_note = "Could not start session:";
         char *error_msg = malloc(strlen(error_note)+strlen(strerror(error))+3);
         sprintf(error_msg, "%s %s", error_note, strerror(error));
         MessageBox (NULL, error_msg, "tn5250", MB_OK);
         free (error_msg);
     }

#ifndef NDEBUG
	tn5250_log_close();
#endif
     exit (0);
}

static void usage(void) {
 MessageBox(NULL,
 "Usage: tn5250 [options] HOST[:PORT]\n"
 "Options:\n"
 "\t+/-ruler			Draw a ruler pointing to the cursor position\n"
 "\t+/-version 		Display the version of the emulator and exit\n"
 "\t+/-pcspeaker 		Play beeps through the PC speaker\n"
 "\tmap=NAME 		Character map (default is '37')\n"
 "\tbeepfile=FILE 		Filename of a .wav file to play for beeps\n"
 "\tcopymode=MODE 		Copy to clipboard mode: 'bitmap', 'text' or 'both'\n"
 "\t+/-unix_like_copy		Text is copied to the clipboard automatically\n"
 "\t			when it is highlighted, and text can be pasted\n"
 "\t			by right-clicking\n"
 "\tfont_80=NAME  		Name of font to use in 80 col mode\n"
 "\tfont_132=NAME 		Name of font to use in 132 col mode\n"
 "\tenv.DEVNAME=NAME 	Use NAME as AS/400 device name(default: none)\n"
 "\tenv.TERM=TYPE 		Emulate IBM terminal type\n"
 "\ttrace=FILE 		Log debugging info to FILE\n"
 "\t+/-ssl_verify_server	 	Verify/don't verify the server's SSL certificate\n"
 "\tssl_ca_file=FILE 		Use certificate authority (CA) certs from FILE\n"
 "\tssl_cert_file=FILE 		File containing SSL certificate in PEM format to\n"
 "\t		 	   use if the AS/400 requires client authentication\n"
 "\tssl_pem_pass=PHRASE 	Passphrase to use when decrypting a PEM private\n"
 "\t			   key.  Used in conjunction with ssl_cert_file\n",
 "TN5250", 
 MB_OK|MB_ICONERROR
 );
 exit (255);
}


#define MAXMSG 1024
void msgboxf(const char *fmt, ...) {
   char themsg[MAXMSG+1];
   va_list vl;
   va_start(vl, fmt);
   _vsnprintf(themsg, MAXMSG, fmt, vl);
   va_end(vl);
   MessageBox(NULL, themsg, "TN5250", MB_OK);
}


/*
 *  Windows passes all arguments to WinMain() as a single string.
 *  this breaks them up to look like a typical argc, argv on
 *  a DOS or *NIX program.
 *
 *  Returns the number of arguments found.
 *  You can set my_argv to NULL if you just want a count of the arguments.
 *
 */
int parse_cmdline(char *cmdline, char **my_argv) {

#define MAXARG 256
    char q;
    int arglen, argcnt;
    char arg[MAXARG+1];
    int state = 0, pos = 0;
    
    if (my_argv!=NULL) {
         my_argv[0] = malloc(MAXARG+1);
         TN5250_ASSERT(my_argv[0]!=NULL);
         GetModuleFileName(NULL, my_argv[0], MAXARG);
    }

    argcnt = 1;
    arglen = 0;
    pos = 0;

    while (cmdline[pos]!=0) {
        switch (state) {
           case 0:
              switch (cmdline[pos]) {
                case '"':
                case '\'':
                   q = cmdline[pos];
                   state=1;
                   break;
                case ' ':
                   if (arglen>0 && argcnt<MAXARG) {
                       if (my_argv!=NULL) {
                            my_argv[argcnt] = 
                                   (char *)malloc((arglen+1) * sizeof(char));
                            TN5250_ASSERT(my_argv[argcnt]!=NULL);
                            strcpy(my_argv[argcnt], arg);
                       }
                       argcnt++;
                       arg[0]=0;
                       arglen=0;
                   }
                   break;
                default:
                   if (arglen<MAXARG) {
                       arg[arglen] = cmdline[pos];
                       arglen++;
                       arg[arglen] = '\0';
                   }
                   break;
              }
              break;
           case 1:
              if (cmdline[pos]==q) 
                  state = 0;
              else {
                  if (arglen<MAXARG) {
                      arg[arglen] = cmdline[pos];
                      arglen++;
                      arg[arglen] = '\0';
                  }
              }
              break;
        }
        pos++;
    }
    if (arglen>0 && argcnt<MAXARG) {
        if (my_argv!=NULL) {
            my_argv[argcnt] = (char *)malloc((arglen+1) * sizeof(char));
            TN5250_ASSERT(my_argv[argcnt]!=NULL);
            strcpy(my_argv[argcnt], arg);
        }
        argcnt++;
    }
              
    return argcnt;
#undef MAXARG
}
