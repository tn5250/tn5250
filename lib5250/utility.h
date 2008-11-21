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

struct _clientaddr {
  unsigned long int address;
  unsigned long int mask;
};  

typedef struct _clientaddr clientaddr;

extern Tn5250CharMap tn5250_transmaps[];
/*******/

Tn5250CharMap *tn5250_char_map_new(const char *maping);
void tn5250_char_map_destroy(Tn5250CharMap *This);

void tn5250_closeall(int fd);
int tn5250_daemon(int nochdir, int noclose, int ignsigcld);
int tn5250_make_socket(unsigned short int port);

Tn5250Char tn5250_char_map_to_remote(Tn5250CharMap *This, Tn5250Char ascii);
Tn5250Char tn5250_char_map_to_local(Tn5250CharMap *This, Tn5250Char ebcdic);

int tn5250_char_map_printable_p(Tn5250CharMap *This, Tn5250Char data);
int tn5250_char_map_attribute_p(Tn5250CharMap *This, Tn5250Char data);
int tn5250_setenv(const char *name, const char *value, int overwrite);

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
#define TN5250_ASSERT(expr)
#endif

#include "conf.h"

int tn5250_parse_color(Tn5250Config *config, const char *colorname, 
                        int *r, int *g, int *b);
int tn5250_run_cmd(const char *cmd, int wait);

#ifdef __cplusplus
}
#endif

#endif				/* UTILITY_H */

/* vi:set sts=3 sw=3: */
