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

#include "tn5250-private.h"

static void syntax(void);
static int parse_options(int argc, char *argv[]);

extern char *version_string;

Tn5250PrintSession *printsess = NULL;
Tn5250Stream *stream = NULL;

char remotehost[20];
char sessionname[20];
char transformname[20];
char outputcommand[30];
char *mapname = "37";
char logname[20] = "";

int main(int argc, char *argv[])
{
   

   if (parse_options(argc, argv) < 0)
      syntax();

    if (tn5250_daemon(0,0,0) < 0)
    {
        perror("tn5250_daemon");
        exit(2);
    }

#ifndef NDEBUG
   if(strlen(logname) > 0) {
      tn5250_log_open(logname);
      TN5250_LOG(("lp5250d version %s, built on %s\n", version_string, 
            __DATE__));
   }
#endif 

    openlog("lp5250d", LOG_PID, LOG_DAEMON);
    stream = tn5250_stream_open (remotehost);
    if(stream == NULL) {
       syslog(LOG_INFO, "Couldn't connect to %s", remotehost);
       exit(1);
    }
       
    printsess = tn5250_print_session_new();
    tn5250_stream_setenv(stream, "TERM", "IBM-3812-1");
    tn5250_stream_setenv(stream, "DEVNAME", sessionname);
    tn5250_stream_setenv(stream, "IBMFONT", "12");
    syslog(LOG_INFO, "DEVNAME = %s", sessionname);
    if (strlen(transformname) > 0) {
       syslog(LOG_INFO, "TRANSFORM = %s", transformname);
       tn5250_stream_setenv(stream, "IBMTRANSFORM", "1");
       tn5250_stream_setenv(stream, "IBMMFRTYPMDL", transformname);
    } else
       tn5250_stream_setenv(stream, "IBMTRANSFORM", "0");
    tn5250_print_session_set_fd(printsess, tn5250_stream_socket_handle(stream));
    tn5250_print_session_set_stream(printsess, stream);
    tn5250_print_session_set_char_map(printsess, mapname);
    tn5250_print_session_set_output_command(printsess, outputcommand);
    tn5250_print_session_main_loop(printsess);

    tn5250_print_session_destroy(printsess);
    tn5250_stream_destroy (stream);
 #ifndef NDEBUG
    tn5250_log_close();
#endif 
    return 0;
}

static int parse_options(int argc, char *argv[])
{
   int arg;
   
   while ((arg = getopt(argc, argv, "t:m:s:T:P:Vwy:")) != EOF) {
      switch (arg) {
#ifndef NDEBUG
      case 't':
         strcpy(logname,optarg);
         break;
#endif         
      case 'm':
	 mapname = optarg;
	 break;

      case 'P':
	 strcpy(outputcommand,optarg);
	 break;

      case 's':
	 strcpy(sessionname,optarg);
	 break;

      case 'T':
	 strcpy(transformname,optarg);
	 break;

      case 'V':
	 printf("tn5250 version %s\n\n", version_string);
	 exit(0);
	 break;

      case ':':
	 return -1;
      }
   }

   if (optind >= argc)
      return -1;
   strcpy(remotehost,argv[optind++]);
   if (optind != argc)
      return -1;
   return 0;
}

static void syntax()
{
   printf("Usage:  lp5250d [options] host[:port]\n"
	  "Options:\n"
	  "\t-t name	 specify FULL path to log file\n"
	  "\t-m map      specify translation map\n"
	  "\t-s name     specify session name\n"
	  "\t-T name     specify host print transform\n"
	  "\t-P cmd      specify the print output command\n"
	  "\t-V          display version\n");

   exit (255);
}
