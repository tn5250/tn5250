/* tn5250 -- an implentation of the 5250 telnet protocol.
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
#include "tn5250-config.h"

#ifdef WIN32
#include <windows.h>
#endif

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "utility.h"

static unsigned char const /*@null@*/ *ebcdicmap = NULL;
static unsigned char const /*@null@*/ *asciimap = NULL;

struct _Tn5250TransMap {
   char *mapname;
   unsigned char const *ebcdicmap;
   unsigned char const *asciimap;
};

typedef struct _Tn5250TransMap Tn5250TransMap;

#include "transmaps.h"

/****f* lib5250/tn5250_ascii2ebcdic
 * NAME
 *    tn5250_ascii2ebcdic
 * SYNOPSIS
 *    ret = tn5250_ascii2ebcdic (ascii);
 * INPUTS
 *    unsigned char        ascii      - The ASCII character to translate.
 * DESCRIPTION
 *    Translate the specified character from ASCII to EBCDIC.
 *****/
unsigned char tn5250_ascii2ebcdic(unsigned char ascii)
{
   if (asciimap == NULL)
      tn5250_settransmap("en");
   return (asciimap[ascii]);
}

/****f* lib5250/tn5250_ebcdic2ascii
 * NAME
 *    tn5250_ebcdic2ascii
 * SYNOPSIS
 *    ret = tn5250_ebcdic2ascii (ebcdic);
 * INPUTS
 *    unsigned char        ebcdic     - The EBCDIC character to translate.
 * DESCRIPTION
 *    Translate the specified character from EBCDIC to ASCII.
 *****/
unsigned char tn5250_ebcdic2ascii(unsigned char ebcdic)
{
   if (ebcdicmap == NULL)
      tn5250_settransmap ("en");
   switch (ebcdic) {
   case 0x1C:
      return '*';
   case 0:
      return ' ';
   default:
      return ebcdicmap[ebcdic];
   }
}

/****f* lib5250/tn5250_settransmap
 * NAME
 *    tn5250_settransmap
 * SYNOPSIS
 *    tn5250_settransmap (map);
 * INPUTS
 *    char *               map        - Name of the character translation map.
 * DESCRIPTION
 *    Set the current (program-wide) translation map.
 * BUGS
 *    The translation map is system-wide, and not associated with a display or
 *    session.
 *****/
void tn5250_settransmap(char *map)
{
   Tn5250TransMap *t;
   for (t = transmaps; t->mapname; t++) {
      if (strcmp(t->mapname, map) == 0) {
	 asciimap = t->asciimap;
	 ebcdicmap = t->ebcdicmap;
	 break;
      }
   }
   if (!t->mapname) {
#ifndef WIN32
      printf("Invalid mapname: %s\n"
	     "Try one out of:\n ", map);
      for (t = transmaps; t->mapname; t++) {
	 printf("%s ", t->mapname);
      }
      printf("\n");
#else
      char msg[1024] = "Invalid map name, try one of:\r\n";
      for (t = transmaps; t->mapname; t++) {
	 strcat(msg, t->mapname);
	 if ((t + 1)->mapname)
	    strcat(msg, ", ");
      }
      MessageBox(NULL, msg, "tn5250", MB_ICONEXCLAMATION);
#endif
      exit(1);
   }
   /* ebcdicmap[0] = ' '; */
}

/****f* lib5250/tn5250_printable
 * NAME
 *    tn5250_printable
 * SYNOPSIS
 *    ret = tn5250_printable (data);
 * INPUTS
 *    unsigned char        data       - the character to test.
 * DESCRIPTION
 *    Determines whether the specified character is printable, which means 
 *    either it is a displayable EBCDIC character, an ideographic control
 *    character, a NUL, or a few other odds and ends.
 * SOURCE
 */
int tn5250_printable(unsigned char data)
{
   switch (data) {
   case 0x00:
   case 0x0d: /* ? - appears in data submitted by Sean Porterfield */
   case 0x0a: /* ? - ditto */
   case 0x16: /* ? - ditto */
   case 0x1c: /* DUP */
   case 0x1e: /* Field Mark (?) */
      return 1;

   case 0x0e: /* Ideographic Shift-In. */
   case 0x0f: /* Ideographic Shift-Out. */
      TN5250_ASSERT(0); /* FIXME: Not implemented. */
      break;
   }
   return (data >= 0x1C) && (data <= 0xFF);
}
/*******/

/****f* lib5250/tn5250_attribute
 * NAME
 *    tn5250_attribute
 * SYNOPSIS
 *    ret = tn5250_attribute (data);
 * INPUTS
 *    unsigned char        data       - the character to test.
 * DESCRIPTION
 *    Determines whether the character is a 5250 attribute.
 *****/
int tn5250_attribute(unsigned char data)
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

