/* TN5250 - An implementation of the 5250 telnet protocol.
 * Copyright (C) 1997 Michael Madore
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

static void syntax(void);
static int parse_options(int argc, char *argv[]);

extern char *version_string;

Tn5250PrintSession *printsess = NULL;
Tn5250Stream *stream = NULL;
Tn5250Config *config = NULL;

int main(int argc, char *argv[])
{

   config = tn5250_config_new ();
   if (tn5250_config_load_default (config) == -1) {
        tn5250_config_unref(config);
        exit (1);  
   }
   if (tn5250_config_parse_argv (config, argc, argv) == -1) {
       tn5250_config_unref(config);
       syntax();  
   }

   if (tn5250_config_get (config, "help")) 
       syntax();
   else if (tn5250_config_get (config, "version")) {
       printf ("tn5250 version %s\n", version_string);
       exit (0);
   }
   else if (!tn5250_config_get (config, "host"))   
       syntax();


    if (tn5250_daemon(0,0,0) < 0)
    {
        perror("tn5250_daemon");
        exit(2);
    } 

#ifndef NDEBUG
   if(tn5250_config_get (config, "trace")) {
      tn5250_log_open(tn5250_config_get (config, "trace"));
      TN5250_LOG(("lp5250d version %s, built on %s\n", version_string, 
            __DATE__));
      TN5250_LOG(("host = %s\n", tn5250_config_get (config, "host")));
   }
#endif 

    openlog("lp5250d", LOG_PID, LOG_DAEMON);

    stream = tn5250_stream_open (tn5250_config_get(config, "host"), config);
    if(stream == NULL) {
       syslog(LOG_INFO, "Couldn't connect to %s", 
		         tn5250_config_get (config,"host"));
       exit(1);
    }
       
    printsess = tn5250_print_session_new();
    tn5250_stream_setenv(stream, "TERM", "IBM-3812-1");
/*    tn5250_stream_setenv(stream, "DEVNAME", sessionname); */
    tn5250_stream_setenv(stream, "IBMFONT", "12");
    syslog(LOG_INFO, "DEVNAME = %s", tn5250_stream_getenv(stream, "DEVNAME"));
    if (tn5250_stream_getenv(stream, "IBMMFRTYPMDL")) {
       syslog(LOG_INFO, "TRANSFORM = %s", 
                     tn5250_stream_getenv(stream, "IBMMFRTYPMDL"));
       tn5250_stream_setenv(stream, "IBMTRANSFORM", "1");
    } else
       tn5250_stream_setenv(stream, "IBMTRANSFORM", "0");
    tn5250_print_session_set_fd(printsess, tn5250_stream_socket_handle(stream));
    tn5250_print_session_set_stream(printsess, stream);
    if (tn5250_config_get (config, "map")) {
         tn5250_print_session_set_char_map(printsess, 
                  tn5250_config_get (config, "map"));
    } else {
         tn5250_print_session_set_char_map(printsess, "37");
    }
    if (tn5250_config_get (config, "outputcommand")) 
         tn5250_print_session_set_output_command(printsess, 
                 tn5250_config_get (config, "outputcommand"));
    else
         tn5250_print_session_set_output_command(printsess, 
                 "scs2ascii|lpr");

    tn5250_print_session_main_loop(printsess);

    tn5250_print_session_destroy(printsess);
    tn5250_stream_destroy (stream);
    if (config!=NULL)
          tn5250_config_unref (config);
 #ifndef NDEBUG
    tn5250_log_close();
#endif 
    return 0;
}

static void syntax()
{
   printf("Usage:  lp5250d [options] host[:port]\n"
	  "Options:\n"
	  "\ttrace=FILE                 specify FULL path to log file\n"
	  "\tmap=NAME                   specify translation map\n"
	  "\tenv.DEVNAME=NAME           specify session name\n"
	  "\tenv.IBMMFRTYPMDL=NAME      specify host print transform name\n"
	  "\toutputcommand=CMD          specify the print output command\n"
	  "\t+/-version                 display version\n");

   exit (255);
}
