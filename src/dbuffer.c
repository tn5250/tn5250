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
#include <string.h>
#include <stdio.h>
#include <malloc.h>
#include "utility.h"
#include "dbuffer.h"
#include "field.h"
#include "buffer.h"
#include "record.h"
#include "stream.h"
#include "session.h"

#ifdef NDEBUG
#define ASSERT_VALID(This)
#else
#define ASSERT_VALID(This) \
   { \
      TN5250_ASSERT ((This) != NULL); \
      TN5250_ASSERT ((This)->cy >= 0); \
      TN5250_ASSERT ((This)->cx >= 0); \
      TN5250_ASSERT ((This)->cy < (This)->h); \
      TN5250_ASSERT ((This)->cx < (This)->w); \
   }
#endif

/*
 *    Allocate a new display buffer.
 */
Tn5250DBuffer *tn5250_dbuffer_new(int width, int height)
{
   int n;
   Tn5250DBuffer *This = tn5250_new(Tn5250DBuffer, 1);

   if (This == NULL)
      return NULL;

   This->w = width;
   This->h = height;
   This->cx = This->cy = 0;
   This->tcx = This->tcy = 0;
   This->next = This->prev = NULL;

   This->field_count = 0;
   This->field_list = NULL;
   This->master_mdt = 0;
   This->header_data = NULL;
   This->header_length = 0;

   This->data = tn5250_new(unsigned char, width * height);
   if (This->data == NULL) {
      free(This);
      return NULL;
   }
   
   tn5250_dbuffer_clear(This);
   return This;
}

/*
 *    Allocate a new display buffer and copy the contents of the old
 *    one.
 */
Tn5250DBuffer *tn5250_dbuffer_copy(Tn5250DBuffer * dsp)
{
   int n;
   Tn5250DBuffer *This = tn5250_new(Tn5250DBuffer, 1);
   if (This == NULL)
      return NULL;

   ASSERT_VALID(dsp);

   This->w = dsp->w;
   This->h = dsp->h;
   This->cx = dsp->cx;
   This->cy = dsp->cy;
   This->tcx = dsp->tcx;
   This->tcy = dsp->tcy;
   This->data = tn5250_new(unsigned char, dsp->w * dsp->h);
   if (This->data == NULL) {
      free(This);
      return NULL;
   }
   memcpy (This->data, dsp->data, dsp->w * dsp->h);

   This->field_list = tn5250_field_list_copy (dsp->field_list);
   This->header_length = dsp->header_length;
   if (dsp->header_data != NULL) {
      This->header_data = (unsigned char *)malloc (This->header_length);
      TN5250_ASSERT (This->header_data != NULL);
      memcpy (This->header_data, dsp->header_data, dsp->header_length);
   } else
      This->header_data = NULL;

   ASSERT_VALID(This);
   return This;
}

/*
 *    Free a display buffer and destroy all sub-structures.
 */
void tn5250_dbuffer_destroy(Tn5250DBuffer * This)
{
   free(This->data);
   if (This->header_data != NULL)
      free (This->header_data);
   (void)tn5250_field_list_destroy(This->field_list);
   free(This);
}

/*
 *    Set the format table header data.
 */
void tn5250_dbuffer_set_header_data(Tn5250DBuffer *This, unsigned char *data, int len)
{
   if (This->header_data != NULL)
      free (This->header_data);
   This->header_length = len;
   if (data != NULL) {
      TN5250_ASSERT (len > 0);
      This->header_data = (unsigned char *)malloc (len);
      TN5250_ASSERT (This->header_data != NULL);
      memcpy (This->header_data, data, len);
   }
}

/*
 *    Determine, according to the format table header, if we should send
 *    data for this aid key.
 */
int tn5250_dbuffer_send_data_for_aid_key(Tn5250DBuffer *This, int k)
{
   int byte, bit, result;

   if (This->header_data == NULL || This->header_length <= 6) {
      result = 1;
      TN5250_LOG (("tn5250_dbuffer_send_data_for_aid_key: no format table header or key mask.\n"));
      goto send_data_done;
   }

#ifndef NDEBUG
   TN5250_LOG (("tn5250_dbuffer_send_data_for_aid_key: format table header = \n"));
   for (byte = 0; byte < This->header_length; byte++) {
      TN5250_LOG (("0x%02X ",This->header_data[byte]));
   }
   TN5250_LOG (("\n"));
#endif

   switch (k) {
   case TN5250_SESSION_AID_F1: byte = 6; bit = 7; break;
   case TN5250_SESSION_AID_F2: byte = 6; bit = 6; break;
   case TN5250_SESSION_AID_F3: byte = 6; bit = 5; break;
   case TN5250_SESSION_AID_F4: byte = 6; bit = 4; break;
   case TN5250_SESSION_AID_F5: byte = 6; bit = 3; break;
   case TN5250_SESSION_AID_F6: byte = 6; bit = 2; break;
   case TN5250_SESSION_AID_F7: byte = 6; bit = 1; break;
   case TN5250_SESSION_AID_F8: byte = 6; bit = 0; break;
   case TN5250_SESSION_AID_F9: byte = 5; bit = 7; break;
   case TN5250_SESSION_AID_F10: byte = 5; bit = 6; break;
   case TN5250_SESSION_AID_F11: byte = 5; bit = 5; break;
   case TN5250_SESSION_AID_F12: byte = 5; bit = 4; break;
   case TN5250_SESSION_AID_F13: byte = 5; bit = 3; break;
   case TN5250_SESSION_AID_F14: byte = 5; bit = 2; break;
   case TN5250_SESSION_AID_F15: byte = 5; bit = 1; break;
   case TN5250_SESSION_AID_F16: byte = 5; bit = 0; break;
   case TN5250_SESSION_AID_F17: byte = 4; bit = 7; break;
   case TN5250_SESSION_AID_F18: byte = 4; bit = 6; break;
   case TN5250_SESSION_AID_F19: byte = 4; bit = 5; break;
   case TN5250_SESSION_AID_F20: byte = 4; bit = 4; break;
   case TN5250_SESSION_AID_F21: byte = 4; bit = 3; break;
   case TN5250_SESSION_AID_F22: byte = 4; bit = 2; break;
   case TN5250_SESSION_AID_F23: byte = 4; bit = 1; break;
   case TN5250_SESSION_AID_F24: byte = 4; bit = 0; break;
   default:
     result = 1;
     goto send_data_done;
   }

   result = ((This->header_data[byte] & (0x80 >> bit)) == 0);
send_data_done:
   TN5250_LOG (("tn5250_dbuffer_send_data_for_aid_key() = %d\n", result));
   return result;
}

/*
 *    Resize the display (say, to 132 columns ;)
 */
void tn5250_dbuffer_set_size(Tn5250DBuffer * This, int rows, int cols)
{
   This->w = cols;
   This->h = rows;

   free(This->data);
   This->data = tn5250_new(unsigned char, rows * cols);
   TN5250_ASSERT (This->data != NULL);

   tn5250_dbuffer_clear(This);
}

void tn5250_dbuffer_cursor_set(Tn5250DBuffer * This, int y, int x)
{
   This->cy = y;
   This->cx = x;

   ASSERT_VALID(This);
}

void tn5250_dbuffer_clear(Tn5250DBuffer * This)
{
   int r, c;
   memset (This->data, 0, This->w * This->h);
   This->cx = This->cy = 0;
   tn5250_dbuffer_clear_table (This);
}

void tn5250_dbuffer_add_field(Tn5250DBuffer *This, Tn5250Field *field)
{
   field->id = This->field_count++;
   field->table = This;
   This->field_list = tn5250_field_list_add(This->field_list, field);
}

void tn5250_dbuffer_clear_table(Tn5250DBuffer * This)
{
   TN5250_LOG(("tn5250_dbuffer_clear_table() entered.\n"));
   This->field_list = tn5250_field_list_destroy(This->field_list);
   This->field_count = 0;
   This->master_mdt = 0;
   This->header_length = 0;
   if (This->header_data != NULL) {
      free (This->header_data);
      This->header_data = NULL;
   }
}

Tn5250Field *tn5250_dbuffer_field_yx (Tn5250DBuffer *This, int y, int x)
{
   Tn5250Field *iter;
   if ((iter = This->field_list) != NULL) {
      do {
	 if (tn5250_field_hit_test(iter, y, x))
	    return iter;
	 iter = iter->next;
      } while (iter != This->field_list);
   }
   return NULL; 
}

void tn5250_dbuffer_right(Tn5250DBuffer * This, int n)
{
   This->cx += n;
   This->cy += (This->cx / This->w);
   This->cx = (This->cx % This->w);
   This->cy = (This->cy % This->h);

   ASSERT_VALID(This);
}

void tn5250_dbuffer_left(Tn5250DBuffer * This)
{
   This->cx--;
   if (This->cx == -1) {
      This->cx = This->w - 1;
      This->cy--;
      if (This->cy == -1) {
	 This->cy = This->h - 1;
      }
   }

   ASSERT_VALID(This);
}

void tn5250_dbuffer_up(Tn5250DBuffer * This)
{
   if (--This->cy == -1)
      This->cy = This->h - 1;

   ASSERT_VALID(This);
}

void tn5250_dbuffer_down(Tn5250DBuffer * This)
{
   if (++This->cy == This->h)
      This->cy = 0;

   ASSERT_VALID(This);
}

void tn5250_dbuffer_goto_ic(Tn5250DBuffer * This)
{
   ASSERT_VALID(This);

   This->cy = This->tcy;
   This->cx = This->tcx;

   ASSERT_VALID(This);
}

void tn5250_dbuffer_addch(Tn5250DBuffer * This, unsigned char c)
{
   ASSERT_VALID(This);

   This->data[(This->cy * This->w) + This->cx] = c;
   tn5250_dbuffer_right(This, 1);

   ASSERT_VALID(This);
}

void tn5250_dbuffer_del(Tn5250DBuffer * This, int shiftcount)
{
   int x = This->cx, y = This->cy, fwdx, fwdy, i;

   for (i = 0; i < shiftcount; i++) {
      fwdx = x + 1;
      fwdy = y;
      if (fwdx == This->w) {
	 fwdx = 0;
	 fwdy++;
      }
      This->data[y * This->w + x] = This->data[fwdy * This->w + fwdx];
      x = fwdx;
      y = fwdy;
   }
   This->data[y * This->w + x] = 0x40;

   ASSERT_VALID(This);
}

void tn5250_dbuffer_ins(Tn5250DBuffer * This, unsigned char c, int shiftcount)
{
   int x = This->cx, y = This->cy, i;
   unsigned char c2;

   for (i = 0; i <= shiftcount; i++) {
      c2 = This->data[y * This->w + x];
      This->data[y * This->w + x] = c;
      c = c2;
      if (++x == This->w) {
	 x = 0;
	 y++;
      }
   }
   tn5250_dbuffer_right(This, 1);

   ASSERT_VALID(This);
}

void tn5250_dbuffer_set_ic(Tn5250DBuffer * This, int y, int x)
{
   This->tcx = x;
   This->tcy = y;

   ASSERT_VALID(This);
}

void tn5250_dbuffer_roll(Tn5250DBuffer * This, int top, int bot, int lines)
{
   int n, c;

   ASSERT_VALID(This);

   if (lines == 0)
      return;

   if (lines < 0) {
      /* Move text up */
      for (n = top; n <= bot; n++) {
	 if (n + lines >= top) {
	    for (c = 0; c < This->w; c++) {
	       This->data[(n + lines) * This->w + c] =
		  This->data[n * This->w + c];
	    }
	 }
      }
   } else {
      for (n = bot; n >= top; n--) {
	 if (n + lines <= bot) {
	    for (c = 0; c < This->w; c++) {
	       This->data[(n  + lines) * This->w + c] =
		  This->data[n * This->w + c];
	    }
	 }
      }
   }
   ASSERT_VALID(This);
}

unsigned char tn5250_dbuffer_char_at(Tn5250DBuffer * This, int y, int x)
{
   ASSERT_VALID(This);
   TN5250_ASSERT(y >= 0);
   TN5250_ASSERT(x >= 0);
   TN5250_ASSERT(y < This->h);
   TN5250_ASSERT(x < This->w);
   return This->data[y * This->w + x];
}

/* vi:set cindent sts=3 sw=3: */
