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
#ifndef UTILITY_H
#define UTILITY_H

#ifdef __cplusplus
extern "C" {
#endif

#if SIZEOF_SHORT == 2
   typedef unsigned short Tn5250Uint16;
   typedef signed short Tn5250Sint16;
#elif SIZEOF_INT == 2
   typedef unsigned int Tn5250Uint16;
   typedef signed int Tn5250Sint16;
#else
   ACK!  Need a 16-bit type!
#endif

/* Idea shamelessly stolen from GTK+ */
#define tn5250_new(type,count) (type *)malloc (sizeof (type) * (count))

   unsigned char tn5250_ascii2ebcdic(unsigned char ascii);
   unsigned char tn5250_ebcdic2ascii(unsigned char ebcdic);
   void tn5250_settransmap(char *map);
   int tn5250_printable(unsigned char data);
   int tn5250_attribute(unsigned char data);
   int tn5250_isnumeric(char data);

#define TN5250_MAKESTRING(expr) #expr
#ifndef NDEBUG
   void tn5250_log_open(const char *fname);
   void tn5250_log_printf(const char *fmt,...);
   void tn5250_log_close(void);
   void tn5250_log_assert(int val, char const *expr, char const *file, int line);
#define TN5250_LOG(args) tn5250_log_printf args
#define TN5250_ASSERT(expr) \
   tn5250_log_assert((expr), TN5250_MAKESTRING(expr), __FILE__, __LINE__)

extern FILE * tn5250_logfile;
#else
#define TN5250_LOG(args)
#endif

#ifdef __cplusplus
}

#endif
#endif				/* UTILITY_H */
