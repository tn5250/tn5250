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

/* TODO:
 *    - Check to see if This->next_save_id is used before creating a
 *      new Tn5250TableSaveBuffer.
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

Tn5250Table *tn5250_table_new(Tn5250DBuffer * display)
{
   Tn5250Table *This = tn5250_new(Tn5250Table, 1);
   if (This == NULL)
      return NULL;
   This->curfield = 0;
   This->numfields = 0;
   This->field_list = NULL;
   This->next_save_id = 1;
   This->save_buffers = NULL;
   This->display = display;
   This->MasterMDT = 0;
   This->message_line = 25;
   return This;
}

void tn5250_table_destroy(Tn5250Table * This)
{
   Tn5250TableSaveBuffer *iter, *next;

   (void)tn5250_field_list_destroy(This->field_list);

   if ((iter = This->save_buffers) != NULL) {
      /*@-usereleased@*/
      do {
	 next = iter->next;
	 (void)tn5250_field_list_destroy(iter->fields);
	 free(iter);
	 iter = next;
      } while (iter != This->save_buffers);
      /*@=usereleased@*/
   }

   free (This);
}

void tn5250_table_add_field(Tn5250Table * This, Tn5250Field * field)
{
   field->id = This->numfields++;
   field->table = This;
   This->field_list = tn5250_field_list_add(This->field_list, field);
}

void tn5250_table_set_current_field(Tn5250Table * This, int CurField)
{
   TN5250_LOG(("FormatTable::SetCurField: CurField = %d; This->numfields = %d\n",
	CurField, This->numfields));

   TN5250_ASSERT(CurField == -1 || tn5250_table_field_n(This, CurField) != NULL);
   This->curfield = CurField;
}

int tn5250_table_first_field(Tn5250Table * This)
{
   Tn5250Field *field;
   if (This->numfields > 0) {
      This->curfield = 0;
      field = tn5250_table_field_n(This, This->curfield);
      while (field != NULL && tn5250_field_is_bypass(field)) {
	 if (This->curfield == tn5250_table_field_count(This) - 1) {
	    This->curfield = -1;
	    break;
	 } else
	    This->curfield++;
	 field = tn5250_table_field_n(This, This->curfield);
      }
   } else
      This->curfield = -1;

   return (This->curfield);
}

int tn5250_table_next_field(Tn5250Table * This)
{
   int old_curfield = This->curfield;
   Tn5250Field *field;

   TN5250_LOG(("FormatTable::NextField: This->numfields = %d; This->curfield = %d\n",
	This->numfields, This->curfield));

   TN5250_ASSERT(This->curfield != -1);

   if (This->numfields > 0) {
      do {
	 This->curfield = (This->curfield + 1) % This->numfields;
	 field = tn5250_table_field_n(This, This->curfield);
	 if (field == NULL) {
	    This->curfield = -1;
	    break;
	 }
      } while (tn5250_field_is_bypass(field) && This->curfield != old_curfield);
   } else
      This->curfield = -1;

   return (This->curfield);
}

int tn5250_table_next_field2(Tn5250Table * This, int y, int x)
{
   int nx = x, ny = y;
   Tn5250Field *field;
   if ((This->curfield = tn5250_table_field_number(This, y, x)) >= 0)
      return tn5250_table_next_field(This);

   do {
      if (++nx == tn5250_dbuffer_width(This->display)) {
	 nx = 0;
	 if (++ny == tn5250_dbuffer_height(This->display))
	    ny = 0;
      }
      This->curfield = tn5250_table_field_number(This, ny, nx);
   } while (This->curfield < 0 && !(x == nx && y == ny));

   field = tn5250_table_field_n(This, This->curfield);
   if (field != NULL) {
      if (tn5250_field_is_bypass(field))
	 return tn5250_table_next_field(This);
   } else
      This->curfield = -1;

   return This->curfield;
}

int tn5250_table_prev_field(Tn5250Table * This)
{
   int old_curfield = This->curfield;
   Tn5250Field *field;

   TN5250_LOG(("FormatTable::PrevField: This->numfields = %d; This->curfield = %d\n",
	This->numfields, This->curfield));

   TN5250_ASSERT(This->curfield != -1);

   if (This->numfields > 0) {
      do {
	 if (--This->curfield < 0)
	    This->curfield = This->numfields - 1;
	 field = tn5250_table_field_n(This, This->curfield);
      } while (field != NULL && tn5250_field_is_bypass(field)
	    && This->curfield != old_curfield);
   } else
      This->curfield = -1;

   return (This->curfield);
}

int tn5250_table_prev_field2(Tn5250Table * This, int y, int x)
{
   int nx = x, ny = y;
   Tn5250Field *field;

   if ((This->curfield = tn5250_table_field_number(This, y, x)) >= 0)
      return tn5250_table_prev_field(This);

   do {
      if (--x < 0) {
	 x = tn5250_dbuffer_width(This->display) - 1;
	 if (--y < 0)
	    y = tn5250_dbuffer_height(This->display) - 1;
      }
      This->curfield = tn5250_table_field_number(This, y, x);
   } while (This->curfield < 0 && !(nx == x && ny == y));

   field = tn5250_table_field_n(This, This->curfield);
   if (field != NULL) {
      if (tn5250_field_is_bypass(field))
	 return tn5250_table_prev_field(This);
   } else
      This->curfield = -1;

   return This->curfield;
}

int tn5250_table_field_number(Tn5250Table * This, int y, int x)
{
   Tn5250Field *field;

   field = tn5250_table_field_yx (This, y, x);
   return field ? field->id : -1;
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

int tn5250_table_save(Tn5250Table * This)
{
   Tn5250TableSaveBuffer *buf;
   
   buf = tn5250_new(Tn5250TableSaveBuffer, 1);
   if (buf == NULL)
      return -1; /* Out of memory */

   buf->fields = tn5250_field_list_copy(This->field_list);
   if (buf->fields == NULL && This->field_list != NULL) {
      free (buf);
      return -1; /* Out of memory */
   }

   buf->numfields = This->numfields;

   if (This->save_buffers == NULL)
      This->save_buffers = buf->next = buf->prev = buf;
   else {
      buf->next = This->save_buffers;
      buf->prev = This->save_buffers->prev;
      buf->next->prev = buf;
      buf->prev->next = buf;
   }

   if ((buf->id = ++This->next_save_id) == 256)
      This->next_save_id = buf->id = 0;

   return buf->id;
}

void tn5250_table_restore(Tn5250Table * This, int formatnum)
{
   Tn5250TableSaveBuffer *iter;
   Tn5250Field *fiter;

   This->MasterMDT = 0;
   if ((iter = This->save_buffers) != NULL) {
      do {
	 if (iter->id == formatnum) {

	    /* Restore the fields */
	    (void)tn5250_field_list_destroy(This->field_list);
	    This->field_list = iter->fields;
	    This->numfields = iter->numfields;

	    /* Remove the save buffer from the list, destroy it. */
	    if (iter->next == iter)
	       This->save_buffers = NULL;
	    else {
	       if (iter == This->save_buffers)
		  This->save_buffers = This->save_buffers->next;
	       iter->next->prev = iter->prev;
	       iter->prev->next = iter->next;
	    }
	    free(iter);

	    /* Set the master MDT flag if any of the field MDT flags
	     * are set.  HACK: Setting display of each field to our
	     * display. */
	    if ((fiter = This->field_list) != NULL) {
	       do {
		  fiter->display = This->display;
		  if (tn5250_field_mdt (fiter))
		     This->MasterMDT = 1;
		  fiter = fiter->next;
	       } while (fiter != This->field_list);
	    }
	    
	    return;
	 }
	 iter = iter->next;
      } while (iter != This->save_buffers);
   }
}

void tn5250_table_clear(Tn5250Table * This)
{
   This->field_list = tn5250_field_list_destroy(This->field_list);
   This->numfields = 0;

   This->MasterMDT = 0;
   This->message_line = 24;

   This->curfield = 0;

   TN5250_LOG(("FormatTable::Clear: entered.\n"));
}

void tn5250_table_add_char(Tn5250Table * This, int y, int x, unsigned char Data)
{
   int ins_loc;
   Tn5250Field *field;

   field = tn5250_table_field_yx(This,y,x);
   TN5250_ASSERT(field != NULL);
   TN5250_ASSERT(!tn5250_field_is_bypass (field));

   ins_loc = tn5250_field_count_left(field, y, x);

   tn5250_field_put_char(field, ins_loc, Data);
   tn5250_field_set_mdt(field);
}

void tn5250_table_ins_char(Tn5250Table * This, int y, int x, unsigned char Data)
{
   Tn5250Field *field;
   int fieldnum;
   int ins_loc;
   int shiftcount;

   fieldnum = tn5250_table_field_number(This, y, x);
   field = tn5250_field_list_find_by_id(This->field_list, fieldnum);

   TN5250_ASSERT(field != NULL);
   TN5250_ASSERT(!tn5250_field_is_bypass (field));

   ins_loc = tn5250_field_count_left(field, y, x);
   shiftcount = tn5250_field_count_right(field, y, x);

   TN5250_LOG(("tn5250_table_ins_char: id = %d, ins_loc = %d, shiftcount = %d\n",
	    field->id, ins_loc, shiftcount));

   if (shiftcount > 0) {
      memmove(field->data + ins_loc + 1,
	      field->data + ins_loc, shiftcount);
   }
   tn5250_field_put_char(field, ins_loc, Data);
   tn5250_field_set_mdt(field);
}

void tn5250_table_del_char(Tn5250Table * This, int y, int x)
{
   Tn5250Field *field;
   int fieldnum;
   int ins_loc;
   int shiftcount;

   fieldnum = tn5250_table_field_number(This, y, x);
   field = tn5250_table_field_n(This, fieldnum);

   TN5250_ASSERT(field != NULL);
   TN5250_ASSERT(!tn5250_field_is_bypass (field));

   ins_loc = tn5250_field_count_left(field, y, x);
   shiftcount = tn5250_field_count_right(field, y, x);

   TN5250_LOG(("tn5250_table_del_char: id = %d, ins_loc = %d, shiftcount = %d\n",
	    field->id, ins_loc, shiftcount));

   if (shiftcount > 0) {
      memmove(field->data + ins_loc,
	      field->data + ins_loc + 1, shiftcount);
   }
   field->data[ins_loc + shiftcount] = 0x00;
   tn5250_field_set_mdt(field);
}

int tn5250_table_field_exit(Tn5250Table * This, int y, int x)
{
   Tn5250Field *field;

   This->curfield = tn5250_table_field_number (This, y, x);
   TN5250_ASSERT(This->curfield >= 0);
   field = tn5250_table_field_n (This, This->curfield);
   TN5250_ASSERT(field != NULL);
   TN5250_ASSERT(!tn5250_field_is_bypass (field));

   TN5250_LOG(("FormatTable::FieldExit Current Field: %d, Column %d, Row %d\n",
	This->curfield, x, y));

   /* Print the Information about this field for reference */
   TN5250_LOG(("SR %d, SC %d, ER %d, EC %d, Length %d, CountLeft %d, CountRight %d\n",
	tn5250_field_start_row(field),
	tn5250_field_start_col(field),
	tn5250_field_end_row(field),
	tn5250_field_end_col(field),
	tn5250_field_length(field),
	tn5250_field_count_left(field, y, x),
	tn5250_field_count_right(field, y, x)));
   tn5250_field_dump(field);

   /* Note as you read this code column and Row are numbered as 1 offset
      where as X and Y are 0 offset in the upper left corner */

   /* FIXME: This seems odd... can't we just call tn5250_field_process_adjust? */
   switch (tn5250_field_type(field)) {
   case TN5250_FIELD_ALPHA_SHIFT:
      TN5250_LOG(("Processing Alpha Shift\n"));
      /* accept all characters */
      tn5250_field_process_adjust(field, y, x);
      break;
   case TN5250_FIELD_ALPHA_ONLY:
      TN5250_LOG(("Processing Alpha Only\n"));
      tn5250_field_process_adjust(field, y, x);
      break;
   case TN5250_FIELD_NUM_SHIFT:
      TN5250_LOG(("Processing Numeric Shift\n"));
      tn5250_field_process_adjust(field, y, x);
      break;
   case TN5250_FIELD_NUM_ONLY:
      TN5250_LOG(("Processing Numberic Only\n"));
      tn5250_field_process_adjust(field, y, x);
      break;
   case TN5250_FIELD_KATA_SHIFT:
      TN5250_LOG(("Processing Katakana NOT CODED\n"));
      break;
   case TN5250_FIELD_DIGIT_ONLY:
      TN5250_LOG(("Processing Digit Only\n"));
      tn5250_field_process_adjust(field, y, x);
      break;
   case TN5250_FIELD_MAG_READER:
      TN5250_LOG(("Processing Mag Reader NOT CODED\n"));
      /* FIXME: Implement */
      break;
   case TN5250_FIELD_SIGNED_NUM:
      TN5250_LOG(("Processing Signed Numeric\n"));
      tn5250_field_shift_right_fill(field,
				    tn5250_field_count_left(field, y, x), tn5250_ascii2ebcdic(' '));
      tn5250_field_update_display (field);
   }

   tn5250_field_set_mdt(field);
   return (0);
}

/* vi:set cindent sts=3 sw=3: */
