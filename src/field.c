/* tn5250 -- an implentation of the 5250 telnet protocol.
 * Copyright (C) 1999 Michael Madore
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

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <string.h>

#include "utility.h"
#include "dbuffer.h"
#include "field.h"
#include "formattable.h"

Tn5250Field *tn5250_field_new(int w)
{
   Tn5250Field *This = tn5250_new(Tn5250Field, 1);
   if (This == NULL)
      return NULL;
   memset(This, 0, sizeof(Tn5250Field));
   This->id = -1;
   This->w = w;
   return This;
}

Tn5250Field *tn5250_field_copy(Tn5250Field * This)
{
   Tn5250Field *fld = tn5250_new(Tn5250Field, 1);
   if (fld == NULL)
      return NULL;
   memcpy(fld, This, sizeof(Tn5250Field));
   fld->next = NULL;
   fld->prev = NULL;
   return fld;
}

void tn5250_field_destroy(Tn5250Field * This)
{
   free(This);
}

#ifndef NDEBUG
void tn5250_field_dump(Tn5250Field * This)
{
   int curchar;
   Tn5250Uint16 ffw = This->FFW;

   TN5250_LOG(("tn5250_field_dump: ffw flags = "));
   if ((ffw & TN5250_FIELD_BYPASS) != 0)
      TN5250_LOG(("bypass "));
   if ((ffw & TN5250_FIELD_DUP_ENABLE) != 0)
      TN5250_LOG(("dup-enable "));
   if ((ffw & TN5250_FIELD_MODIFIED) != 0)
      TN5250_LOG(("mdt "));
   if ((ffw & TN5250_FIELD_AUTO_ENTER) != 0)
      TN5250_LOG(("auto-enter"));
   if ((ffw & TN5250_FIELD_FER) != 0)
      TN5250_LOG(("fer "));
   if ((ffw & TN5250_FIELD_MONOCASE) != 0)
      TN5250_LOG(("monocase "));
   if ((ffw & TN5250_FIELD_MANDATORY) != 0)
      TN5250_LOG(("mandatory "));
   TN5250_LOG(("\ntn5250_field_dump: type = %s\n", tn5250_field_description (This)));
   TN5250_LOG(("tn5250_field_dump: adjust = %s\ntn5250_field_dump: data = ", tn5250_field_adjust_description (This)));
   
   TN5250_LOG(("\n"));
}
#endif

/*
 *    Determine if the screen position at row ``y'', column ``x'' is contained
 *    within this field.  (A hit-test, in other words.)
 */
int tn5250_field_hit_test(Tn5250Field * This, int y, int x)
{
   int pos = (y * This->w) + x;

   return (pos >= tn5250_field_start_pos(This)
	   && pos <= tn5250_field_end_pos(This));
}

/*
 *    Figure out the starting address of this field.
 */
int tn5250_field_start_pos(Tn5250Field * This)
{
   return This->start_row * This->w +
       This->start_col;
}

/*
 *    Figure out the ending address of this field.
 */
int tn5250_field_end_pos(Tn5250Field * This)
{
   return tn5250_field_start_pos(This) + tn5250_field_length(This) - 1;
}

/*
 *    Figure out the ending row of this field.
 */
int tn5250_field_end_row(Tn5250Field * This)
{
   return tn5250_field_end_pos(This) / This->w;
}

/*
 *    Figure out the ending column of this field.
 */
int tn5250_field_end_col(Tn5250Field * This)
{
   return tn5250_field_end_pos(This) % This->w;
}

/*
 *    Get a description of this field.
 */
const char *tn5250_field_description(Tn5250Field * This)
{
   switch (This->FFW & TN5250_FIELD_FIELD_MASK) {
   case TN5250_FIELD_ALPHA_SHIFT:
      return "Alpha Shift";
   case TN5250_FIELD_DUP_ENABLE:
      return "Dup Enabled";
   case TN5250_FIELD_ALPHA_ONLY:
      return "Alpha Only";
   case TN5250_FIELD_NUM_SHIFT:
      return "Numeric Shift";
   case TN5250_FIELD_NUM_ONLY:
      return "Numeric Only";
   case TN5250_FIELD_KATA_SHIFT:
      return "Katakana";
   case TN5250_FIELD_DIGIT_ONLY:
      return "Digits Only";
   case TN5250_FIELD_MAG_READER:
      return "Mag Reader I/O Field";
   case TN5250_FIELD_SIGNED_NUM:
      return "Signed Numeric";
   default:
      return "(?)";
   }
}

/*
 *    Get a description of the mandatory fill mode for this field.
 */
const char *tn5250_field_adjust_description (Tn5250Field * This)
{
   switch (This->FFW & TN5250_FIELD_MAND_FILL_MASK) {
   case TN5250_FIELD_NO_ADJUST:
      return "No Adjust";
   case TN5250_FIELD_MF_RESERVED_1:
      return "Reserved 1";
   case TN5250_FIELD_MF_RESERVED_2:
      return "Reserved 2";
   case TN5250_FIELD_MF_RESERVED_3:
      return "Reserved 3";
   case TN5250_FIELD_MF_RESERVED_4:
      return "Reserved 4";
   case TN5250_FIELD_RIGHT_ZERO:
      return "Right Adjust, Zero Fill";
   case TN5250_FIELD_RIGHT_BLANK:
      return "Right Adjust, Blank Fill";
   case TN5250_FIELD_MANDATORY_FILL:
      return "Mandatory Fill";
   default:
      return "";
   }
}

/*
 *    Return the number of characters in the this field which
 *    are to the left oof the specified cursor position.  Used
 *    as an index to insert data when the user types.
 */
int tn5250_field_count_left(Tn5250Field *This, int y, int x)
{
   int pos;

   pos = (y * This->w + x);
   pos -= tn5250_field_start_pos(This);

   TN5250_ASSERT (tn5250_field_hit_test(This, y, x));
   TN5250_ASSERT (pos >= 0);
   TN5250_ASSERT (pos < tn5250_field_length(This));

   return pos;
}

/*
 *    This returns the number of characters in the specified field
 *    which are to the right of the specified cursor position.
 */
int tn5250_field_count_right (Tn5250Field *This, int y, int x)
{
   TN5250_ASSERT(tn5250_field_hit_test(This, y, x));
   return tn5250_field_end_pos (This) - (y * This->w + x);
}

/*
 *    Determine if the supplied character is a valid data character
 *    for this field.
 */
int tn5250_field_valid_char (Tn5250Field *field, int ch)
{
   TN5250_LOG(("HandleKey: fieldtype = %d\n", tn5250_field_type (field)));
   switch (tn5250_field_type (field)) {
   case TN5250_FIELD_ALPHA_SHIFT:
      return 1;

   case TN5250_FIELD_ALPHA_ONLY:
      return (isalpha(ch) ||
	  ch == ',' ||
	  ch == '.' ||
	  ch == '-' ||
	  ch == ' ');

   case TN5250_FIELD_NUM_SHIFT:
      return 1;

   case TN5250_FIELD_NUM_ONLY:
      return (tn5250_isnumeric(ch) ||
	  ch == '+' ||
	  ch == ',' ||
	  ch == '.' ||
	  ch == '-' ||
	  ch == ' ');

   case TN5250_FIELD_KATA_SHIFT:
      TN5250_LOG(("KATAKANA not implemneted.\n"));
      return 1;

   case TN5250_FIELD_DIGIT_ONLY:
      return isdigit (ch);

   case TN5250_FIELD_MAG_READER:
      TN5250_LOG(("MAG_READER not implemneted.\n"));
      return 1;

   case TN5250_FIELD_SIGNED_NUM:
      return (isdigit(ch) ||
	  ch == '+' ||
	  ch == '-');
   }
   return 0;
}

/*
 *    Set the MDT flag for this field and for the table which owns it.
 */
void tn5250_field_set_mdt (Tn5250Field *This)
{
   TN5250_ASSERT(This->table != NULL);

   This->FFW |= TN5250_FIELD_MODIFIED;
   tn5250_table_set_mdt(This->table);
}

/*
 *    Destroy all fields in a field list.
 */
Tn5250Field *tn5250_field_list_destroy(Tn5250Field * list)
{
   Tn5250Field *iter, *next;
   if ((iter = list) != NULL) {
      /*@-usereleased@*/
      do {
	 next = iter->next;
	 tn5250_field_destroy(iter);
	 iter = next;
      } while (iter != list);
      /*@=usereleased@*/
   }
   return NULL;
}

/*
 *    Add a field to the end of a list of fields.
 */
Tn5250Field *tn5250_field_list_add(Tn5250Field * list, Tn5250Field * node)
{
   node->prev = node->next = NULL;

   if (list == NULL) {
      node->next = node->prev = node;
      return node;
   }
   node->next = list;
   node->prev = list->prev;
   node->prev->next = node;
   node->next->prev = node;
   return list;
}

/*
 *    Remove a field from a list of fields.
 */
Tn5250Field *tn5250_field_list_remove(Tn5250Field * list, Tn5250Field * node)
{
   if (list == NULL)
      return NULL;
   if (list->next == list && list == node) {
      node->next = node->prev = NULL;
      return NULL;
   }
   if (list == node)
      list = list->next;

   node->next->prev = node->prev;
   node->prev->next = node->next;
   node->prev = node->next = NULL;
   return list;
}

/*
 *    Find a field by its numeric id.
 */
Tn5250Field *tn5250_field_list_find_by_id(Tn5250Field * list, int id)
{
   Tn5250Field *iter;

   if ((iter = list) != NULL) {
      do {
	 if (iter->id == id)
	    return iter;
	 iter = iter->next;
      } while (iter != list);
   }
   return NULL;
}

/*
 *    Copy all fields in a list to another list.
 */
Tn5250Field *tn5250_field_list_copy(Tn5250Field * This)
{
   Tn5250Field *new_list = NULL, *iter, *new_field;
   if ((iter = This) != NULL) {
      do {
	 new_field = tn5250_field_copy(iter);
	 if (new_field != NULL)
	    new_list = tn5250_field_list_add(new_list, new_field);
	 iter = iter->next;
      } while (iter != This);
   }
   return new_list;
}

/* vi:set cindent sts=3 sw=3: */
