/* TN5250
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
#include "tn5250-private.h"

#define BUFFER_DELTA 128

/****f* lib5250/tn5250_buffer_init
 * NAME
 *    tn5250_buffer_init
 * SYNOPSIS
 *    tn5250_buffer_init (&buf);
 * INPUTS
 *    Tn5250Buffer *	buf	  - Pointer to a Tn5250Buffer object.
 * DESCRIPTION
 *    Zeros internal members.
 *****/
void tn5250_buffer_init(Tn5250Buffer * This)
{
   This->len = This->allocated = 0;
   This->data = NULL;
}

/****f* lib5250/tn5250_buffer_free
 * NAME
 *    tn5250_buffer_free
 * SYNOPSIS
 *    tn5250_buffer_free (&buf);
 * INPUTS
 *    Tn5250Buffer *	buf	 - Pointer to a buffer object.
 * DESCRIPTION
 *    Frees variable-length data and zeros internal members.
 *****/
void tn5250_buffer_free(Tn5250Buffer * This)
{
   if (This->data != NULL)
      free(This->data);
   This->data = NULL;
   This->len = This->allocated = 0;
}

/****f* lib5250/tn5250_buffer_append_byte
 * NAME
 *    tn5250_buffer_append_byte
 * SYNOPSIS
 *    tn5250_buffer_append_byte (&buf, byte);
 * INPUTS
 *    Tn5250Buffer *	buf	 - Pointer to a buffer object.
 *    unsigned char byte	 - The byte to append.
 * DESCRIPTION
 *    Appends a single byte to the end of the variable-length data
 *    and reallocates the buffer if necessary to accomodate the new
 *    byte.
 *****/
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

/****f* lib5250/tn5250_buffer_append_data
 * NAME
 *    tn5250_buffer_append_data
 * SYNOPSIS
 *    tn5250_buffer_append_data (&buf, data, len);
 * INPUTS
 *    Tn5250Buffer *	buf	    - Pointer to a buffer object.
 *    unsigned char *	data	    - Data to append.
 *    int		len	    - Length of data to append.
 * DESCRIPTION
 *    Appends a variable number of bytes to the end of the variable-length
 *    data and reallocates the buffer if necessary to accomodate the new
 *    data.
 *****/
void tn5250_buffer_append_data(Tn5250Buffer * This, unsigned char *data, int len)
{
   while (len--)
      tn5250_buffer_append_byte(This, *data++);
}

/****f* lib5250/tn5250_buffer_log
 * NAME
 *    tn5250_buffer_log
 * SYNOPSIS
 *    tn5250_buffer_log (&buf,"> ");
 * INPUTS
 *    Tn5250Buffer *	buf	    - Pointer to a buffer object.
 *    const char *	prefix	    - Character string to prefix dump lines
 *                                    with in log.
 * DESCRIPTION
 *    Dumps the contents of the buffer to the 5250 logfile, if one is
 *    currently open.
 *****/
void tn5250_buffer_log(Tn5250Buffer * This, const char *prefix)
{
   int pos;
   unsigned char t[17];
   unsigned char c;
   unsigned char a;
   int n;
   Tn5250CharMap *map = tn5250_char_map_new ("37");

   TN5250_LOG (("Dumping buffer (length=%d):\n", This->len));
   for (pos = 0; pos < This->len;) {
      memset(t, 0, sizeof(t));
      TN5250_LOG(("%s +%4.4X ", prefix, pos));
      for (n = 0; n < 16; n++) {
	 if (pos < This->len) {
	    c = This->data[pos];
	    a = tn5250_char_map_to_local (map, c);
	    TN5250_LOG(("%02x", c));
	    t[n] = (isprint(a)) ? a : '.';
	 } else
	    TN5250_LOG(("  "));
	 pos++;
	 if ((pos & 3) == 0)
	    TN5250_LOG((" "));
      }
      TN5250_LOG((" %s\n", t));
   }
   TN5250_LOG (("\n"));
}

/* vi:set sts=3 sw=3 autoindent: */
