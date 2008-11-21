/* TN5250 - An implementation of the 5250 telnet protocol.
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

/****f* lib5250/tn5250_record_new
 * NAME
 *    tn5250_record_new
 * SYNOPSIS
 *    ret = tn5250_record_new ();
 * INPUTS
 *    None
 * DESCRIPTION
 *    DOCUMENT ME!!!
 *****/
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

/****f* lib5250/tn5250_record_destroy
 * NAME
 *    tn5250_record_destroy
 * SYNOPSIS
 *    tn5250_record_destroy (This);
 * INPUTS
 *    Tn5250Record *       This       - 
 * DESCRIPTION
 *    DOCUMENT ME!!!
 *****/
void tn5250_record_destroy(Tn5250Record * This)
{
   if (This != NULL) {
      tn5250_buffer_free (&(This->data));
      free(This);
   }
}

/****f* lib5250/tn5250_record_get_byte
 * NAME
 *    tn5250_record_get_byte
 * SYNOPSIS
 *    ret = tn5250_record_get_byte (This);
 * INPUTS
 *    Tn5250Record *       This       - 
 * DESCRIPTION
 *    DOCUMENT ME!!!
 *****/
unsigned char tn5250_record_get_byte(Tn5250Record * This)
{
   This->cur_pos++;
   TN5250_ASSERT(This->cur_pos <= tn5250_record_length(This));
   return (tn5250_buffer_data (&(This->data)))[This->cur_pos - 1];
}

/****f* lib5250/tn5250_record_unget_byte
 * NAME
 *    tn5250_record_unget_byte
 * SYNOPSIS
 *    tn5250_record_unget_byte (This);
 * INPUTS
 *    Tn5250Record *       This       - 
 * DESCRIPTION
 *    DOCUMENT ME!!!
 *****/
void tn5250_record_unget_byte(Tn5250Record * This)
{
   TN5250_LOG(("Record::UnGetByte: entered.\n"));
   TN5250_ASSERT(This->cur_pos > 0);
   This->cur_pos--;
}

/****f* lib5250/tn5250_record_is_chain_end
 * NAME
 *    tn5250_record_is_chain_end
 * SYNOPSIS
 *    ret = tn5250_record_is_chain_end (This);
 * INPUTS
 *    Tn5250Record *       This       - 
 * DESCRIPTION
 *    DOCUMENT ME!!!
 *****/
int tn5250_record_is_chain_end(Tn5250Record * This)
{
   return tn5250_record_length(This) == This->cur_pos;
}

/****f* lib5250/tn5250_record_is_chain_end
 * NAME
 *    tn5250_record_skip_to_end
 * SYNOPSIS
 *    ret = tn5250_record_skip_to_end (This);
 * INPUTS
 *    Tn5250Record *       This       - 
 * DESCRIPTION
 *    DOCUMENT ME!!!
 *****/
void tn5250_record_skip_to_end(Tn5250Record * This)
{
   This->cur_pos = tn5250_record_length(This);
}

/****f* lib5250/tn5250_record_dump
 * NAME
 *    tn5250_record_dump
 * SYNOPSIS
 *    tn5250_record_dump (This);
 * INPUTS
 *    Tn5250Record *       This       - 
 * DESCRIPTION
 *    DOCUMENT ME!!!
 *****/
void tn5250_record_dump(Tn5250Record * This)
{
   tn5250_buffer_log (&(This->data),"@record");
   TN5250_LOG (("@eor\n"));
}

/****f* lib5250/tn5250_record_list_add
 * NAME
 *    tn5250_record_list_add
 * SYNOPSIS
 *    ret = tn5250_record_list_add (list, record);
 * INPUTS
 *    Tn5250Record *       list       - 
 *    Tn5250Record *       record     - 
 * DESCRIPTION
 *    Add a record to the end of a list of records.
 *****/
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

/****f* lib5250/tn5250_record_list_remove
 * NAME
 *    tn5250_record_list_remove
 * SYNOPSIS
 *    ret = tn5250_record_list_remove (list, record);
 * INPUTS
 *    Tn5250Record *       list       - 
 *    Tn5250Record *       record     - 
 * DESCRIPTION
 *    Remove a record from a list of records.
 *****/
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

/****f* lib5250/tn5250_record_list_destroy
 * NAME
 *    tn5250_record_list_destroy
 * SYNOPSIS
 *    ret = tn5250_record_list_destroy (list);
 * INPUTS
 *    Tn5250Record *       list       - 
 * DESCRIPTION
 *    Destroy all records in a record list and return NULL.
 *****/
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

