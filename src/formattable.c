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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "utility.h"
#include "dbuffer.h"
#include "field.h"
#include "formattable.h"
#include "ctype.h"
#include "codes5250.h"

Tn5250Table *tn5250_table_new()
{
   Tn5250Table *This = tn5250_new(Tn5250Table, 1);
   if (This == NULL)
      return NULL;
   This->numfields = 0;
   This->field_list = NULL;
   This->MasterMDT = 0;
   This->message_line = 25;
   This->next = This->prev = NULL;
   return This;
}

Tn5250Table *tn5250_table_copy(Tn5250Table *table)
{
   Tn5250Field *iter;
   Tn5250Table *This = tn5250_new(Tn5250Table, 1);
   if (This == NULL)
      return NULL;
   memcpy(This,table,sizeof(Tn5250Table));
   This->next = This->prev = NULL;
   This->field_list = tn5250_field_list_copy (table->field_list);
   return This;
}

void tn5250_table_destroy(Tn5250Table * This)
{
   (void)tn5250_field_list_destroy(This->field_list);
   free (This);
}

void tn5250_table_add_field(Tn5250Table * This, Tn5250Field * field)
{
   field->id = This->numfields++;
   field->table = This;
   This->field_list = tn5250_field_list_add(This->field_list, field);
}

Tn5250Field *tn5250_table_field_yx (Tn5250Table *This, int y, int x)
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

void tn5250_table_clear(Tn5250Table * This)
{
   This->field_list = tn5250_field_list_destroy(This->field_list);
   This->numfields = 0;

   This->MasterMDT = 0;
   This->message_line = 24;

   TN5250_LOG(("FormatTable::Clear: entered.\n"));
}

/* vi:set cindent sts=3 sw=3: */
