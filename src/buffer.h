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
#ifndef BUFFER_H
#define BUFFER_H

#ifdef __cplusplus
extern "C" {
#endif

   struct _Tn5250Buffer {
      unsigned char /*@null@*/ *data;
      int len;
      int allocated;
   };

   typedef /*@abstract@*/ /*@immutable@*/ struct _Tn5250Buffer Tn5250Buffer;

/* These don't work like new/destroy */
   extern void tn5250_buffer_init( /*@out@*/ Tn5250Buffer * This);
   extern void tn5250_buffer_free(Tn5250Buffer * This);

#define tn5250_buffer_data(This) \
	((This)->data ? (This)->data : (unsigned char *)"")
#define tn5250_buffer_length(This) ((This)->len)

   extern void tn5250_buffer_append_byte(Tn5250Buffer * This, unsigned char b);
   extern void tn5250_buffer_append_data(Tn5250Buffer * This, unsigned char *data, int len);

#ifdef __cplusplus
}

#endif
#endif				/* BUFFER_H */
