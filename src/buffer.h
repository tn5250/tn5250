/* TN5250
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
#ifndef BUFFER_H
#define BUFFER_H

#ifdef __cplusplus
extern "C" {
#endif

/****s* lib5250/Tn5250Buffer
 * NAME
 *    Tn5250Buffer
 * SYNOPSIS
 *    Tn5250Buffer buf;
 *    tn5250_buffer_init(&buf);
 *    tn5250_buffer_append_byte(&buf, '\n');
 *    tn5250_buffer_append_data(&buf, data, len);
 *    fwrite(file,tn5250_buffer_data(&buf),1,tn5250_buffer_length(&buf));
 *    tn5250_buffer_free(&buf);
 * DESCRIPTION
 *    A dynamically-sizing buffer of bytes.
 * SOURCE
 */
struct _Tn5250Buffer {
   unsigned char /*@null@*/ *data;
   int len;
   int allocated;
};

typedef /*@abstract@*/ /*@immutable@*/ struct _Tn5250Buffer Tn5250Buffer;
/*******/

/* These don't work like new/destroy */
extern void tn5250_buffer_init( /*@out@*/ Tn5250Buffer * This);
extern void tn5250_buffer_free(Tn5250Buffer * This);

#define tn5250_buffer_data(This) \
	((This)->data ? (This)->data : (unsigned char *)"")
#define tn5250_buffer_length(This) ((This)->len)

extern void tn5250_buffer_append_byte(Tn5250Buffer * This, unsigned char b);
extern void tn5250_buffer_append_data(Tn5250Buffer * This, unsigned char *data, int len);
extern void tn5250_buffer_log (Tn5250Buffer *This, const char *prefix);

#ifdef __cplusplus
}

#endif
#endif				/* BUFFER_H */

/* vi:set sts=3 sw=3 autoindent: */
