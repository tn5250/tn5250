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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include "buffer.h"
#include "utility.h"

#define BUFFER_DELTA 128

void tn5250_buffer_init(Tn5250Buffer * This)
{
   This->len = This->allocated = 0;
   This->data = NULL;
}

void tn5250_buffer_free(Tn5250Buffer * This)
{
   if (This->data != NULL)
      free(This->data);
   This->data = NULL;
   This->len = This->allocated = 0;
}

void tn5250_buffer_append_byte(Tn5250Buffer * This, unsigned char b)
{
   if (This->len + 1 >= This->allocated) {
      if (This->data == NULL) {
	 This->allocated = BUFFER_DELTA;
	 This->data = (unsigned char *) malloc(This->allocated);
      } else {
	 This->allocated += BUFFER_DELTA;
	 This->data = (unsigned char *) realloc(This->data, This->allocated);
      }
   }
   TN5250_ASSERT (This->data != NULL);
   This->data[This->len++] = b;
}

void tn5250_buffer_append_data(Tn5250Buffer * This, unsigned char *data, int len)
{
   while (len--)
      tn5250_buffer_append_byte(This, *data++);
}

/* vi:set sts=3 sw=3 autoindent: */
