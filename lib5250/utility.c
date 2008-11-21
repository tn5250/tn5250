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
#include "transmaps.h"

static char mapfix[256];
static char mapfix2[256];
static char mapfix3[256];
static char mapfix4[256];

#ifndef WIN32

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

#endif  /* ifndef WIN32 */

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
#ifndef WIN32
      syslog(LOG_INFO, "socket: %s\n", strerror(errno));
#endif
      exit (EXIT_FAILURE);
    }

  /* Give the socket a name. */
  name.sin_family = AF_INET;
  name.sin_port = htons (port);
  name.sin_addr.s_addr = htonl (INADDR_ANY);
  setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (const char *)&on, sizeof(on));
  TN_IOCTL(sock, FIONBIO, &ioctlarg);
  if (bind (sock, (struct sockaddr *) &name, sizeof (name)) < 0)
    {
#ifndef WIN32
      syslog(LOG_INFO, "bind: %s\n", strerror(errno));
#endif
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

/* XXX: HACK: These characters were reported wrong in transmaps.h.
        Since that's a generated file, I'm overriding them here -SCK */

   TN5250_LOG(("tn5250_char_map_new: map = \"%s\"\n", map));

   if (!strcmp(map, "870") || !strcmp(map, "win870")) {

      TN5250_LOG(("tn5250_char_map_new: Installing 870 workaround\n"));

      memcpy(mapfix, windows_1250_to_ibm870, sizeof(mapfix));
      memcpy(mapfix2, ibm870_to_windows_1250, sizeof(mapfix2));
      memcpy(mapfix3, iso_8859_2_to_ibm870, sizeof(mapfix3));
      memcpy(mapfix4, ibm870_to_iso_8859_2, sizeof(mapfix4));

      mapfix[142] = 184;
      mapfix[143] = 185;
      mapfix[158] = 182;
      mapfix[159] = 183;
      mapfix[163] = 186;
      mapfix[202] = 114;
      mapfix[234] = 82;

      mapfix2[82] = 234;
      mapfix2[114] = 202;
      mapfix2[182] = 158;
      mapfix2[183] = 159;
      mapfix2[184] = 142;
      mapfix2[185] = 143;
      mapfix2[186] = 163;

      mapfix3[163] = 186;
      mapfix3[172] = 185;
      mapfix3[188] = 183;
      mapfix3[202] = 114;
      mapfix3[234] = 82;

      mapfix4[82] = 234;
      mapfix4[114] = 202;
      mapfix4[183] = 188;
      mapfix4[185] = 172;
      mapfix4[186] = 163;

      for (t = tn5250_transmaps;  t->name; t++) {
          if (!strcmp(t->name, "win870")) {
               t->to_remote_map = (const char *)mapfix;
               t->to_local_map = (const char *)mapfix2;
               TN5250_LOG(("Workaround installed for map \"win870\"\n"));
          }
          else if (!strcmp(t->name, "870")) {
               t->to_remote_map = (const char *)mapfix3;
               t->to_local_map = (const char *)mapfix4;
               TN5250_LOG(("Workaround installed for map \"870\"\n"));
          }
      }
   }
   
   /* Under Windows, we'll try the "winXXX" maps first, then fall back 
      to the standard (unix) versions */
#ifdef WIN32
   {
      char winmap[10];
      _snprintf(winmap, sizeof(winmap)-1, "win%s", map);
      for (t = tn5250_transmaps; t->name; t++) {
         if (strcmp(t->name, winmap) == 0) {
              TN5250_LOG(("Using map %s\n", t->name));
       	      return t;
         }
      }
   }
#endif

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
   switch (data) 
     {
       /* 
	  Ideographic Shift-In and Shift-Out. 
	  case 0x0e: 
	  case 0x0f: 
       */
     default:
       break;
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


/****f* lib5250/tn5250_parse_color
 * NAME
 *    tn5250_parse_color
 * SYNOPSIS
 *    tn5250_parse_color (config, "green", &red, &green, &blue);
 * INPUTS
 *    Tn5250Config *       config     -
 *    const char   *       colorname  -
 *    int          *       red        - 
 *    int          *       green      - 
 *    int          *       blue       - 
 * DESCRIPTION
 *    This loads a color from the TN5250 config object, and then
 *    parses it into it's red, green, blue components.
 *****/
int tn5250_parse_color(Tn5250Config *config, const char *colorname, 
                        int *red, int *green, int *blue) {

    const char *p;
    char colorspec[16];
    int r, g, b;

    if ((p=tn5250_config_get(config, colorname)) == NULL) {
        return -1;
    }

    strncpy(colorspec, p, sizeof(colorspec));
    colorspec[sizeof(colorspec)-1] = '\0';

    if (*colorspec != '#') {
          if (!strcasecmp(colorspec, "white")) {
               r = 255; g = 255; b = 255;
          } else if (!strcasecmp(colorspec, "yellow")) {
               r = 255; g = 255; b = 0;
          } else if (!strcasecmp(colorspec, "lightmagenta")) {
               r = 255; g = 0;   b = 255;
          } else if (!strcasecmp(colorspec, "lightred")) {
               r = 255; g = 0;   b = 0;
          } else if (!strcasecmp(colorspec, "lightcyan")) {
               r = 0;   g = 255; b = 255;
          } else if (!strcasecmp(colorspec, "lightgreen")) {
               r = 0;   g = 255; b = 0;
          } else if (!strcasecmp(colorspec, "lightblue")) {
               r = 0;   g = 0;   b = 255;
          } else if (!strcasecmp(colorspec, "lightgray")) {
               r = 192; g = 192; b = 192;
          } else if (!strcasecmp(colorspec, "gray")) {
               r = 128; g = 128; b = 128;
          } else if (!strcasecmp(colorspec, "brown")) {
               r = 128; g = 128; b = 0;
          } else if (!strcasecmp(colorspec, "red")) {
               r = 128; g = 0;   b = 0;
          } else if (!strcasecmp(colorspec, "cyan")) {
               r = 0;   g = 128; b = 128;
          } else if (!strcasecmp(colorspec, "green")) {
               r = 0;   g = 128; b = 0;
          } else if (!strcasecmp(colorspec, "blue")) {
               r = 0;   g = 0;   b = 128;
          } else if (!strcasecmp(colorspec, "black")) {
               r = 0;   g = 0;   b = 0;
          }
    }
    else {
         if (strlen(colorspec) != 7)
              return -1;
         if (sscanf(&colorspec[1], "%02x%02x%02x", &r, &g, &b)!=3) 
              return -1;
    }

    *red = r;
    *green = g;
    *blue = b;
    return 0;
}


/****f* lib5250/tn5250_setenv
 * NAME
 *    tn5250_setenv
 * SYNOPSIS
 *    tn5250_setenv ("TN5250_CCSID", "37", 1);
 * INPUTS
 *    const char   *       name       -
 *    const char   *       value      - 
 *    int                  overwrite  - 
 * DESCRIPTION
 *    This works just like setenv(3), but setenv(3) doesn't
 *    exist on all systems. 
 *****/
int tn5250_setenv(const char *name, const char *value, int overwrite) {

     char *strval;
     int ret;

     if (!overwrite) 
         if (getenv(name)!=NULL) return 0;

     strval = malloc(strlen(name)+strlen(value)+2);
     TN5250_ASSERT(strval!=NULL);

     strcpy(strval, name);
     strcat(strval, "=");
     strcat(strval, value);

     ret = putenv(strval);

     /* free(strval)   on some systems, it continues to use our memory,
                       so we should not free it...                    */

     return ret;
}

/****f* lib5250/tn5250_run_cmd
 * NAME
 *    tn5250_run_cmd
 * SYNOPSIS
 *    tn5250_run_cmd ("notepad", 0);
 * INPUTS
 *    const char   *       cmd        -
 *    int                  wait       - 
 * DESCRIPTION
 *    Run a command (submitted to us by the AS/400)
 *****/
#ifndef WIN32
int tn5250_run_cmd(const char *cmd, int wait) {

    struct sigaction sa;
    int sig;
    int cpid;

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

    switch ( cpid = fork() ) {
        case -1:
           return -1;

        case 0:
           system(cmd);
           _exit(0);
    }

    if (wait) 
      waitpid(cpid, NULL, 0);

    return 0;
}
#endif

#ifdef WIN32
int win32_run_process(const char *cmd, int wait);

int tn5250_run_cmd(const char *cmd, int wait) {

  char *tempcmd;
  char *prefix = "cmd.exe /c ";
  int rc;

  rc = win32_run_process(cmd, wait);

  if (rc == -1) {

     tempcmd = malloc(strlen(prefix) + strlen(cmd) + 1);
     strcpy(tempcmd, prefix);
     strcat(tempcmd, cmd);

     rc = win32_run_process(tempcmd, wait);
     free(tempcmd);
  }

  return rc;
}

int win32_run_process(const char *cmd, int wait) {
    
  STARTUPINFO si;
  PROCESS_INFORMATION pi;

  ZeroMemory(&si, sizeof(si));
  ZeroMemory(&pi, sizeof(si));
  si.cb = sizeof(si);

  if ( !CreateProcess( NULL,            /* no module name */
                       (LPTSTR)cmd,     /* command line */
                       NULL,            /* don't inherit process bandle */
                       NULL,            /* don't inherit thread handle */
                       FALSE,           /* no inheritance, by golly! */
                       0,               /* flags */
                       NULL,            /* use parent's environment */
                       NULL,            /* use parent's directory */
                       &si,             /* STARTUPINFO */
                       &pi )) {         /* PROCESS_INFORMATION */
       return -1;
  }

  if (wait)
     WaitForSingleObject( pi.hProcess, INFINITE );

  CloseHandle(pi.hProcess);
  CloseHandle(pi.hThread);
  return 0;
}
    
#endif /* ifdef WIN32 */
