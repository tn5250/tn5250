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
   This->disp_indicators = 0;
   This->next = This->prev = NULL;

   This->data = tn5250_new(unsigned char, width * height);
   if (This->data == NULL) {
      free(This);
      return NULL;
   }
   
   tn5250_dbuffer_clear(This);
   return This;
}

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
   This->disp_indicators = dsp->disp_indicators;
   This->data = tn5250_new(unsigned char, dsp->w * dsp->h);
   if (This->data == NULL) {
      free(This);
      return NULL;
   }
   memcpy (This->data, dsp->data, dsp->w * dsp->h);

   ASSERT_VALID(This);
   return This;
}

void tn5250_dbuffer_destroy(Tn5250DBuffer * This)
{
   free(This->data);
   free(This);
}

/*
 *    Resize the display (say, to 132 columns ;)
 */
void tn5250_dbuffer_set_size(Tn5250DBuffer * This, int rows, int cols)
{
   int r;

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

void tn5250_dbuffer_indicator_set(Tn5250DBuffer * This, int inds)
{
   TN5250_LOG(("tn5250_dbuffer_indicator_set: setting indicators X'%02X'.\n", inds));
   This->disp_indicators |= inds;
}

void tn5250_dbuffer_indicator_clear(Tn5250DBuffer * This, int inds)
{
   TN5250_LOG(("tn5250_dbuffer_indicator_clear: clearing indicators X'%02X'.\n", inds));
   This->disp_indicators &= ~inds;
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

   for (i = 0; i < shiftcount; i++) {
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

void tn5250_dbuffer_set_temp_ic(Tn5250DBuffer * This, int y, int x)
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
