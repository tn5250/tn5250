/*
 * Copyright (C) 2000 Michael Madore
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

#include "tn5250-config.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <syslog.h>
#include <errno.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#include <stdlib.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/wait.h>

#include "tn5250.h"

#define TCP_PORT 8888

static void syntax(void);
static int parse_options(int argc, char *argv[]);
void closeall(int fd);
int daemon(int nochdir, int noclose);

/* closeall() -- close all FDs >= a specified value */

void closeall(int fd)
{
    int fdlimit = sysconf(_SC_OPEN_MAX);

    while (fd < fdlimit)
      close(fd++);
}

/* daemon() - detach process from user and disappear into the background
 * returns -1 on failure, but you can't do much except exit in that case
 * since we may already have forked. This is based on the BSD version,
 * so the caller is responsible for things like the umask, etc.
 */

/* believed to work on all Posix systems */

int daemon(int nochdir, int noclose)
{
    switch (fork())
    {
        case 0:  break;
        case -1: return -1;
        default: _exit(0);          /* exit the original process */
    }

    if (setsid() < 0)               /* shoudn't fail */
      return -1;

    /* dyke out this switch if you want to acquire a control tty in */
    /* the future -- not normally advisable for daemons */

    switch (fork())
    {
        case 0:  break;
        case -1: return -1;
        default: _exit(0);
    }

    if (!nochdir)
      chdir("/");

    if (!noclose)
    {
        closeall(0);
        open("/dev/null",O_RDWR);
        dup(0); dup(0);
    }

    umask(0);

    return 0;
}

extern char *version_string;

Tn5250PrintSession *printsess = NULL;
Tn5250Stream *stream = NULL;

char remotehost[20];
char sessionname[20];
char transformname[20];
char outputcommand[30];
char *mapname = NULL;

int main(int argc, char *argv[])
{
   
   openlog("lp5250d", LOG_PID, LOG_DAEMON);

   if (parse_options(argc, argv) < 0)
      syntax();

    if (daemon(0,0) < 0)
    {
        perror("daemon");
        exit(2);
    }
    

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
    if (transformname != NULL) {
       syslog(LOG_INFO, "TRANSFORM = %s", transformname);
       tn5250_stream_setenv(stream, "IBMTRANSFORM", "1");
       tn5250_stream_setenv(stream, "IBMMFRTYPMDL", transformname);
    } else
       tn5250_stream_setenv(stream, "IBMTRANSFORM", "0");
    tn5250_print_session_set_fd(printsess, tn5250_stream_socket_handle(stream));
    tn5250_print_session_set_stream(printsess, stream);
    if (mapname == NULL)
      mapname = "en";
    tn5250_print_session_set_char_map(printsess, mapname);
    tn5250_print_session_set_output_command(printsess, outputcommand);
    tn5250_print_session_main_loop(printsess);

    tn5250_print_session_destroy(printsess);
    tn5250_stream_destroy (stream);

    return 0;
}

static int parse_options(int argc, char *argv[])
{
   int arg;
   
   while ((arg = getopt(argc, argv, "m:s:T:P:Vwy:")) != EOF) {
      switch (arg) {
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
   struct valid_term *p;
   printf("Usage:  tn5250 [options] host[:port]\n"
	  "Options:\n"
	  "\t-m map      specify translation map\n"
	  "\t-s name     specify session name\n"
	  "\t-T name     specify host print transform\n"
	  "\t-P cmd      specify the print output command\n"
	  "\t-V          display version\n");

   exit (255);
}
