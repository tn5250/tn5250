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
#include "formattable.h"
#include "display.h"
#include "terminal.h"
#include "buffer.h"
#include "record.h"
#include "stream.h"
#include "session.h"
#include "codes5250.h"
#include "wtd.h"

static void tn5250_display_add_dbuffer(Tn5250Display * display,
				       Tn5250DBuffer * dbuffer);
static void tn5250_display_add_table(Tn5250Display * This,
				     Tn5250Table * table);

Tn5250Display *tn5250_display_new()
{
   Tn5250Display *This;

   if ((This = tn5250_new(Tn5250Display, 1)) == NULL)
      return NULL;
   This->display_buffers = NULL;
   This->format_tables = NULL;
   This->terminal = NULL;
   This->indicators = 0;
   This->indicators_dirty = 0;
   This->pending_insert = 0;
   This->session = NULL;

   tn5250_display_add_dbuffer(This, tn5250_dbuffer_new(80, 24));
   tn5250_display_add_table(This, tn5250_table_new());
   return This;
}

void tn5250_display_destroy(Tn5250Display * This)
{
   Tn5250DBuffer *diter, *dnext;
   Tn5250Table *titer, *tnext;

   if ((diter = This->display_buffers) != NULL) {
      do {
	 dnext = diter->next;
	 tn5250_dbuffer_destroy(diter);
	 diter = dnext;
      } while (diter != This->display_buffers);
   }
   if ((titer = This->format_tables) != NULL) {
      do {
	 tnext = titer->next;
	 tn5250_table_destroy(titer);
	 titer = tnext;
      } while (titer != This->format_tables);
   }
   if (This->terminal != NULL)
      tn5250_terminal_destroy(This->terminal);

   free(This);
}

void tn5250_display_set_session(Tn5250Display *This, struct _Tn5250Session *s)
{
   This->session = s;
   if (This->session != NULL)
      This->session->display = This;
}

/*
 *    Create a new display buffer and assign the old one an id so we can
 *    later restore it.  Return the id which must be > 0.
 */
Tn5250DBuffer *tn5250_display_push_dbuffer(Tn5250Display * This)
{
   Tn5250DBuffer *dbuf;

   dbuf = tn5250_dbuffer_copy(This->display_buffers);
   tn5250_display_add_dbuffer(This, dbuf);
   return dbuf;	/* Pointer is used as unique identifier in data stream. */
}

/*
 *    Create a new format table and assign the old one an id so we can
 *    later restore it.  Return the id which must be > 0.
 */
Tn5250Table *tn5250_display_push_table(Tn5250Display * This)
{
   Tn5250Table *table;

   table = tn5250_table_copy(This->format_tables);
   tn5250_display_add_table(This, table);
   return table; /* Pointer is used as unique identifier in data stream. */
}

/*
 *    Delete the current dbuffer and replace it with the one with id `id'.
 */
void tn5250_display_restore_dbuffer(Tn5250Display * This, Tn5250DBuffer * id)
{
   Tn5250DBuffer *iter;

   /* Sanity check to make sure that the display buffer is for real and
    * that it isn't the one which is currently active. */
   if ((iter = This->display_buffers) != NULL) {
      do {
	 if (iter == id && iter != This->display_buffers)
	    break;
	 iter = iter->next;
      } while (iter != This->display_buffers);

      if (iter != id || iter == This->display_buffers)
	 return;
   } else
      return;

   This->display_buffers->prev->next = This->display_buffers->next;
   This->display_buffers->next->prev = This->display_buffers->prev;
   tn5250_dbuffer_destroy(This->display_buffers);
   This->display_buffers = iter;
}

/*
 *    Delete the current table and replace it with the one with id `id'.
 */
void tn5250_display_restore_table(Tn5250Display * This, Tn5250Table * id)
{
   Tn5250Table *iter;

   /* Sanity check to make sure that the format table is for real and
    * that it isn't the one which is currently active. */
   if ((iter = This->format_tables) != NULL) {
      do {
	 if (iter == id && iter != This->format_tables)
	    break;
	 iter = iter->next;
      } while (iter != This->format_tables);

      if (iter != id || iter == This->format_tables)
	 return;
   } else
      return;

   This->format_tables->prev->next = This->format_tables->next;
   This->format_tables->next->prev = This->format_tables->prev;
   tn5250_table_destroy(This->format_tables);
   This->format_tables = iter;
}

/*
 *    Add a display buffer into this display's circularly linked list of
 *    display buffers.
 */
static void tn5250_display_add_dbuffer(Tn5250Display * This, Tn5250DBuffer * dbuffer)
{
   TN5250_ASSERT(dbuffer != NULL);

   if (This->display_buffers == NULL) {
      This->display_buffers = dbuffer;
      dbuffer->next = dbuffer->prev = dbuffer;
   } else {
      dbuffer->next = This->display_buffers;
      dbuffer->prev = This->display_buffers->prev;
      dbuffer->next->prev = dbuffer;
      dbuffer->prev->next = dbuffer;
   }
}

/*
 *    Add a format table into this display's circularly linked list of
 *    format tables.
 */
static void tn5250_display_add_table(Tn5250Display * This, Tn5250Table * table)
{
   TN5250_ASSERT(table != NULL);

   if (This->format_tables == NULL) {
      This->format_tables = table;
      table->next = table->prev = table;
   } else {
      table->next = This->format_tables;
      table->prev = This->format_tables->prev;
      table->next->prev = table;
      table->prev->next = table;
   }
}

/*
 *    Set the terminal associated with this display.
 */
void tn5250_display_set_terminal(Tn5250Display * This, Tn5250Terminal * term)
{
   if (This->terminal != NULL)
      tn5250_terminal_destroy(This->terminal);
   This->terminal = term;
   This->indicators_dirty = 1;
   tn5250_terminal_update(This->terminal, This);
   tn5250_terminal_update_indicators(This->terminal, This);
}

/*
 *    Update the terminal's representation of the display.
 */
void tn5250_display_update(Tn5250Display * This)
{
   if (This->terminal != NULL) {
      tn5250_terminal_update(This->terminal, This);
      if (This->indicators_dirty) {
	 tn5250_terminal_update_indicators(This->terminal, This);
	 This->indicators_dirty = 0;
      }
   }
}

/*
 *    Wait for a terminal event.  FIXME: Handle keystrokes while we're at it.
 */
int tn5250_display_waitevent(Tn5250Display * This)
{
   if (This->terminal == NULL)
      return 0;
   return tn5250_terminal_waitevent(This->terminal);
}

/*
 *    Get the next keystroke in the keyboard buffer.
 */
int tn5250_display_getkey(Tn5250Display * This)
{
   if (This->terminal == NULL)
      return -1;
   return tn5250_terminal_getkey(This->terminal);
}

/*
 *    The required beep function.
 */
void tn5250_display_beep(Tn5250Display * This)
{
   if (This->terminal == NULL)
      return;
   tn5250_terminal_beep(This->terminal);
}

/*
 *    Set the pending insert flag and the insert cursor position.
 */
void tn5250_display_set_pending_insert (Tn5250Display *This, int y, int x)
{
   This->pending_insert = 1;
   tn5250_dbuffer_set_ic (This->display_buffers, y, x);
}

Tn5250Field *tn5250_display_field_at(Tn5250Display *This, int y, int x)
{
   return tn5250_table_field_yx(This->format_tables, y, x);
}

/*
 *    Find the field currently under the cursor.  The answer might be NULL
 *    if there is no field under the cursor.
 */
Tn5250Field *tn5250_display_current_field(Tn5250Display * This)
{
   return tn5250_display_field_at(This,
				tn5250_display_cursor_y(This),
				tn5250_display_cursor_x(This));
}

/*
 *    Return a pointer to the next field after the current one which is not
 *    a bypass field.
 */
Tn5250Field *tn5250_display_next_field(Tn5250Display * This)
{
   Tn5250Field *iter = NULL, *next;
   int y, x;

   y = tn5250_display_cursor_y (This);
   x = tn5250_display_cursor_x (This);

   iter = tn5250_display_field_at (This, y, x);
   if (iter == NULL) {
      /* Find the first field on the display after the cursor, wrapping if we
       * hit the bottom of the display. */
      while (iter == NULL) {
	 if ((iter = tn5250_display_field_at(This, y, x)) == NULL) {
	    if (++x == tn5250_dbuffer_width(This->display_buffers)) {
	       x = 0;
	       if (++y == tn5250_dbuffer_height(This->display_buffers))
		  y = 0;
	    }
	    if (y == tn5250_display_cursor_y (This) &&
		x == tn5250_display_cursor_x (This))
	       return NULL;	/* No fields on display */
	 }
      }
   } else
      iter = iter->next;

   next = iter;
   while (tn5250_field_is_bypass(next)) {
      next = next->next;	/* Hehe */
      if (next == iter && tn5250_field_is_bypass(next))
	 return NULL;		/* No non-bypass fields. */
   }

   return next;
}

/*
 *    Return a pointer to the first preceding field which is not a bypass
 *    field.
 */
Tn5250Field *tn5250_display_prev_field(Tn5250Display * This)
{
   Tn5250Field *iter = NULL, *prev;
   int y, x;

   y = tn5250_display_cursor_y(This);
   x = tn5250_display_cursor_x(This);

   iter = tn5250_display_field_at(This, y, x);
   if (iter == NULL) {
      /* Find the first field on the display after the cursor, wrapping if we
       * hit the bottom of the display. */
      while (iter == NULL) {
	 if ((iter = tn5250_display_field_at(This, y, x)) == NULL) {
	    if (x-- == 0) {
	       x = tn5250_dbuffer_width(This->display_buffers) - 1;
	       if (y-- == 0)
		  y = tn5250_dbuffer_height(This->display_buffers) - 1;
	    }
	    if (y == tn5250_display_cursor_y(This) &&
		x == tn5250_display_cursor_x(This))
	       return NULL;	/* No fields on display */
	 }
      }
   } else
      iter = iter->prev;

   prev = iter;
   while (tn5250_field_is_bypass(prev)) {
      prev = prev->prev;	/* Hehe */
      if (prev == iter && tn5250_field_is_bypass(prev))
	 return NULL;		/* No non-bypass fields. */
   }

   return prev;
}

/*
 *    Set the cursor to the home position on the current display buffer.
 */
void tn5250_display_set_cursor_home(Tn5250Display * This)
{
   if (This->pending_insert)
      tn5250_dbuffer_goto_ic(This->display_buffers);
   else
      tn5250_display_set_cursor(This, 0, 0);
}

/*
 *    Set the cursor position on the current display buffer to the home
 *    position of the specified field.
 */
void tn5250_display_set_cursor_field(Tn5250Display * This, Tn5250Field * field)
{
   if (field == NULL) {
      tn5250_display_set_cursor_home(This);
      return;
   }
   tn5250_dbuffer_cursor_set(This->display_buffers,
			     tn5250_field_start_row(field),
			     tn5250_field_start_col(field));
}

/*
 *    Move the cursor to the next non-bypass field.  This will move the
 *    cursor to the home position if there are no non-bypass fields.
 */
void tn5250_display_set_cursor_next_field(Tn5250Display * This)
{
   Tn5250Field *field = tn5250_display_next_field(This);
   tn5250_display_set_cursor_field(This, field);
}

/*
 *    Move the cursor to the previous non-bypass field.  This will move the
 *    cursor to the home position if there are no non-bypass fields.
 */
void tn5250_display_set_cursor_prev_field(Tn5250Display * This)
{
   Tn5250Field *field = tn5250_display_prev_field(This);
   tn5250_display_set_cursor_field(This, field);
}

/*
 *    Reconstruct WTD data as it might be sent from a host.  We use this
 *    to save our format table and display buffer.  We assume the buffer
 *    has been initialized, and we append to it.
 */
void tn5250_display_make_wtd_data (Tn5250Display *This, Tn5250Buffer *buf,
      Tn5250DBuffer *src_dbuffer, Tn5250Table *src_table)
{
   Tn5250WTDContext *ctx;

   if ((ctx = tn5250_wtd_context_new (buf, src_dbuffer, src_table, 
	       This->display_buffers, This->format_tables)) == NULL)
      return;

   tn5250_wtd_context_convert (ctx);
   tn5250_wtd_context_destroy (ctx);
}

/*
 *    Add a character to the display, and set the field's MDT flag.  This
 *    is meant to be called by the keyboard handler when the user is
 *    entering data.
 */
void tn5250_display_interactive_addch(Tn5250Display * This, unsigned char ch)
{
   Tn5250Field *field = tn5250_display_current_field(This);
   int end_of_field = 0;

   if (field == NULL || tn5250_field_is_bypass(field)) {
      tn5250_display_inhibit(This);
      return;
   }
   /* Upcase the character if this is a monocase field. */
   if (tn5250_field_is_monocase(field) && isalpha(ch))
      ch = toupper(ch);

   /* '+' and '-' keys activate field exit/field minus for numeric fields. */
   if (tn5250_field_is_num_only(field) || tn5250_field_is_signed_num(field)) {
      switch (ch) {
      case '+':
	 tn5250_display_kf_field_plus(This);
	 return;

      case '-':
	 tn5250_display_kf_field_minus(This);
	 return;
      }
   }
   /* Make sure this is a valid data character for this field type. */
   if (!tn5250_field_valid_char(field, ch)) {
      TN5250_LOG (("Inhibiting: invalid character for field type.\n"));
      tn5250_display_inhibit(This);
      return;
   }
   /* Are we at the last character of the field? */
   if (tn5250_display_cursor_y(This) == tn5250_field_end_row(field) &&
       tn5250_display_cursor_x(This) == tn5250_field_end_col(field))
      end_of_field = 1;

   /* Don't allow the user to enter data in the sign portion of a signed
    * number field. */
   if (end_of_field && tn5250_field_is_signed_num(field)) {
      TN5250_LOG (("Inhibiting: last character of signed num field.\n"));
      tn5250_display_inhibit(This);
      return;
   }
   /* Add or insert the character (depending on whether insert mode is on). */
   if ((tn5250_display_indicators(This) & TN5250_DISPLAY_IND_INSERT) != 0) {
      int ofs = tn5250_field_length(field) - 1;
      unsigned char *data = tn5250_display_field_data (This, field);
      if (tn5250_field_is_signed_num(field))
	 ofs--;
      if (data[ofs] != '\0' && tn5250_ebcdic2ascii(data[ofs]) != ' ') {
	 tn5250_display_inhibit(This);
	 return;
      }
      tn5250_dbuffer_ins(This->display_buffers, tn5250_ascii2ebcdic(ch),
	    tn5250_field_count_right(field,
	       tn5250_display_cursor_y(This),
	       tn5250_display_cursor_x(This)));
   } else
      tn5250_dbuffer_addch(This->display_buffers, tn5250_ascii2ebcdic(ch));

   tn5250_field_set_mdt(field);

   /* If at the end of the field and not a fer field, advance to the
    * next field. */
   if (end_of_field) {
      if (tn5250_field_is_fer(field))
	 tn5250_display_set_cursor (This,
				   tn5250_field_end_row(field),
				   tn5250_field_end_col(field));
      else {
	 tn5250_display_field_adjust(This, field);
	 if (tn5250_field_is_auto_enter(field)) {
	    tn5250_display_do_aidkey (This, TN5250_SESSION_AID_ENTER);
	    return;
	 }
	 tn5250_display_set_cursor_next_field(This);
      }
   }
}

/*
 *    Return a pointer into the current display buffer where the data for the
 *    specified field begins.
 */
unsigned char *tn5250_display_field_data(Tn5250Display * This, Tn5250Field * field)
{
   if (This->display_buffers == NULL)
      return NULL;
   return &This->display_buffers->data[
			    field->start_row * This->display_buffers->w +
					 field->start_col
       ];
}

/*
 *    Move all the data characters in the field to the right-hand side of
 *    the field and left-fill the field with `fill' characters.
 */
void tn5250_display_shift_right(Tn5250Display * This, Tn5250Field * field, unsigned char fill)
{
   int n, end;
   unsigned char *ptr;

   ptr = tn5250_display_field_data(This, field);
   end = tn5250_field_length(field) - 1;

   tn5250_field_set_mdt(field);

   /* Don't adjust the sign position of signed num type fields. */
   if (tn5250_field_is_signed_num(field))
      end--;

   /* Left fill the field until the first non-null or non-blank character. */
   for (n = 0; n <= end && (ptr[n] == 0 || ptr[n] == 0x40); n++)
      ptr[n] = fill;

   /* If the field is entirely blank and we don't do this, we spin forever. */
   if (n > end)
      return;

   /* Shift the contents of the field right one place and put the fill char in
    * position 0 until the data is right-justified in the field. */
   while (ptr[end] == 0 || ptr[end] == 0x40) {
      for (n = end; n > 0; n--)
	 ptr[n] = ptr[n - 1];
      ptr[0] = fill;
   }
}

/*
 *    Adjust the field data as required by the Field Format Word.  This is
 *    called from tn5250_display_kf_field_exit.
 */
void tn5250_display_field_adjust(Tn5250Display * This, Tn5250Field * field)
{
   int mand_fill_type;

   /* Because of special processing during transmit and other weirdness,
    * we need to shift signed number fields right regardless.
    * (num only too?) */
   mand_fill_type = tn5250_field_mand_fill_type(field);
   if (tn5250_field_type(field) == TN5250_FIELD_SIGNED_NUM ||
	 tn5250_field_type(field) == TN5250_FIELD_NUM_ONLY)
      mand_fill_type = TN5250_FIELD_RIGHT_BLANK;

   switch (mand_fill_type) {
   case TN5250_FIELD_NO_ADJUST:
   case TN5250_FIELD_MANDATORY_FILL:
      break;
   case TN5250_FIELD_RIGHT_ZERO:
      tn5250_display_shift_right(This, field, tn5250_ascii2ebcdic('0'));
      break;
   case TN5250_FIELD_RIGHT_BLANK:
      tn5250_display_shift_right(This, field, tn5250_ascii2ebcdic(' '));
      break;
   }

   tn5250_field_set_mdt(field);
}

/*
 *    Translate a keystroke and handle it.
 */
void tn5250_display_do_key(Tn5250Display *This, int key)
{
   TN5250_LOG (("@key %d\n", key));

   /* FIXME: Translate from terminal key via keyboard map to 5250 key. */

   if (tn5250_display_inhibited(This)) {
      if (key != K_SYSREQ && key != K_RESET) {
	 tn5250_display_beep (This);
	 return;
      }
   }

   switch (key) {
   case K_RESET:
      tn5250_display_uninhibit(This);
      break;

   case K_BACKSPACE:
   case K_LEFT:
      tn5250_display_kf_left (This);
      break;

   case K_RIGHT:
      tn5250_display_kf_right (This);
      break;

   case K_UP:
      tn5250_display_kf_up (This);
      break;

   case K_DOWN:
      tn5250_display_kf_down (This);
      break;

   case K_HELP:
      tn5250_display_do_aidkey (This, TN5250_SESSION_AID_HELP);
      break;

   case K_HOME:
      tn5250_display_kf_home (This);
      break;

   case K_END:
      tn5250_display_kf_end (This);
      break;

   case K_DELETE:
      tn5250_display_kf_delete (This);
      break;

   case K_INSERT:
      tn5250_display_kf_insert (This);
      break;

   case K_TAB:
      tn5250_display_kf_tab (This);
      break;

   case K_BACKTAB:
      tn5250_display_kf_backtab (This);
      break;

   case K_ENTER:
      tn5250_display_do_aidkey (This, TN5250_SESSION_AID_ENTER);
      break;

   case K_ROLLDN:
      tn5250_display_do_aidkey (This, TN5250_SESSION_AID_PGUP);
      break;

   case K_ROLLUP:
      tn5250_display_do_aidkey (This, TN5250_SESSION_AID_PGDN);
      break;

   case K_FIELDEXIT:
      tn5250_display_kf_field_exit (This);
      break;

   case K_FIELDMINUS:
      tn5250_display_kf_field_minus (This);
      break;

   case K_SYSREQ:
      tn5250_display_do_aidkey (This, TN5250_SESSION_AID_SYSREQ);
      break;

   case K_ATTENTION:
      tn5250_display_do_aidkey (This, TN5250_SESSION_AID_ATTN);
      break;

   case K_PRINT:
      tn5250_display_do_aidkey (This, TN5250_SESSION_AID_PRINT);
      break;

   case K_DUPLICATE:
      tn5250_display_kf_dup(This);
      break;

   default:
      /* Handle function/command keys. */
      if (key >= K_F1 && key <= K_F24) {
	 if (key <= K_F12) {
	    TN5250_LOG (("Key = %d; K_F1 = %d; Key - K_F1 = %d\n",
		     key, K_F1, key-K_F1));
	    TN5250_LOG (("AID_F1 = %d; Result = %d\n",
		     TN5250_SESSION_AID_F1, key-K_F1+TN5250_SESSION_AID_F1));
	    tn5250_display_do_aidkey (This, key-K_F1+TN5250_SESSION_AID_F1);
	 } else
	    tn5250_display_do_aidkey (This, key-K_F13+TN5250_SESSION_AID_F13);
	 break;
      }

      /* Handle data keys. */
      if (key >= ' ' && key <= 255) {
	 TN5250_LOG(("HandleKey: key = %c\n", key));
	 tn5250_display_interactive_addch (This, key);
      } else {
	 TN5250_LOG (("HandleKey: Weird key ignored: %d\n", key));
      }
   }
}

/*
 *    Process a field exit function.
 */
void tn5250_display_kf_field_exit(Tn5250Display * This)
{
   Tn5250Field *field;
   unsigned char *data;
   int i;

   field = tn5250_display_current_field(This);
   if (field == NULL || tn5250_field_is_bypass(field)) {
      tn5250_display_inhibit(This);
      return;
   }

   /* NUL out remainder of field from cursor position. */
   data = tn5250_display_field_data (This, field);
   i = tn5250_field_count_left(field, tn5250_display_cursor_y(This),
	 tn5250_display_cursor_x(This));
   for (; i < tn5250_field_length(field); i++)
      data[i] = 0;

   tn5250_display_field_adjust(This, field);

   if (tn5250_field_is_auto_enter(field)) {
      tn5250_display_do_aidkey (This, TN5250_SESSION_AID_ENTER);
      return;
   }

   tn5250_display_set_cursor_next_field (This);
}

/*
 *    Process a field plus function.
 */
void tn5250_display_kf_field_plus(Tn5250Display * This)
{
   Tn5250Field *field;
   unsigned char *data;
   unsigned char c;

   TN5250_LOG (("Field+ entered.\n"));
   
   field = tn5250_display_current_field (This);
   if (field == NULL || 
	 (tn5250_field_type(field) != TN5250_FIELD_SIGNED_NUM) &&
	 (tn5250_field_type(field) != TN5250_FIELD_NUM_ONLY)) {
      tn5250_display_inhibit(This);
      return;
   }

   tn5250_display_kf_field_exit(This);

   /* For numeric only fields, we change the zone of the last digit if
    * field plus is pressed.  For signed numeric fields, we change the
    * sign position to a '+'. */
   data = tn5250_display_field_data (This, field);
   if (tn5250_field_type(field) == TN5250_FIELD_NUM_ONLY) {
      c = data[tn5250_field_length (field) - 1];
      if (isdigit (tn5250_ebcdic2ascii (c)))
	 data[tn5250_field_length (field) - 1] = (c & 0x0f) | 0xf0;
   } else
      data[tn5250_field_length (field) - 1] = 0;
}

/*
 *    Process a field minus function.
 */
void tn5250_display_kf_field_minus(Tn5250Display * This)
{
   Tn5250Field *field;
   unsigned char *data;
   unsigned char c;

   TN5250_LOG (("Field- entered.\n"));
   
   field = tn5250_display_current_field (This);
   if (field == NULL || 
	 (tn5250_field_type(field) != TN5250_FIELD_SIGNED_NUM) &&
	 (tn5250_field_type(field) != TN5250_FIELD_NUM_ONLY)) {
      tn5250_display_inhibit(This);
      return;
   }

   tn5250_display_kf_field_exit(This);

   /* For numeric only fields, we change the zone of the last digit if
    * field minus is pressed.  For signed numeric fields, we change the
    * sign position to a '-'. */
   data = tn5250_display_field_data (This, field);
   if (tn5250_field_type(field) == TN5250_FIELD_NUM_ONLY) {
      c = data[tn5250_field_length (field) - 1];
      if (isdigit (tn5250_ebcdic2ascii (c)))
	 data[tn5250_field_length (field) - 1] = (c & 0x0f) | 0xd0;
   } else
      data[tn5250_field_length (field) - 1] = tn5250_ascii2ebcdic('-');
}

/*
 *    Handle an aid code.
 */
void tn5250_display_do_aidkey (Tn5250Display *This, int aidcode)
{
   /* FIXME: We should check to see if this aidkey is currently enabled...
    * or we might just want to leave that to the session to handle. */
   /* FIXME: If this returns zero, we need to stop processing. */
   TN5250_LOG (("tn5250_display_do_aidkey (0x%02X) called.\n", aidcode));

   ( *(This->session->handle_aidkey)) (This->session, aidcode);
}

/*
 *    Process a DUP key function.
 */
void tn5250_display_kf_dup(Tn5250Display * This)
{
   int y, x, i;
   Tn5250Field *field;
   unsigned char *data;

   field = tn5250_display_current_field (This);
   if (field == NULL || tn5250_field_is_bypass (field)) {
      tn5250_display_inhibit(This);
      return;
   }

   tn5250_field_set_mdt(field);

   /* Hmm, should we really go to operator error mode when operator
    * hits Dup in a non-Dupable field? */
   if (!tn5250_field_is_dup_enable(field)) {
      tn5250_display_inhibit(This);
      return;
   }

   i = tn5250_field_count_left(field, tn5250_display_cursor_y(This),
	 tn5250_display_cursor_x(This));
   data = tn5250_display_field_data (This, field);
   for (; i < tn5250_field_length(field); i++)
      data[i] = 0x1c;

   if (tn5250_field_is_fer (field))
      tn5250_dbuffer_cursor_set (This->display_buffers,
	    tn5250_field_end_row (field),
	    tn5250_field_end_col (field));
   else {
      tn5250_display_field_adjust (This, field);
      if (tn5250_field_is_auto_enter (field)) {
	 tn5250_display_do_aidkey (This, TN5250_SESSION_AID_ENTER);
	 return;
      }
      tn5250_display_set_cursor_next_field (This);
   }
}

/*
 *    Set the specified indicators and set a flag noting that the indicators
 *    must be refreshed.
 */
void tn5250_display_indicator_set (Tn5250Display *This, int inds)
{
   This->indicators |= inds;
   This->indicators_dirty = 1;
}

/*
 *    Clear the specified indicators and set a flag noting that the indicators
 *    must be refreshed.
 */
void tn5250_display_indicator_clear (Tn5250Display *This, int inds)
{
   This->indicators &= ~inds;
   This->indicators_dirty = 1;
}

/*
 *    Clear the display and set the display size to 24x80.
 */
void tn5250_display_clear_unit (Tn5250Display *This)
{
   tn5250_table_clear(This->format_tables);
   tn5250_dbuffer_set_size(This->display_buffers, 24, 80);
   tn5250_display_indicator_set(This, TN5250_DISPLAY_IND_X_SYSTEM);
   tn5250_display_indicator_clear(This,
	 TN5250_DISPLAY_IND_INSERT | TN5250_DISPLAY_IND_INHIBIT);
   tn5250_dbuffer_set_ic(This->display_buffers, 0, 0);
}

/*
 *    Clear the display and set the display size to 27x132.
 */
void tn5250_display_clear_unit_alternate (Tn5250Display *This)
{
   tn5250_table_clear(This->format_tables);
   tn5250_dbuffer_set_size(This->display_buffers, 27, 132);
   tn5250_display_indicator_set(This, TN5250_DISPLAY_IND_X_SYSTEM);
   tn5250_display_indicator_clear(This,
	 TN5250_DISPLAY_IND_INSERT | TN5250_DISPLAY_IND_INHIBIT);
   tn5250_dbuffer_set_ic(This->display_buffers, 0, 0);
}

/*
 *    Clear the format table.
 */
void tn5250_display_clear_format_table (Tn5250Display *This)
{
   tn5250_table_clear(This->format_tables);
   tn5250_display_set_cursor(This, 0, 0);
   tn5250_display_indicator_set(This, TN5250_DISPLAY_IND_X_SYSTEM);
   tn5250_display_indicator_clear(This, TN5250_DISPLAY_IND_INSERT);
}

/*
 *    Move the cursor left.
 */
void tn5250_display_kf_left (Tn5250Display *This)
{
   tn5250_dbuffer_left (This->display_buffers);
}

/*
 *    Move the cursor right.
 */
void tn5250_display_kf_right (Tn5250Display *This)
{
   tn5250_dbuffer_right (This->display_buffers, 1);
}

/*
 *    Move the cursor up.
 */
void tn5250_display_kf_up (Tn5250Display *This)
{
   tn5250_dbuffer_up (This->display_buffers);
}

/*
 *    Move the cursor down.
 */
void tn5250_display_kf_down (Tn5250Display *This)
{
   tn5250_dbuffer_down (This->display_buffers);
}

/*
 *    Toggle the insert indicator.
 */
void tn5250_display_kf_insert (Tn5250Display *This)
{
   if ((tn5250_display_indicators(This) & TN5250_DISPLAY_IND_INSERT) != 0)
      tn5250_display_indicator_clear(This, TN5250_DISPLAY_IND_INSERT);
   else
      tn5250_display_indicator_set(This, TN5250_DISPLAY_IND_INSERT);
}

/*
 *    Tab function.
 */
void tn5250_display_kf_tab (Tn5250Display *This)
{
   tn5250_display_set_cursor_next_field (This);
}

/*
 *    Backwards tab function.
 */
void tn5250_display_kf_backtab (Tn5250Display *This)
{
   /* Backtab: Move to start of this field, or start of 
    * previous field if already there. */
   Tn5250Field *field = tn5250_display_current_field (This);
   if (field == NULL || tn5250_field_count_left(field,
	    tn5250_display_cursor_y(This),
	    tn5250_display_cursor_x(This)) == 0)
      field = tn5250_display_prev_field (This);

   if (field != NULL)
      tn5250_display_set_cursor_field (This, field);
   else
      tn5250_display_set_cursor_home (This);
}

/*
 *    End key function.
 */
void tn5250_display_kf_end (Tn5250Display *This)
{
   Tn5250Field *field = tn5250_display_current_field(This);
   if (field != NULL && !tn5250_field_is_bypass(field)) {
      unsigned char *data = tn5250_display_field_data (This, field);
      int i = tn5250_field_length (field) - 1;
      int y = tn5250_field_start_row (field);
      int x = tn5250_field_start_col (field);

      if (data[i] == '\0') {
	 while (i > 0 && data[i] == '\0')
	    i--;
	 while (i >= 0) {
	    if (++x == tn5250_display_width(This)) {
	       x = 0;
	       if (++y == tn5250_display_height(This))
		  y = 0;
	    }
	    i--;
	 }
      } else {
	 y = tn5250_field_end_row (field);
	 x = tn5250_field_end_col (field);
      }
      tn5250_display_set_cursor (This, y, x);
   } else
      tn5250_display_inhibit(This);
}

/*
 *    Home key function.
 */
void tn5250_display_kf_home (Tn5250Display *This)
{
   Tn5250Field *field;
   int gx, gy;

   /* FIXME: Is this how this is supposed to behave in the case of a 
    * pending insert? */

   if (This->pending_insert)
      tn5250_display_set_cursor_home (This);
   else {
      field = tn5250_display_current_field (This);
      if (field != NULL) {
	 gy = tn5250_field_start_row(field);
	 gx = tn5250_field_start_col(field);
      } else
	 gx = gy = 0;

      if (gy == tn5250_display_cursor_y(This)
	    && gx == tn5250_display_cursor_x(This))
	 tn5250_display_do_aidkey (This, TN5250_SESSION_AID_RECORD_BS);
      else
	 tn5250_display_set_cursor(This, gy, gx);
   }
}

/*
 *    Delete key function.
 */
void tn5250_display_kf_delete (Tn5250Display *This)
{
   Tn5250Field *field = tn5250_display_current_field (This);
   if (field == NULL || tn5250_field_is_bypass(field))
      tn5250_display_inhibit(This);
   else {
      tn5250_field_set_mdt (field);
      tn5250_dbuffer_del(This->display_buffers,
	    tn5250_field_count_right (field,
	       tn5250_display_cursor_y (This),
	       tn5250_display_cursor_x (This)));
   }
}

/* vi:set cindent sts=3 sw=3: */
