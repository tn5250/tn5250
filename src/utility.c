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
#include "transmaps.h"

/****f* lp5250d/tn5250_closeall
 * NAME
 *    tn5250_closeall
 * SYNOPSIS
 *    tn5250_closeall (fd);
 * INPUTS
 *    int fd	- The starting file descriptor.
 * DESCRIPTION
 *    Closes all file descriptors >= a specified value.
 *****/
void tn5250_closeall(int fd)
{
    int fdlimit = sysconf(_SC_OPEN_MAX);

    while (fd < fdlimit)
      close(fd++);
}

/*
  Signal handler for SIGCHLD.  We use waitpid instead of wait, since there
  is no way to tell wait not to block if there are still non-terminated child
  processes.
*/
void
sig_child(int signum)
{
  int pid;
  int status;

  while( pid = waitpid(-1, &status, WNOHANG) > 0);

  return;
}

/****f* lp5250d/tn5250_daemon
 * NAME
 *    tn5250_daemon
 * SYNOPSIS
 *    ret = tn5250_daemon (nochdir, noclose);
 * INPUTS
 *    int nochdir	- 0 to perform chdir.
 *    int noclose	- 0 to close all file handles.
 * DESCRIPTION
 *    Detach process from user and disappear into the background
 *    returns -1 on failure, but you can't do much except exit in that 
 *    case since we may already have forked. Believed to work on all 
 *    Posix systems.
 *****/
int tn5250_daemon(int nochdir, int noclose, int ignsigcld)
{
  struct sigaction sa;

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
        tn5250_closeall(0);
        open("/dev/null",O_RDWR);
        dup(0); dup(0);
    }

    umask(0);


    if(ignsigcld) {
      sa.sa_handler = sig_child;
      sigemptyset(&sa.sa_mask);
      sa.sa_flags = SA_RESTART;

#ifdef SIGCHLD
      sigaction(SIGCHLD, &sa, NULL);
#else
#ifdef SIGCLD
      sigaction(SIGCLD, &sa, NULL);
#endif
#endif
    }

    return 0;
}

int 
tn5250_make_socket (unsigned short int port)
{
  int sock;
  int on = 1;
  struct sockaddr_in name;
  u_long ioctlarg = 0;

  /* Create the socket. */
  sock = socket (PF_INET, SOCK_STREAM, 0);
  if (sock < 0)
    {
      syslog(LOG_INFO, "socket: %s\n", strerror(errno));
      exit (EXIT_FAILURE);
    }

  /* Give the socket a name. */
  name.sin_family = AF_INET;
  name.sin_port = htons (port);
  name.sin_addr.s_addr = htonl (INADDR_ANY);
  setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));
  TN_IOCTL(sock, FIONBIO, &ioctlarg);
  if (bind (sock, (struct sockaddr *) &name, sizeof (name)) < 0)
    {
      syslog(LOG_INFO, "bind: %s\n", strerror(errno));
      exit (EXIT_FAILURE);
    }

  return sock;
}


/****f* lib5250/tn5250_char_map_to_remote
 * NAME
 *    tn5250_char_map_to_remote
 * SYNOPSIS
 *    ret = tn5250_char_map_to_remote (map,ascii);
 * INPUTS
 *    Tn5250Char           ascii      - The local character to translate.
 * DESCRIPTION
 *    Translate the specified character from local to remote.
 *****/
Tn5250Char tn5250_char_map_to_remote(Tn5250CharMap *map, Tn5250Char ascii)
{
   return map->to_remote_map[ascii];
}

/****f* lib5250/tn5250_char_map_to_local
 * NAME
 *    tn5250_char_map_to_local
 * SYNOPSIS
 *    local = tn5250_char_map_to_local (map, ebcdic);
 * INPUTS
 *    Tn5250Char           ebcdic     - The remote character to translate.
 * DESCRIPTION
 *    Translate the specified character from remote character to local.
 *****/
Tn5250Char tn5250_char_map_to_local(Tn5250CharMap *map, Tn5250Char ebcdic)
{
   switch (ebcdic) {
   case 0x1C:
      return '*'; /* This should be an overstriken asterisk (DUP) */
   case 0:
      return ' ';
   default:
      return map->to_local_map[ebcdic];
   }
}

/****f* lib5250/tn5250_char_map_new
 * NAME
 *    tn5250_char_map_new
 * SYNOPSIS
 *    cmap = tn5250_char_map_new ("37");
 * INPUTS
 *    const char *         map        - Name of the character translation map.
 * DESCRIPTION
 *    Create a new translation map.
 * NOTES
 *    Translation maps are currently statically allocated, although you should
 *    call tn5250_char_map_destroy (a no-op) for future compatibility.
 *****/
Tn5250CharMap *tn5250_char_map_new (const char *map)
{
   Tn5250CharMap *t;
   for (t = tn5250_transmaps; t->name; t++) {
      if (strcmp(t->name, map) == 0)
	 return t;
   }
   return NULL;
}

/****f* lib5250/tn5250_char_map_destroy
 * NAME
 *    tn5250_char_map_destroy
 * SYNOPSIS
 *    tn5250_char_map_destroy (map);
 * INPUTS
 *    Tn5250CharMap *         map     - The character map to destroy.
 * DESCRIPTION
 *    Frees the character map's resources.
 *****/
void tn5250_char_map_destroy (Tn5250CharMap *map)
{
   /* NOOP */
}

/****f* lib5250/tn5250_char_map_printable_p
 * NAME
 *    tn5250_char_map_printable_p
 * SYNOPSIS
 *    if (tn5250_char_map_printable_p (map,ec))
 *	 ;
 * INPUTS
 *    Tn5250CharMap *      map        - the character map to use.
 *    Tn5250Char           ec         - the character to test.
 * DESCRIPTION
 *    Determines whether the specified character is printable, which means 
 *    either it is a displayable EBCDIC character, an ideographic control
 *    character, a NUL, or a few other odds and ends.
 * SOURCE
 */
int tn5250_char_map_printable_p(Tn5250CharMap *map, Tn5250Char data)
{
   switch (data) {
   case 0x0e: /* Ideographic Shift-In. */
   case 0x0f: /* Ideographic Shift-Out. */
      TN5250_ASSERT(0); /* FIXME: Not implemented. */
      return 0;
   case 0x01:  /* These chars "cause an RNQ1251" on an IBM 3179-2 */
   case 0x02:  
   case 0x03:
   case 0x04:
   case 0x10:
   case 0x11:
   case 0x12:
   case 0x13:
   case 0x14:
   case 0x15:
   case 0x1d:
      return 0;                                                               
   }
   return 1;
}
/*******/

/****f* lib5250/tn5250_char_map_attribute_p
 * NAME
 *    tn5250_char_map_attribute_p
 * SYNOPSIS
 *    ret = tn5250_char_map_attribute_p (map,ec);
 * INPUTS
 *    Tn5250CharMap *      map        - the translation map to use.
 *    Tn5250Char           ec         - the character to test.
 * DESCRIPTION
 *    Determines whether the character is a 5250 attribute.
 *****/
int tn5250_char_map_attribute_p(Tn5250CharMap *map, Tn5250Char data)
{
   return ((data & 0xE0) == 0x20);
}

#ifndef NDEBUG
FILE *tn5250_logfile = NULL;

/****f* lib5250/tn5250_log_open
 * NAME
 *    tn5250_log_open
 * SYNOPSIS
 *    tn5250_log_open (fname);
 * INPUTS
 *    const char *         fname      - Filename of tracefile.
 * DESCRIPTION
 *    Opens the debug tracefile for this session.
 *****/
void tn5250_log_open(const char *fname)
{
   if (tn5250_logfile != NULL)
      fclose(tn5250_logfile);
   tn5250_logfile = fopen(fname, "w");
   if (tn5250_logfile == NULL) {
      perror(fname);
      exit(1);
   }
   /* FIXME: Write $TERM, version, and uname -a to the file. */
#ifndef WIN32
   /* Set file mode to 0600 since it may contain passwords. */
   fchmod(fileno(tn5250_logfile), 0600);
#endif
   setbuf(tn5250_logfile, NULL);
}

/****f* lib5250/tn5250_log_close
 * NAME
 *    tn5250_log_close
 * SYNOPSIS
 *    tn5250_log_close ();
 * INPUTS
 *    None
 * DESCRIPTION
 *    Close the current tracefile if one is open.
 *****/
void tn5250_log_close()
{
   if (tn5250_logfile != NULL) {
      fclose(tn5250_logfile);
      tn5250_logfile = NULL;
   }
}

/****f* lib5250/tn5250_log_printf
 * NAME
 *    tn5250_log_printf
 * SYNOPSIS
 *    tn5250_log_printf (fmt, );
 * INPUTS
 *    const char *         fmt        - 
 * DESCRIPTION
 *    This is an internal function called by the TN5250_LOG() macro.  Use
 *    the macro instead, since it can be conditionally compiled.
 *****/
void tn5250_log_printf(const char *fmt,...)
{
   va_list vl;
   if (tn5250_logfile != NULL) {
      va_start(vl, fmt);
      vfprintf(tn5250_logfile, fmt, vl);
      va_end(vl);
   }
}

/****f* lib5250/tn5250_log_assert
 * NAME
 *    tn5250_log_assert
 * SYNOPSIS
 *    tn5250_log_assert (val, expr, file, line);
 * INPUTS
 *    int                  val        - 
 *    char const *         expr       - 
 *    char const *         file       - 
 *    int                  line       - 
 * DESCRIPTION
 *    This is an internal function called by the TN5250_ASSERT() macro.  Use
 *    the macro instead, since it can be conditionally compiled.
 *****/
void tn5250_log_assert(int val, char const *expr, char const *file, int line)
{
   if (!val) {
      tn5250_log_printf("\nAssertion %s failed at %s, line %d.\n", expr, file, line);
      fprintf (stderr,"\nAssertion %s failed at %s, line %d.\n", expr, file, line);
      abort ();
   }
}

#endif				/* NDEBUG */

