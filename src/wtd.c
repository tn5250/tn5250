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

#include "config.h"
#include <string.h>
#include <stdio.h>
#include <malloc.h>
#include "utility.h"
#include "dbuffer.h"
#include "field.h"
#include "formattable.h"
#include "display.h"
#include "terminal.h"
#include "buffer.h"
#include "record.h"
#include "stream.h"
#include "session.h"
#include "codes5250.h"
#include "wtd.h"

static void tn5250_wtd_context_putc (Tn5250WTDContext *This, unsigned char c);
static void tn5250_wtd_context_ra_putc (Tn5250WTDContext *This, unsigned char c);
static void tn5250_wtd_context_ra_flush (Tn5250WTDContext *This);
static void tn5250_wtd_context_write_field (Tn5250WTDContext *This, Tn5250Field *field, unsigned char attr);
static void tn5250_wtd_context_convert_nosrc (Tn5250WTDContext *This);
static Tn5250Field *tn5250_wtd_context_peek_field (Tn5250WTDContext *This);

/*
 *    Create a new instance of our WTD context object and initialize it.
 */
Tn5250WTDContext *tn5250_wtd_context_new (Tn5250Buffer *buffer, Tn5250DBuffer *sd, Tn5250Table *st, Tn5250DBuffer *dd, Tn5250Table *dt)
{
   Tn5250WTDContext *This;

   TN5250_ASSERT(dd != NULL);
   TN5250_ASSERT(dt != NULL);
   TN5250_ASSERT(buffer != NULL);

   if ((This = tn5250_new (Tn5250WTDContext, 1)) == NULL)
      return NULL;

   This->buffer = buffer;
   This->src_dbuffer = sd;
   This->src_table = st;
   This->dst_dbuffer = dd;
   This->dst_table = dt;
   This->ra_count = 0;
   This->ra_char = 0x00;
   This->clear_unit = 0;

   return This;
}

/*
 *    Destroy a WTD context object.
 */
void tn5250_wtd_context_destroy (Tn5250WTDContext *This)
{
   TN5250_ASSERT (This != NULL);
   free (This);
}

/*
 *    Convert the display info to WTD data.
 */
void tn5250_wtd_context_convert (Tn5250WTDContext *This)
{
   /* The differential conversion is not yet implemented, and probably
    * won't be until we implement the 5250 server. */
   TN5250_ASSERT (This->src_dbuffer == NULL && This->src_table == NULL);

   tn5250_wtd_context_convert_nosrc (This);
}

/*
 *    This is our routine which is used when we don't know the prior state
 *    of the format table or display buffer.
 */
static void tn5250_wtd_context_convert_nosrc (Tn5250WTDContext *This)
{
   unsigned char c;
   Tn5250Field *field;
   int fy, fx;

   TN5250_LOG (("wtd_context_convert entered.\n"));

   /* Since we don't know the unit's prior state, we clear the unit. */
   tn5250_wtd_context_putc (This, ESC);
   if (tn5250_dbuffer_width (This->dst_dbuffer) != 80) {
      tn5250_wtd_context_putc (This, CMD_CLEAR_UNIT_ALTERNATE);
      tn5250_wtd_context_putc (This, 0x00);
   } else
      tn5250_wtd_context_putc (This, CMD_CLEAR_UNIT);
   This->clear_unit = 1;

   tn5250_wtd_context_putc (This, ESC);
   tn5250_wtd_context_putc (This, CMD_WRITE_TO_DISPLAY);

   /* FIXME: We will want to use different CC1/CC2 codes sometimes... */
   tn5250_wtd_context_putc (This, 0x00); /* CC1 */
   tn5250_wtd_context_putc (This, 0x00); /* CC2 */

   /* If we have header data, start with a SOH order. */
   if (This->dst_table->header_length != 0) {
      int i;
      tn5250_wtd_context_putc (This, SOH);
      tn5250_wtd_context_putc (This, This->dst_table->header_length);
      for (i = 0; i < This->dst_table->header_length; i++)
	 tn5250_wtd_context_putc (This, This->dst_table->header_data[i]);
   }

   /* FIXME: Set the insert-cursor address using IC if necessary. */

   for (This->y = 0; This->y < tn5250_dbuffer_height(This->dst_dbuffer);
	 This->y++) {
      for (This->x = 0; This->x < tn5250_dbuffer_width(This->dst_dbuffer);
	    This->x++) {
	 c = tn5250_dbuffer_char_at (This->dst_dbuffer, This->y, This->x);
	 field = tn5250_wtd_context_peek_field (This);
	 if (field != NULL) {
	    /* Start of a field, write an SF order.  We have to remove
	     * the last byte we put on the buffer (since its the attribute,
	     * which is taken care of here. */

	    tn5250_wtd_context_write_field (This, field, c);
	 } else
	    tn5250_wtd_context_ra_putc (This, c);
      }
   }

   TN5250_LOG_BUFFER (This->buffer);
}

/*
 *    Return a pointer to the field that _starts_ at the next position on the
 *    display, or NULL if no field starts there.
 */
static Tn5250Field *tn5250_wtd_context_peek_field (Tn5250WTDContext *This)
{
   int nx = This->x, ny = This->y;
   Tn5250Field *field;
   
   if (++nx == tn5250_dbuffer_width (This->dst_dbuffer)) {
      if (++ny == tn5250_dbuffer_height (This->dst_dbuffer))
	 return NULL;
   }

   field = tn5250_table_field_yx (This->dst_table, ny, nx);
   if (field == NULL)
      return NULL;

   if (tn5250_field_start_row (field) != ny
	 || tn5250_field_start_col (field) != nx)
      return NULL;

   return field;
}

/*
 *    Put a character (which may be an attribute or a parameter for an order)
 *    directly into the buffer.  Flush the RA buffer if necessary.
 */
static void tn5250_wtd_context_putc (Tn5250WTDContext *This, unsigned char c)
{
   tn5250_wtd_context_ra_flush (This);
   tn5250_buffer_append_byte (This->buffer, c);
}

static void tn5250_wtd_context_ra_putc (Tn5250WTDContext *This, unsigned char c)
{
   if (This->ra_char != c)
      tn5250_wtd_context_ra_flush (This);
   This->ra_char = c;
   This->ra_count++;
}

static void tn5250_wtd_context_ra_flush (Tn5250WTDContext *This)
{
   if (This->ra_count == 0)
      return;

   /* Determine which is smaller, the RA/SBA order or just repeating the
    * bytes. */
   if (This->ra_count <= 4 &&
	 !(This->ra_count == 3 && This->ra_char == 0x00 && This->clear_unit)) {
      /* Just repeating the bytes won. */
      while (This->ra_count > 0) {
	 tn5250_buffer_append_byte (This->buffer, This->ra_char);
	 This->ra_count--;
      }
   } else {
      /* We can just use an SBA if the character is a NUL and we've just
       * cleared the unit.  Yes, I'm being picky over saving one byte. */
      if (This->clear_unit && This->ra_char == 0x00) {
	 tn5250_buffer_append_byte (This->buffer, SBA);
	 tn5250_buffer_append_byte (This->buffer, This->y + 1);
	 tn5250_buffer_append_byte (This->buffer, This->x + 1);
      } else {
	 int px = This->x, py = This->y;
	 if (px == 0) {
	    px = tn5250_dbuffer_width (This->dst_dbuffer) - 1;
	    TN5250_ASSERT (py != 0);
	    py--;
	 } else
	    px--;

	 tn5250_buffer_append_byte (This->buffer, RA);
	 tn5250_buffer_append_byte (This->buffer, py + 1);
	 tn5250_buffer_append_byte (This->buffer, px + 1);
	 tn5250_buffer_append_byte (This->buffer, This->ra_char);
      }
   }

   This->ra_count = 0;
}

/*
 *    Write the SF order and the field's information.
 */
static void tn5250_wtd_context_write_field (Tn5250WTDContext *This, Tn5250Field *field, unsigned char attr)
{
   TN5250_LOG (("Writing SF order in stream data.\n"));

   tn5250_wtd_context_putc (This, SF);

   /* Put the field format word. */
   if (field->FFW != 0) {
      tn5250_wtd_context_putc (This, (unsigned char)(field->FFW >> 8));
      tn5250_wtd_context_putc (This, (unsigned char)(field->FFW & 0x00ff));

      /* Put the field control word(s). */
      if (field->FCW != 0) {
	 tn5250_wtd_context_putc (This, (unsigned char)(field->FCW >> 8));
	 tn5250_wtd_context_putc (This, (unsigned char)(field->FCW & 0x00ff));
      }
   }

   /* Put the screen attribute. */
   TN5250_ASSERT (tn5250_attribute (attr));
   tn5250_wtd_context_putc (This, attr);

   /* Put the field length. */
   tn5250_wtd_context_putc (This, (unsigned char)(field->length >> 8));
   tn5250_wtd_context_putc (This, (unsigned char)(field->length & 0x00ff));
}

/* vi:set cindent sts=3 sw=3: */
