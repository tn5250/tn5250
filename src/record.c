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
#include <string.h>
#include <ctype.h>
#include <malloc.h>

#include "utility.h"
#include "record.h"

Tn5250Record *tn5250_record_new()
{
   Tn5250Record *This = tn5250_new(Tn5250Record, 1);
   if (This == NULL)
      return NULL;

   This->flags = 0;
   This->opcode = 0;
   This->cur_pos = 0;
   This->length = 0;
   This->allocated = 0;
   This->data = NULL;
   This->prev = NULL;
   This->next = NULL;
   This->header_length = 0;
   return This;
}

void tn5250_record_destroy(Tn5250Record * This)
{
   if (This != NULL) {
      if (This->data != NULL)
	 free(This->data);
      free(This);
   }
}

void tn5250_record_append_byte(Tn5250Record * This, unsigned char b)
{
   if (This->length + 1 >= This->allocated) {
      if (This->data == NULL) {
	 This->allocated = 128;
	 This->data = (unsigned char *) malloc(This->allocated);
      } else {
	 This->allocated += 128;
	 This->data = (unsigned char *) realloc(This->data, This->allocated);
      }
      TN5250_ASSERT(This->data != NULL);
   }
   This->data[This->length++] = b;
}

unsigned char tn5250_record_get_byte(Tn5250Record * This)
{
   This->cur_pos++;
   TN5250_ASSERT(This->cur_pos <= (This->length - This->header_length));
   return This->data[This->cur_pos - 1];
}

void tn5250_record_unget_byte(Tn5250Record * This)
{
   TN5250_LOG(("Record::UnGetByte: entered.\n"));
   TN5250_ASSERT(This->cur_pos > 0);
   This->cur_pos--;
}

void tn5250_record_set_flow_type(Tn5250Record * This, unsigned char byte1, unsigned char byte2)
{
   This->flowtype[0] = byte1;
   This->flowtype[1] = byte2;
}

int tn5250_record_is_chain_end(Tn5250Record * This)
{
   return (This->length - This->header_length) == This->cur_pos;
}

#ifndef NDEBUG
void tn5250_record_dump(Tn5250Record * This)
{
   int pos;
   unsigned char t[17];
   unsigned char c;
   unsigned char a;
   int n;

   for (pos = 0; pos < This->length - This->header_length;) {
      memset(t, 0, sizeof(t));
      TN5250_LOG(("@record +%4.4X ", pos));
      for (n = 0; n < 16; n++) {
	 if (pos < This->length - This->header_length) {
	    c = This->data[pos];
	    a = tn5250_ebcdic2ascii(c);
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

   TN5250_LOG(("@eor\n"));
}
#endif

/*
 *    Add a record to the end of a list of records.
 */
Tn5250Record *tn5250_record_list_add(Tn5250Record * list, Tn5250Record * record)
{
   if (list == NULL) {
      list = record->next = record->prev = record;
      return list;
   }
   record->next = list;
   record->prev = list->prev;
   record->prev->next = record;
   record->next->prev = record;
   return list;
}

/*
 *    Remove a record from a list of records.
 */
Tn5250Record *tn5250_record_list_remove(Tn5250Record * list, Tn5250Record * record)
{
   if (list == NULL)
      return NULL;
   if (list->next == list) {
      record->prev = record->next = NULL;
      return NULL;
   }

   if (list == record)
      list = list->next;

   record->next->prev = record->prev;
   record->prev->next = record->next;
   record->next = record->prev = NULL;
   return list;
}

/*
 *    Destroy all records in a record list and return NULL.
 */
Tn5250Record *tn5250_record_list_destroy(Tn5250Record * list)
{
   Tn5250Record *iter, *next;

   if ((iter = list) != NULL) {
      do {
	 next = iter->next;
	 tn5250_record_destroy(iter);
	 iter = next;
      } while (iter != list);
   }
   return NULL;
}

/* vi:set sts=3 sw=3: */
