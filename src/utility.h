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
 * As a special exception, the Free Software Foundation gives permission
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

typedef unsigned char Tn5250Char;

/****s* lib5250/Tn5250CharMap
 * NAME
 *    Tn5250CharMap
 * SYNOPSIS
 *    Tn5250CharMap *map = tn5250_char_map_new ("37");
 *    ac = tn5250_char_map_to_local(map,ec);
 *    ec = tn5250_char_map_to_remote(map,ac);
 *    if (tn5250_char_map_printable_p (map,ec))
 *	 ;
 *    if (tn5250_char_map_attribute_p (map,ec))
 *	 ;
 *    tn5250_char_map_destroy(map);
 * DESCRIPTION
 *    The Tn5250CharMap object embodies a translation map which can be
 *    used to translate characters from various flavours of ASCII and
 *    ISO character sets to various flavours of EBCDIC characters sets
 *    (and back).
 * SOURCE
 */
struct _Tn5250CharMap {
   const char *name;
   const unsigned char *to_remote_map;
   const unsigned char *to_local_map;
};

typedef struct _Tn5250CharMap Tn5250CharMap;

extern Tn5250CharMap tn5250_transmaps[];
/*******/

Tn5250CharMap *tn5250_char_map_new(const char *maping);
void tn5250_char_map_destroy(Tn5250CharMap *This);

void tn5250_closeall(int fd);
int tn5250_daemon(int nochdir, int noclose, int ignsigcld);

Tn5250Char tn5250_char_map_to_remote(Tn5250CharMap *This, Tn5250Char ascii);
Tn5250Char tn5250_char_map_to_local(Tn5250CharMap *This, Tn5250Char ebcdic);

int tn5250_char_map_printable_p(Tn5250CharMap *This, Tn5250Char data);
int tn5250_char_map_attribute_p(Tn5250CharMap *This, Tn5250Char data);

/* Idea shamelessly stolen from GTK+ */
#define tn5250_new(type,count) (type *)malloc (sizeof (type) * (count))

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

/* vi:set sts=3 sw=3: */
