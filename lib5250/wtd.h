/* TN5250 - An implementation of the 5250 telnet protocol.
 * Copyright (C) 2000-2008 Jason M. Felice
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
#ifndef WTD_H
#define WTD_H

#ifdef __cplusplus
extern "C"
{
#endif

  struct _Tn5250Buffer;
  struct _Tn5250DBuffer;

/****s* lib5250/Tn5250WTDContext
 * NAME
 *    Tn5250WTDContext
 *
 * SYNOPSIS
 *    Tn5250Buffer buf;
 *    Tn5250WTDContext *ctx;
 *
 *    tn5250_buffer_init(&buf);
 *    ctx = tn5250_wtd_context_new (&buf, NULL, dst_dbuffer);
 *    tn5250_wtd_context_convert (ctx);
 *    tn5250_wtd_context_destroy (ctx);
 *
 *    tn5250_buffer_log(&buf);
 *
 * DESCRIPTION
 *    The WTD context object is used to calculate (and optimize) commands and
 *    WTD-type orders to transition from the source display buffer and format
 *    table to the destination display buffer and format table.
 *
 *    This will be used in our 5250 server implementation later.  Currently,
 *    we only accept NULL for the source display buffer and format table and
 *    create a full set of commands and orders which we send back to the host
 *    (and the host sends back to us) for the save/restore screen
 *    functionality.  Differential Write to Display orders and data should
 *    be implemented when the 5250 server is born.
 *
 * SOURCE
 */
  struct _Tn5250WTDContext
  {
    struct _Tn5250Buffer *buffer;

    struct _Tn5250DBuffer *src;
    struct _Tn5250DBuffer *dst;

    /* Our current position within the display. */
    int y, x;

    /* This is a sort of buffer for run-length-encoding the output data
     * characters using Repeat to Address orders. */
    int ra_count;
    unsigned char ra_char;

    /* A flag indicating that we have used a Clear Unit or a Clear Unit
     * Alternate command.  It's helpful in making assumptions about the state
     * of the display. */
    int clear_unit:1;
  };

  typedef struct _Tn5250WTDContext Tn5250WTDContext;
/*******/

  extern Tn5250WTDContext *tn5250_wtd_context_new (struct _Tn5250Buffer *buf,
						   struct _Tn5250DBuffer *sd,
						   struct _Tn5250DBuffer *dd);
  extern void tn5250_wtd_context_destroy (Tn5250WTDContext * This);
  extern void tn5250_wtd_context_convert (Tn5250WTDContext * This);
  extern void tn5250_wtd_context_set_ic (Tn5250WTDContext * This,
					 int y, int x);
#ifdef __cplusplus
}
#endif

#endif				/* WTD_H */

/* vi:set cindent sts=3 sw=3: */
