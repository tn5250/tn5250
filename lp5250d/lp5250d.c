/* TN5250 - An implementation of the 5250 telnet protocol.
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

#include "tn5250-private.h"

/* If getopt.h exists then getopt_long() probably does as well.  If
 * getopt.h doesn't exist (like on Solaris) then we probably need to use
 * plain old getopt().
 */
#ifdef HAVE_GETOPT_H
#define _GNU_SOURCE
#include <getopt.h>
#endif

static void syntax (void);

/* This just checks what arguments (options) were passed to lp5250d on the
 * command line.
 */
int check_options (int argc, char **argv, char **user, int *nodaemon);

/* This sets the current UID to the uid of the user passed.
 */
int change_user (char *user);

extern char *version_string;

Tn5250PrintSession *printsess = NULL;
Tn5250Stream *stream = NULL;
Tn5250Config *config = NULL;

int
main (int argc, char *argv[])
{
  char *user = NULL;
  int nodaemon = 0;

  if (check_options (argc, argv, &user, &nodaemon) != 0)
    {
      exit (1);
    }

  /* Change the UID if required before parsing the options */
  if (user != NULL)
    {
      if (change_user (user) != 0)
	{
	  exit (1);
	}
    }

  config = tn5250_config_new ();
  if (tn5250_config_load_default (config) == -1)
    {
      tn5250_config_unref (config);
      exit (1);
    }

  if (tn5250_config_parse_argv (config, argc, argv) == -1)
    {
      tn5250_config_unref (config);
      syntax ();
    }

  if (tn5250_config_get (config, "help"))
    {
      syntax ();
    }
  else if (tn5250_config_get (config, "version"))
    {
      printf ("tn5250 version %s\n", version_string);
      exit (0);
    }
  else if (!tn5250_config_get (config, "host"))
    {
      syntax ();
    }

  if (!nodaemon)
      nodaemon = tn5250_config_get_bool (config, "nodaemon");

  if (!nodaemon) 
      if (tn5250_daemon (0, 0, 0) < 0)
        {
          perror ("tn5250_daemon");
          exit (2);
        }

#ifndef NDEBUG
  if (tn5250_config_get (config, "trace"))
    {
      tn5250_log_open (tn5250_config_get (config, "trace"));
      TN5250_LOG (("lp5250d version %s, built on %s\n", version_string,
		   __DATE__));
      TN5250_LOG (("host = %s\n", tn5250_config_get (config, "host")));
    }
#endif

  openlog ("lp5250d", LOG_PID, LOG_DAEMON);

  stream = tn5250_stream_open (tn5250_config_get (config, "host"), config);
  if (stream == NULL)
    {
      syslog (LOG_INFO, "Couldn't connect to %s",
	      tn5250_config_get (config, "host"));
      exit (1);
    }

  printsess = tn5250_print_session_new ();
  tn5250_stream_setenv (stream, "TERM", "IBM-3812-1");
/*    tn5250_stream_setenv(stream, "DEVNAME", sessionname); */
  tn5250_stream_setenv (stream, "IBMFONT", "12");
  syslog (LOG_INFO, "DEVNAME = %s", tn5250_stream_getenv (stream, "DEVNAME"));
  if (tn5250_stream_getenv (stream, "IBMMFRTYPMDL"))
    {
      syslog (LOG_INFO, "TRANSFORM = %s",
	      tn5250_stream_getenv (stream, "IBMMFRTYPMDL"));
      tn5250_stream_setenv (stream, "IBMTRANSFORM", "1");
    }
  else
    {
      tn5250_stream_setenv (stream, "IBMTRANSFORM", "0");
    }
  tn5250_print_session_set_fd (printsess,
			       tn5250_stream_socket_handle (stream));
  tn5250_print_session_set_stream (printsess, stream);
  if (tn5250_config_get (config, "map"))
    {
      tn5250_print_session_set_char_map (printsess,
					 tn5250_config_get (config, "map"));
      tn5250_setenv ("TN5250_CCSIDMAP", tn5250_config_get (config, "map"), 0);
    }
  else
    {
      tn5250_print_session_set_char_map (printsess, "37");
      tn5250_setenv ("TN5250_CCSIDMAP", "37", 0);
    }
  if (tn5250_config_get (config, "outputcommand"))
    {
      tn5250_print_session_set_output_command (printsess,
					       tn5250_config_get (config,
								  "outputcommand"));
    }
  else
    {
      tn5250_print_session_set_output_command (printsess, "scs2ascii|lpr");
    }

  tn5250_print_session_main_loop (printsess);

  tn5250_print_session_destroy (printsess);
  tn5250_stream_destroy (stream);
  if (config != NULL)
    {
      tn5250_config_unref (config);
    }
#ifndef NDEBUG
  tn5250_log_close ();
#endif
  return 0;
}

static void
syntax ()
{
  printf ("Usage:  lp5250d [options] host[:port]\n"
	  "Options:\n"
	  "\ttrace=FILE                 specify FULL path to log file\n"
	  "\tmap=NAME                   specify translation map\n"
	  "\tenv.DEVNAME=NAME           specify session name\n"
	  "\tenv.IBMMFRTYPMDL=NAME      specify host print transform name\n"
	  "\toutputcommand=CMD          specify the print output command\n"
	  "\t-u,--user=NAME             display user to run as\n"
	  "\t-v,--version               display version\n"
	  "\t-N,--nodaemon              do not run as daemon (run in foreground)\n"
	  "\t-H,--help                  display this help\n");

  exit (255);
}


/* This checks the options lp5250d was passed.
 */
int
check_options (int argc, char **argv, char **user, int *nodaemon)
{
#ifdef HAVE_GETOPT_H
  struct option options[4];
#endif
  int i;
  extern char *optarg;

#ifdef HAVE_GETOPT_H
  options[0].name = "user";
  options[0].has_arg = required_argument;
  options[0].flag = NULL;
  options[0].val = 'u';
  options[1].name = "version";
  options[1].has_arg = no_argument;
  options[1].flag = NULL;
  options[1].val = 'v';
  options[2].name = "help";
  options[2].has_arg = no_argument;
  options[2].flag = NULL;
  options[2].val = 'H';
  options[3].name = "nodaemon";
  options[3].has_arg = no_argument;
  options[3].flag = NULL;
  options[3].val = 'N';
  options[4].name = 0;
  options[4].has_arg = 0;
  options[4].flag = 0;
  options[4].val = 0;
#endif

#ifdef HAVE_GETOPT_H
  while ((i = getopt_long (argc, argv, "u:vHN", options, NULL)) != -1)
#else
  while ((i = getopt (argc, argv, "u:vHN")) != -1)
#endif
    {
      switch (i)
	{
	case '?':
	  syntax ();
	  return -1;
	case 'u':
	  *user = optarg;
	  break;
	case 'H':
	  syntax ();
	  return -1;
	case 'v':
	  printf ("lp5250d version:  %s\n", VERSION);
	  return -1;
	case 'N':
	  *nodaemon=1;
          break;
	default:
	  syntax ();
	  return -1;
	}
    }
  return 0;
}


/* Set the UID to run lp5250d as.
 */
int
change_user (char *user)
{
  struct passwd *pwent;

  errno = 0;
  pwent = getpwnam (user);

  if (pwent == NULL)
    {
      printf ("Unable to set UID to user %s\n", user);
      perror ("getpwnam()");
      return (-1);
    }

  if (setuid (pwent->pw_uid) != 0)
    {
      printf ("Unable to set UID to user %s\n", user);
      perror ("setuid()");
      return (-1);
    }

  return (0);
}
