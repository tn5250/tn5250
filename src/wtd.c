/* TN5250 - An implementation of the 5250 telnet protocol.
 * Copyright (C) 2000 Jason M. Felice
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

/* TODO:
 * - We need to generate TD (Transparent Data) orders if needed.
 */
static void tn5250_wtd_context_putc (Tn5250WTDContext *This, unsigned char c);
static void tn5250_wtd_context_ra_putc (Tn5250WTDContext *This, unsigned char c);
static void tn5250_wtd_context_ra_flush (Tn5250WTDContext *This);
static void tn5250_wtd_context_write_field (Tn5250WTDContext *This, Tn5250Field *field, unsigned char attr);
static void tn5250_wtd_context_convert_nosrc (Tn5250WTDContext *This);
static Tn5250Field *tn5250_wtd_context_peek_field (Tn5250WTDContext *This);

/****f* lib5250/tn5250_wtd_context_new
 * NAME
 *    tn5250_wtd_context_new
 * SYNOPSIS
 *    ret = tn5250_wtd_context_new (buffer, sd, dd);
 * INPUTS
 *    Tn5250Buffer *       buffer     - 
 *    Tn5250DBuffer *      sd         - 
 *    Tn5250DBuffer *      dd         - 
 * DESCRIPTION
 *    Create a new instance of our WTD context object and initialize it.
 *****/
Tn5250WTDContext *tn5250_wtd_context_new (Tn5250Buffer *buffer, Tn5250DBuffer *sd, Tn5250DBuffer *dd)
{
   Tn5250WTDContext *This;

   TN5250_ASSERT(dd != NULL);
   TN5250_ASSERT(buffer != NULL);

   if ((This = tn5250_new (Tn5250WTDContext, 1)) == NULL)
      return NULL;

   This->buffer = buffer;
   This->src = sd;
   This->dst = dd;
   This->ra_count = 0;
   This->ra_char = 0x00;
   This->clear_unit = 0;

   return This;
}

/****f* lib5250/tn5250_wtd_context_destroy
 * NAME
 *    tn5250_wtd_context_destroy
 * SYNOPSIS
 *    tn5250_wtd_context_destroy (This);
 * INPUTS
 *    Tn5250WTDContext *   This       - 
 * DESCRIPTION
 *    Destroy a WTD context object.
 *****/
void tn5250_wtd_context_destroy (Tn5250WTDContext *This)
{
   TN5250_ASSERT (This != NULL);
   free (This);
}

/****f* lib5250/tn5250_wtd_context_set_ic
 * NAME
 *   tn5250_wtd_context_set_ic
 * SYNOPSIS
 *   tn5250_wtd_context_set_ic (This, y, x);
 * INPUTS
 *    TN5250WTDContext * This -
 *    int y -
 *    int x -
 * DESCRIPTION
 *    Sets the x and y position to be used in the IC order.
 *****/
void tn5250_wtd_context_set_ic (Tn5250WTDContext *This, int y, int x)
{
   This->y = y;
   This->x = x;
}
/****f* lib5250/tn5250_wtd_context_convert
 * NAME
 *    tn5250_wtd_context_convert
 * SYNOPSIS
 *    tn5250_wtd_context_convert (This);
 * INPUTS
 *    Tn5250WTDContext *   This       - 
 * DESCRIPTION
 *    Convert the display info to WTD data.
 *****/
void tn5250_wtd_context_convert (Tn5250WTDContext *This)
{
   /* The differential conversion is not yet implemented, and probably
    * won't be until we implement the 5250 server. */
   TN5250_ASSERT (This->src == NULL);

   tn5250_wtd_context_convert_nosrc (This);
}

/****i* lib5250/tn5250_wtd_context_convert_nosrc
 * NAME
 *    tn5250_wtd_context_convert_nosrc
 * SYNOPSIS
 *    tn5250_wtd_context_convert_nosrc (This);
 * INPUTS
 *    Tn5250WTDContext *   This       - 
 * DESCRIPTION
 *    This is our routine which is used when we don't know the prior state
 *    of the format table or display buffer.
 *****/
static void tn5250_wtd_context_convert_nosrc (Tn5250WTDContext *This)
{
   unsigned char c;
   Tn5250Field *field;
   int fy, fx;

   TN5250_LOG (("wtd_context_convert entered.\n"));

   tn5250_wtd_context_putc (This, ESC);
   tn5250_wtd_context_putc (This, CMD_RESTORE_SCREEN);

   /* Since we don't know the unit's prior state, we clear the unit. */
   tn5250_wtd_context_putc (This, ESC);
   if (tn5250_dbuffer_width (This->dst) != 80) {
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

   /* 
    Set the insert-cursor address.  This is necessary to ensure that
    we return to same field we were in before the save_screen.
   */
   
   tn5250_wtd_context_putc (This, IC);
   tn5250_wtd_context_putc (This, This->y);
   tn5250_wtd_context_putc (This, This->x);

   /* If we have header data, start with a SOH order. */
   if (This->dst->header_length != 0) {
      int i;
      tn5250_wtd_context_putc (This, SOH);
      tn5250_wtd_context_putc (This, This->dst->header_length);
      for (i = 0; i < This->dst->header_length; i++)
	 tn5250_wtd_context_putc (This, This->dst->header_data[i]);
   }


   for (This->y = 0; This->y < tn5250_dbuffer_height(This->dst);
	 This->y++) {
      for (This->x = 0; This->x < tn5250_dbuffer_width(This->dst);
	    This->x++) {
	 c = tn5250_dbuffer_char_at (This->dst, This->y, This->x);
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

#ifndef NDEBUG
   tn5250_buffer_log (This->buffer, "wtd>");
#endif
}

/****i* lib5250/tn5250_wtd_context_peek_field
 * NAME
 *    tn5250_wtd_context_peek_field
 * SYNOPSIS
 *    ret = tn5250_wtd_context_peek_field (This);
 * INPUTS
 *    Tn5250WTDContext *   This       - 
 * DESCRIPTION
 *    Return a pointer to the field that _starts_ at the next position on the
 *    display, or NULL if no field starts there.
 *****/
static Tn5250Field *tn5250_wtd_context_peek_field (Tn5250WTDContext *This)
{
   int nx = This->x, ny = This->y;
   Tn5250Field *field;

   if (++nx == tn5250_dbuffer_width (This->dst)) {
     if (++ny == tn5250_dbuffer_height (This->dst)) {
	 return NULL;
     } else {
       nx = 0;
     }
   }

   field = tn5250_dbuffer_field_yx (This->dst, ny, nx);
   if (field == NULL)
      return NULL;

   if (tn5250_field_start_row (field) != ny
	 || tn5250_field_start_col (field) != nx)
      return NULL;

   return field;
}

/****i* lib5250/tn5250_wtd_context_putc
 * NAME
 *    tn5250_wtd_context_putc
 * SYNOPSIS
 *    tn5250_wtd_context_putc (This, c);
 * INPUTS
 *    Tn5250WTDContext *   This       - 
 *    unsigned char        c          - 
 * DESCRIPTION
 *    Put a character (which may be an attribute or a parameter for an order)
 *    directly into the buffer.  Flush the RA buffer if necessary.
 *****/
static void tn5250_wtd_context_putc (Tn5250WTDContext *This, unsigned char c)
{
   tn5250_wtd_context_ra_flush (This);
   tn5250_buffer_append_byte (This->buffer, c);
}

/****i* lib5250/tn5250_wtd_context_ra_putc
 * NAME
 *    tn5250_wtd_context_ra_putc
 * SYNOPSIS
 *    tn5250_wtd_context_ra_putc (This, c);
 * INPUTS
 *    Tn5250WTDContext *   This       - 
 *    unsigned char        c          - 
 * DESCRIPTION
 *    DOCUMENT ME!!!
 *****/
static void tn5250_wtd_context_ra_putc (Tn5250WTDContext *This, unsigned char c)
{
   if (This->ra_char != c)
      tn5250_wtd_context_ra_flush (This);
   This->ra_char = c;
   This->ra_count++;
}

/****i* lib5250/tn5250_wtd_context_ra_flush
 * NAME
 *    tn5250_wtd_context_ra_flush
 * SYNOPSIS
 *    tn5250_wtd_context_ra_flush (This);
 * INPUTS
 *    Tn5250WTDContext *   This       - 
 * DESCRIPTION
 *    DOCUMENT ME!!!
 *****/
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
	    px = tn5250_dbuffer_width (This->dst) - 1;
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

/****i* lib5250/tn5250_wtd_context_write_field
 * NAME
 *    tn5250_wtd_context_write_field
 * SYNOPSIS
 *    tn5250_wtd_context_write_field (This, field, attr);
 * INPUTS
 *    Tn5250WTDContext *   This       - 
 *    Tn5250Field *        field      - 
 *    unsigned char        attr       - 
 * DESCRIPTION
 *    Write the SF order and the field's information.
 *****/
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
   /* FIXME: Re-enable assert: TN5250_ASSERT (tn5250_attribute (attr)); */
   /* XXX: For some reason, the attribute in the display buffer doesn't
        pass the following test.  This is a problem because we rely on
        that attribute set-up when parsing the screen during a restore.
        So, for now, work around that problem by sending the attribute
        from the field buffer.  */
   if ((attr&0xe0)!=0x20)
      tn5250_wtd_context_putc(This, tn5250_field_attribute(field));
   else
      tn5250_wtd_context_putc (This, attr);


   /* Put the field length. */
   tn5250_wtd_context_putc (This, (unsigned char)(field->length >> 8));
   tn5250_wtd_context_putc (This, (unsigned char)(field->length & 0x00ff));

}

