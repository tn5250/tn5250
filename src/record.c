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

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <malloc.h>

#include "utility.h"
#include "buffer.h"
#include "record.h"

Tn5250Record *tn5250_record_new()
{
   Tn5250Record *This = tn5250_new(Tn5250Record, 1);
   if (This == NULL)
      return NULL;

   tn5250_buffer_init (&(This->data));

   This->cur_pos = 0;
   This->prev = NULL;
   This->next = NULL;
   return This;
}

void tn5250_record_destroy(Tn5250Record * This)
{
   if (This != NULL) {
      tn5250_buffer_free (&(This->data));
      free(This);
   }
}

unsigned char tn5250_record_get_byte(Tn5250Record * This)
{
   This->cur_pos++;
   TN5250_ASSERT(This->cur_pos <= tn5250_record_length(This));
   return (tn5250_buffer_data (&(This->data)))[This->cur_pos - 1];
}

void tn5250_record_unget_byte(Tn5250Record * This)
{
   TN5250_LOG(("Record::UnGetByte: entered.\n"));
   TN5250_ASSERT(This->cur_pos > 0);
   This->cur_pos--;
}

int tn5250_record_is_chain_end(Tn5250Record * This)
{
   return tn5250_record_length(This) == This->cur_pos;
}

void tn5250_record_dump(Tn5250Record * This)
{
   tn5250_buffer_log (&(This->data),"@record");
   TN5250_LOG (("@eor\n"));
}

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
