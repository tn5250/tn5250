/* TN5250
 * Copyright (C) 1997 Michael Madore
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this software; see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 59 Temple Place, Suite 330,
 * Boston, MA 02111-1307 USA
 * 
 * As a special exception, the copyright holder gives permission
 * for additional uses of the text contained in its release of TN5250.
 * 
 * The exception is that, if you link the TN5250 library with other files
 * to produce an executable, this does not by itself cause the
 * resulting executable to be covered by the GNU General Public License.
 * Your use of that executable is in no way restricted on account of
 * linking the TN5250 library code into it.
 * 
 * This exception does not however invalidate any other reasons why
 * the executable file might be covered by the GNU General Public License.
 * 
 * If you write modifications of your own for TN5250, it is your choice
 * whether to permit this exception to apply to your modifications.
 * If you do not wish that, delete this exception notice. */

#include "tn5250-private.h"

static void tn5250_display_add_dbuffer(Tn5250Display * display,
				       Tn5250DBuffer * dbuffer);

/****f* lib5250/tn5250_display_new
 * NAME
 *    tn5250_display_new
 * SYNOPSIS
 *    ret = tn5250_display_new ();
 * INPUTS
 *    None
 * DESCRIPTION
 *    DOCUMENT ME!!!
 *****/
Tn5250Display *tn5250_display_new()
{
   Tn5250Display *This;

   if ((This = tn5250_new(Tn5250Display, 1)) == NULL)
      return NULL;
   This->display_buffers = NULL;
   This->terminal = NULL;
   This->config = NULL;
   This->indicators = 0;
   This->indicators_dirty = 0;
   This->pending_insert = 0;
   This->session = NULL;
   This->key_queue_head = This->key_queue_tail = 0;
   This->saved_msg_line = NULL;
   This->map = NULL;

   tn5250_display_add_dbuffer(This, tn5250_dbuffer_new(80, 24));
   return This;
}

/****f* lib5250/tn5250_display_destroy
 * NAME
 *    tn5250_display_destroy
 * SYNOPSIS
 *    tn5250_display_destroy (This);
 * INPUTS
 *    Tn5250Display *      This       - 
 * DESCRIPTION
 *    DOCUMENT ME!!!
 *****/
void tn5250_display_destroy(Tn5250Display * This)
{
   Tn5250DBuffer *diter, *dnext;

   if ((diter = This->display_buffers) != NULL) {
      do {
	 dnext = diter->next;
	 tn5250_dbuffer_destroy(diter);
	 diter = dnext;
      } while (diter != This->display_buffers);
   }
   if (This->terminal != NULL)
      tn5250_terminal_destroy(This->terminal);
   if (This->saved_msg_line != NULL)
      free (This->saved_msg_line);
   if (This->config != NULL)
      tn5250_config_unref (This->config);

   free(This);
}

/****f* lib5250/tn5250_display_config
 * NAME
 *    tn5250_display_config
 * SYNOPSIS
 *    tn5250_display_config (This, config);
 * INPUTS
 *    Tn5250Display *      This       - 
 *    Tn5250Config *       config     -
 * DESCRIPTION
 *    Applies configuration to the display.
 *****/
int tn5250_display_config(Tn5250Display *This, Tn5250Config *config)
{
   const char *v;
   const char *termtype;

   tn5250_config_ref (config);
   if (This->config != NULL)
      tn5250_config_unref (This->config);
   This->config = config;

   /* Set a terminal type if necessary */
   termtype = tn5250_config_get(config, "env.TERM");

   if(termtype == NULL) {
       tn5250_config_set(config, "env.TERM", "IBM-3179-2");
   }

   /* Set the new character map. */
   if (This->map != NULL)
      tn5250_char_map_destroy (This->map);
   if ((v = tn5250_config_get (config, "map")) == NULL) {
     tn5250_config_set(config, "map", "37");
     v = tn5250_config_get(config, "map");
   }

   This->map = tn5250_char_map_new (v);
   if (This->map == NULL)
      return -1; /* FIXME: An error message would be nice. */

   return 0;
}

/****f* lib5250/tn5250_display_set_session
 * NAME
 *    tn5250_display_set_session
 * SYNOPSIS
 *    tn5250_display_set_session (This, s);
 * INPUTS
 *    Tn5250Display *      This       - 
 *    struct _Tn5250Session * s          - 
 * DESCRIPTION
 *    DOCUMENT ME!!!
 *****/
void tn5250_display_set_session(Tn5250Display *This, struct _Tn5250Session *s)
{
   This->session = s;
   if (This->session != NULL)
      This->session->display = This;
}

/****f* lib5250/tn5250_display_push_dbuffer
 * NAME
 *    tn5250_display_push_dbuffer
 * SYNOPSIS
 *    ret = tn5250_display_push_dbuffer (This);
 * INPUTS
 *    Tn5250Display *      This       - 
 * DESCRIPTION
 *    Create a new display buffer and assign the old one an id so we can
 *    later restore it.  Return the id which must be > 0.
 *****/
Tn5250DBuffer *tn5250_display_push_dbuffer(Tn5250Display * This)
{
   Tn5250DBuffer *dbuf;

   dbuf = tn5250_dbuffer_copy(This->display_buffers);
   tn5250_display_add_dbuffer(This, dbuf);
   return dbuf;	/* Pointer is used as unique identifier in data stream. */
}

/****f* lib5250/tn5250_display_restore_dbuffer
 * NAME
 *    tn5250_display_restore_dbuffer
 * SYNOPSIS
 *    tn5250_display_restore_dbuffer (This, id);
 * INPUTS
 *    Tn5250Display *      This       - 
 *    Tn5250DBuffer *      id         - 
 * DESCRIPTION
 *    Delete the current dbuffer and replace it with the one with id `id'.
 *****/
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

/****i* lib5250/tn5250_display_add_dbuffer
 * NAME
 *    tn5250_display_add_dbuffer
 * SYNOPSIS
 *    tn5250_display_add_dbuffer (This, dbuffer);
 * INPUTS
 *    Tn5250Display *      This       - 
 *    Tn5250DBuffer *      dbuffer    - 
 * DESCRIPTION
 *    Add a display buffer into this display's circularly linked list of
 *    display buffers.
 *****/
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

/****f* lib5250/tn5250_display_set_terminal
 * NAME
 *    tn5250_display_set_terminal
 * SYNOPSIS
 *    tn5250_display_set_terminal (This, term);
 * INPUTS
 *    Tn5250Display *      This       - 
 *    Tn5250Terminal *     term       - 
 * DESCRIPTION
 *    Set the terminal associated with this display.
 *****/
void tn5250_display_set_terminal(Tn5250Display * This, Tn5250Terminal * term)
{
   if (This->terminal != NULL)
      tn5250_terminal_destroy(This->terminal);
   This->terminal = term;
   This->indicators_dirty = 1;
   tn5250_terminal_update(This->terminal, This);
   tn5250_terminal_update_indicators(This->terminal, This);
}

/****f* lib5250/tn5250_display_update
 * NAME
 *    tn5250_display_update
 * SYNOPSIS
 *    tn5250_display_update (This);
 * INPUTS
 *    Tn5250Display *      This       - 
 * DESCRIPTION
 *    Update the terminal's representation of the display.
 *****/
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

/****f* lib5250/tn5250_display_waitevent
 * NAME
 *    tn5250_display_waitevent
 * SYNOPSIS
 *    ret = tn5250_display_waitevent (This);
 * INPUTS
 *    Tn5250Display *      This       - 
 * DESCRIPTION
 *    Wait for a terminal event.  Handle keystrokes while we're at it
 *    and don't return those to the session (what would it do with them?)
 *****/
int tn5250_display_waitevent(Tn5250Display * This)
{
   int is_x_system, r, handled_key = 0;

   if (This->terminal == NULL)
      return 0;

   while (1) {
      is_x_system = (tn5250_display_indicators(This) & 
	 TN5250_DISPLAY_IND_X_SYSTEM) != 0;

      /* Handle keys from our key queue if we aren't X SYSTEM. */
      if (This->key_queue_head != This->key_queue_tail && !is_x_system) {
	 TN5250_LOG (("Handling buffered key.\n"));
	 tn5250_display_do_key(This,
	       This->key_queue[This->key_queue_head]);
	 if (++ This->key_queue_head == TN5250_DISPLAY_KEYQ_SIZE)
	    This->key_queue_head = 0;
	 handled_key = 1;
	 continue;
      }

      if (handled_key) {
	 tn5250_display_update(This);
	 handled_key = 0;
      }
      r = tn5250_terminal_waitevent(This->terminal);
      if ((r & TN5250_TERMINAL_EVENT_KEY) != 0)
	 tn5250_display_do_keys (This);

      if ((r & ~TN5250_TERMINAL_EVENT_KEY) != 0)
	 return r;
   }
}

/****f* lib5250/tn5250_display_getkey
 * NAME
 *    tn5250_display_getkey
 * SYNOPSIS
 *    ret = tn5250_display_getkey (This);
 * INPUTS
 *    Tn5250Display *      This       - 
 * DESCRIPTION
 *    Get the next keystroke in the keyboard buffer.
 *****/
int tn5250_display_getkey(Tn5250Display * This)
{
   if (This->terminal == NULL)
      return -1;
   return tn5250_terminal_getkey(This->terminal);
}

/****f* lib5250/tn5250_display_beep
 * NAME
 *    tn5250_display_beep
 * SYNOPSIS
 *    tn5250_display_beep (This);
 * INPUTS
 *    Tn5250Display *      This       - 
 * DESCRIPTION
 *    The required beep function.
 *****/
void tn5250_display_beep(Tn5250Display * This)
{
   if (This->terminal == NULL)
      return;
   tn5250_terminal_beep(This->terminal);
}

/****f* lib5250/tn5250_display_set_pending_insert
 * NAME
 *    tn5250_display_set_pending_insert
 * SYNOPSIS
 *    tn5250_display_set_pending_insert (This, y, x);
 * INPUTS
 *    Tn5250Display *      This       - 
 *    int                  y          - 
 *    int                  x          - 
 * DESCRIPTION
 *    Set the pending insert flag and the insert cursor position.
 *****/
void tn5250_display_set_pending_insert (Tn5250Display *This, int y, int x)
{
   This->pending_insert = 1;
   tn5250_dbuffer_set_ic (This->display_buffers, y, x);
}

/****f* lib5250/tn5250_display_field_at
 * NAME
 *    tn5250_display_field_at
 * SYNOPSIS
 *    ret = tn5250_display_field_at (This, y, x);
 * INPUTS
 *    Tn5250Display *      This       - 
 *    int                  y          - 
 *    int                  x          - 
 * DESCRIPTION
 *    DOCUMENT ME!!!
 *****/
Tn5250Field *tn5250_display_field_at(Tn5250Display *This, int y, int x)
{
   return tn5250_dbuffer_field_yx(This->display_buffers, y, x);
}

/****f* lib5250/tn5250_display_current_field
 * NAME
 *    tn5250_display_current_field
 * SYNOPSIS
 *    ret = tn5250_display_current_field (This);
 * INPUTS
 *    Tn5250Display *      This       - 
 * DESCRIPTION
 *    Find the field currently under the cursor.  The answer might be NULL
 *    if there is no field under the cursor.
 *****/
Tn5250Field *tn5250_display_current_field(Tn5250Display * This)
{
   return tn5250_display_field_at(This,
				tn5250_display_cursor_y(This),
				tn5250_display_cursor_x(This));
}

/****f* lib5250/tn5250_display_next_field
 * NAME
 *    tn5250_display_next_field
 * SYNOPSIS
 *    ret = tn5250_display_next_field (This);
 * INPUTS
 *    Tn5250Display *      This       - 
 * DESCRIPTION
 *    Return a pointer to the next field after the current one which is not
 *    a bypass field.
 *****/
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

/****f* lib5250/tn5250_display_prev_field
 * NAME
 *    tn5250_display_prev_field
 * SYNOPSIS
 *    ret = tn5250_display_prev_field (This);
 * INPUTS
 *    Tn5250Display *      This       - 
 * DESCRIPTION
 *    Return a pointer to the first preceding field which is not a bypass
 *    field.
 *****/
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

/****f* lib5250/tn5250_display_set_cursor_home
 * NAME
 *    tn5250_display_set_cursor_home
 * SYNOPSIS
 *    tn5250_display_set_cursor_home (This);
 * INPUTS
 *    Tn5250Display *      This       - 
 * DESCRIPTION
 *    Set the cursor to the home position on the current display buffer.
 *    The home position is:
 *       1) the IC position, if we have one. 
 *       2) the first position of the first non-bypass field, if we have on.
 *       3) 0,0
 *****/
void tn5250_display_set_cursor_home(Tn5250Display * This)
{
  if (This->pending_insert) {
      tn5250_dbuffer_goto_ic(This->display_buffers);
      This->pending_insert = 0;
  } else {
      int y = 0, x = 0;
      Tn5250Field *iter = This->display_buffers->field_list;
      if (iter != NULL) {
	 do {
	    if (!tn5250_field_is_bypass(iter)) {
	       y = tn5250_field_start_row (iter);
	       x = tn5250_field_start_col (iter);
	       break;
	    }
	    iter = iter->next;
	 } while (iter != This->display_buffers->field_list);
      }
      tn5250_display_set_cursor(This, y, x);
   }
}

/****f* lib5250/tn5250_display_set_cursor_field
 * NAME
 *    tn5250_display_set_cursor_field
 * SYNOPSIS
 *    tn5250_display_set_cursor_field (This, field);
 * INPUTS
 *    Tn5250Display *      This       - 
 *    Tn5250Field *        field      - 
 * DESCRIPTION
 *    Set the cursor position on the current display buffer to the home
 *    position of the specified field.
 *****/
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

/****f* lib5250/tn5250_display_set_cursor_next_field
 * NAME
 *    tn5250_display_set_cursor_next_field
 * SYNOPSIS
 *    tn5250_display_set_cursor_next_field (This);
 * INPUTS
 *    Tn5250Display *      This       - 
 * DESCRIPTION
 *    Move the cursor to the next non-bypass field.  This will move the
 *    cursor to the home position if there are no non-bypass fields.
 *****/
void tn5250_display_set_cursor_next_field(Tn5250Display * This)
{
   Tn5250Field *field = tn5250_display_next_field(This);
   tn5250_display_set_cursor_field(This, field);
}

/****f* lib5250/tn5250_display_set_cursor_prev_field
 * NAME
 *    tn5250_display_set_cursor_prev_field
 * SYNOPSIS
 *    tn5250_display_set_cursor_prev_field (This);
 * INPUTS
 *    Tn5250Display *      This       - 
 * DESCRIPTION
 *    Move the cursor to the previous non-bypass field.  This will move the
 *    cursor to the home position if there are no non-bypass fields.
 *****/
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
      Tn5250DBuffer *src_dbuffer)
{
   Tn5250WTDContext *ctx;

   if ((ctx = tn5250_wtd_context_new (buf, src_dbuffer,
	       This->display_buffers)) == NULL)
      return;

   /* 
      These coordinates will be used in an IC order when the screen is 
      restored
   */
   tn5250_wtd_context_set_ic(ctx, 
   	                     tn5250_display_cursor_y(This)+1,
   	                     tn5250_display_cursor_x(This)+1);
   	
   tn5250_wtd_context_convert (ctx);
   tn5250_wtd_context_destroy (ctx);
}

/****f* lib5250/tn5250_display_interactive_addch
 * NAME
 *    tn5250_display_interactive_addch
 * SYNOPSIS
 *    tn5250_display_interactive_addch (This, ch);
 * INPUTS
 *    Tn5250Display *      This       - 
 *    unsigned char        ch         - 
 * DESCRIPTION
 *    Add a character to the display, and set the field's MDT flag.  This
 *    is meant to be called by the keyboard handler when the user is
 *    entering data.
 *****/
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
      if (data[ofs] != '\0' && tn5250_char_map_to_local (This->map, data[ofs]) != ' ') {
	 tn5250_display_inhibit(This);
	 return;
      }
      tn5250_dbuffer_ins(This->display_buffers, tn5250_char_map_to_remote (This->map, ch),
	    tn5250_field_count_right(field,
	       tn5250_display_cursor_y(This),
	       tn5250_display_cursor_x(This)));
   } else
      tn5250_dbuffer_addch(This->display_buffers, tn5250_char_map_to_remote (This->map, ch));

   tn5250_field_set_mdt(field);

   /* If at the end of the field and not a fer field, advance to the
    * next field. */
   if (end_of_field) {
      if (tn5250_field_is_fer(field)) {
	 tn5250_display_indicator_set (This, TN5250_DISPLAY_IND_FER);
	 tn5250_display_set_cursor (This,
				   tn5250_field_end_row(field),
				   tn5250_field_end_col(field));
      } else {
	 tn5250_display_field_adjust(This, field);
	 if (tn5250_field_is_auto_enter(field)) {
	    tn5250_display_do_aidkey (This, TN5250_SESSION_AID_ENTER);
	    return;
	 }
	 tn5250_display_set_cursor_next_field(This);
      }
   }
}

/****f* lib5250/tn5250_display_shift_right
 * NAME
 *    tn5250_display_shift_right
 * SYNOPSIS
 *    tn5250_display_shift_right (This, field, fill);
 * INPUTS
 *    Tn5250Display *      This       - 
 *    Tn5250Field *        field      - 
 *    unsigned char        fill       - 
 * DESCRIPTION
 *    Move all the data characters in the field to the right-hand side of
 *    the field and left-fill the field with `fill' characters.
 *****/
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

/****f* lib5250/tn5250_display_field_adjust
 * NAME
 *    tn5250_display_field_adjust
 * SYNOPSIS
 *    tn5250_display_field_adjust (This, field);
 * INPUTS
 *    Tn5250Display *      This       - 
 *    Tn5250Field *        field      - 
 * DESCRIPTION
 *    Adjust the field data as required by the Field Format Word.  This is
 *    called from tn5250_display_kf_field_exit.
 *****/
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
      tn5250_display_shift_right(This, field, tn5250_char_map_to_remote (This->map, '0'));
      break;
   case TN5250_FIELD_RIGHT_BLANK:
      tn5250_display_shift_right(This, field, tn5250_char_map_to_remote (This->map, ' '));
      break;
   }

   tn5250_field_set_mdt(field);
}

/****f* lib5250/tn5250_display_do_keys
 * NAME
 *    tn5250_display_do_keys
 * SYNOPSIS
 *    tn5250_display_do_keys (This);
 * INPUTS
 *    Tn5250Display *      This       - 
 * DESCRIPTION
 *    Handle keys from the terminal until we run out or are in the X SYSTEM
 *    state.
 *****/
void tn5250_display_do_keys (Tn5250Display *This)
{
   int cur_key;

   do {
      cur_key = tn5250_display_getkey (This);

      if (cur_key != -1) {
	 if ((tn5250_display_indicators(This) & TN5250_DISPLAY_IND_X_SYSTEM) != 0) {
	    /* We can handle system request here. */
	    if (cur_key == K_SYSREQ || cur_key == K_RESET) {
	       /* Flush the keyboard queue. */
	       This->key_queue_head = This->key_queue_tail = 0;
	       tn5250_display_do_key (This, cur_key);
	       break;
	    }

	    if ((This->key_queue_tail + 1 == This->key_queue_head) ||
		  (This->key_queue_head == 0 &&
		   This->key_queue_tail == TN5250_DISPLAY_KEYQ_SIZE - 1)) {
	       TN5250_LOG (("Beep: Key queue full.\n"));
	       tn5250_display_beep (This);
	    }
	    This->key_queue[This->key_queue_tail] = cur_key;
	    if (++ This->key_queue_tail == TN5250_DISPLAY_KEYQ_SIZE)
	       This->key_queue_tail = 0;
	 } else {
	    /* We shouldn't ever be handling a key here if there are keys in the queue. */
	    TN5250_ASSERT(This->key_queue_head == This->key_queue_tail);
	    tn5250_display_do_key (This, cur_key);
	 }
      }
   } while (cur_key != -1);

   tn5250_display_update(This);
}


/****f* lib5250/tn5250_display_do_key
 * NAME
 *    tn5250_display_do_key
 * SYNOPSIS
 *    tn5250_display_do_key (This, key);
 * INPUTS
 *    Tn5250Display *      This       - 
 *    int                  key        - 
 * DESCRIPTION
 *    Translate a keystroke and handle it.
 *****/
void tn5250_display_do_key(Tn5250Display *This, int key)
{
   int pre_FER_clear = 0;

   TN5250_LOG (("@key %d\n", key));

   /* FIXME: Translate from terminal key via keyboard map to 5250 key. */

   if (tn5250_display_inhibited(This)) {
      if (key != K_SYSREQ && key != K_RESET) {
	 tn5250_display_beep (This);
	 return;
      }
   }

   /* In the case we are in the field exit required state, we inhibit on
    * everything except left arrow, backspace, field exit, field+, and
    * field- */
   if ((tn5250_display_indicators (This) & TN5250_DISPLAY_IND_FER) != 0) {
      switch (key) {
      case K_LEFT:
      case K_BACKSPACE:
	 tn5250_display_indicator_clear (This, TN5250_DISPLAY_IND_FER);
	 return;

      case K_FIELDEXIT:
      case K_FIELDMINUS:
      case K_FIELDPLUS:
      case K_TAB:
      case K_BACKTAB:
      case K_RESET:
	 pre_FER_clear = 1;
	 break;

      default:
	 tn5250_display_inhibit (This);
	 return;
      }
   }

   switch (key) {
   case K_RESET:
      tn5250_display_uninhibit(This);
      break;

   case K_BACKSPACE:
      tn5250_display_kf_backspace (This);
      break;

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

   case K_FIELDPLUS:
      tn5250_display_kf_field_plus (This);
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
   if (pre_FER_clear) {
  tn5250_display_indicator_clear (This, TN5250_DISPLAY_IND_FER);
   }
}

/****f* lib5250/tn5250_display_field_pad_and_adjust
 * NAME
 *    tn5250_display_field_pad_and_adjust
 * SYNOPSIS
 *    tn5250_display_field_pad_and_adjust(This, field);
 * INPUTS
 *    Tn5250Display *      This       - 
 *    Tn5250Display *      field      -
 * DESCRIPTION
 *    This nulls out the remainder of the field (after the
 *    cursor position) except for the +/- sign, and then right
 *    adjusts the field.  Does NOT do auto-enter, and does NOT
 *    advance to the next field.
 *****/
void tn5250_display_field_pad_and_adjust(Tn5250Display * This, Tn5250Field *field)
{
   unsigned char *data;
   int i, l;

   /* NUL out remainder of field from cursor position.  For signed numeric
    * fields, we do *not* want to null out the sign position - Field+ and
    * Field- will do this for us.  We do not do this when the FER indicator
    * is set. */
   if ((tn5250_display_indicators (This) & TN5250_DISPLAY_IND_FER) == 0) {
      data = tn5250_display_field_data (This, field);
      i = tn5250_field_count_left(field, tn5250_display_cursor_y(This),
	    tn5250_display_cursor_x(This));
      l = tn5250_field_length(field);
      if (tn5250_field_is_signed_num(field))
	 l--;
      for (; i < l; i++)
	 data[i] = 0;
   }

   tn5250_display_field_adjust(This, field);

}


/****f* lib5250/tn5250_display_kf_field_exit
 * NAME
 *    tn5250_display_kf_field_exit
 * SYNOPSIS
 *    tn5250_display_kf_field_exit (This);
 * INPUTS
 *    Tn5250Display *      This       - 
 * DESCRIPTION
 *    Process a field exit function.
 *****/
void tn5250_display_kf_field_exit(Tn5250Display * This)
{
   Tn5250Field *field;
   unsigned char *data;
   int i, l;

   field = tn5250_display_current_field(This);
   if (field == NULL || tn5250_field_is_bypass(field)) {
      tn5250_display_inhibit(This);
      return;
   }

   tn5250_display_field_pad_and_adjust ( This, field );

   if (tn5250_field_is_auto_enter(field)) {
      tn5250_display_do_aidkey (This, TN5250_SESSION_AID_ENTER);
      return;
   }

   tn5250_display_set_cursor_next_field (This);
}

/****f* lib5250/tn5250_display_kf_field_plus
 * NAME
 *    tn5250_display_kf_field_plus
 * SYNOPSIS
 *    tn5250_display_kf_field_plus (This);
 * INPUTS
 *    Tn5250Display *      This       - 
 * DESCRIPTION
 *    Process a field plus function.
 *****/
void tn5250_display_kf_field_plus(Tn5250Display * This)
{
   Tn5250Field *field;
   unsigned char *data;
   unsigned char c;

   TN5250_LOG (("Field+ entered.\n"));

   field = tn5250_display_current_field (This);
   if (field == NULL || tn5250_field_is_bypass(field)) {
      tn5250_display_inhibit(This);
      return;
   }

   tn5250_display_field_pad_and_adjust ( This, field );
   
   /* NOTE: Field+ should act like field exit on a non-numeric field. */
   if (field != NULL && 
	 (tn5250_field_type(field) == TN5250_FIELD_SIGNED_NUM) ||
	 (tn5250_field_type(field) == TN5250_FIELD_NUM_ONLY)) {

        /* We don't do anything for number only fields.  For signed numeric
         * fields, we change the sign position to a '+'. */
        data = tn5250_display_field_data (This, field);
        if (tn5250_field_type(field) != TN5250_FIELD_NUM_ONLY)
           data[tn5250_field_length (field) - 1] = 0;
   }

   if (tn5250_field_is_auto_enter(field)) {
       tn5250_display_do_aidkey (This, TN5250_SESSION_AID_ENTER);
       return;
   }

   tn5250_display_set_cursor_next_field (This);

}

/****f* lib5250/tn5250_display_kf_field_minus
 * NAME
 *    tn5250_display_kf_field_minus
 * SYNOPSIS
 *    tn5250_display_kf_field_minus (This);
 * INPUTS
 *    Tn5250Display *      This       - 
 * DESCRIPTION
 *    Process a field minus function.
 *****/
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
      /* FIXME: Explain why to the user. */
      tn5250_display_inhibit(This);
      return;
   }

   tn5250_display_field_pad_and_adjust ( This, field );

   /* For numeric only fields, we shift the data one character to the
    * left and insert an ebcdic '}' in the rightmost position.  For
    * signed numeric fields, we change the sign position to a '-'. */
   data = tn5250_display_field_data (This, field);
   if (tn5250_field_type(field) == TN5250_FIELD_NUM_ONLY) {
      if (data[0] != 0x00 && data[0] != 0x40) {
	 /* FIXME: Explain why to the user. */
	 tn5250_display_inhibit(This);
      } else {
	 int i;
	 for (i = 0; i < tn5250_field_length(field) - 1; i++)
	    data[i] = data[i+1];
	 data[tn5250_field_length (field) - 1] = tn5250_char_map_to_remote (This->map, '}');
      }
   } else
      data[tn5250_field_length (field) - 1] = tn5250_char_map_to_remote (This->map, '-');

   if (tn5250_field_is_auto_enter(field)) {
      tn5250_display_do_aidkey (This, TN5250_SESSION_AID_ENTER);
      return;
   }

   tn5250_display_set_cursor_next_field (This);
}

/****f* lib5250/tn5250_display_do_aidkey
 * NAME
 *    tn5250_display_do_aidkey
 * SYNOPSIS
 *    tn5250_display_do_aidkey (This, aidcode);
 * INPUTS
 *    Tn5250Display *      This       - 
 *    int                  aidcode    - 
 * DESCRIPTION
 *    Handle an aid code.
 *****/
void tn5250_display_do_aidkey (Tn5250Display *This, int aidcode)
{
   TN5250_LOG (("tn5250_display_do_aidkey (0x%02X) called.\n", aidcode));

   /* Aidcodes less than zero are pseudo-aid-codes (see session.h) */
   if (This->session->read_opcode || aidcode < 0) {
      /* FIXME: If this returns zero, we need to stop processing. */
      ( *(This->session->handle_aidkey)) (This->session, aidcode);
   }
}

/****f* lib5250/tn5250_display_kf_dup
 * NAME
 *    tn5250_display_kf_dup
 * SYNOPSIS
 *    tn5250_display_kf_dup (This);
 * INPUTS
 *    Tn5250Display *      This       - 
 * DESCRIPTION
 *    Process a DUP key function.
 *****/
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

   if (!tn5250_field_is_dup_enable(field)) {
      /* FIXME: Explain why to the user. */
      tn5250_display_inhibit(This);
      return;
   }

   i = tn5250_field_count_left(field, tn5250_display_cursor_y(This),
	 tn5250_display_cursor_x(This));
   data = tn5250_display_field_data (This, field);
   for (; i < tn5250_field_length(field); i++)
      data[i] = 0x1c;

   if (tn5250_field_is_fer (field)) {
      tn5250_display_indicator_set (This, TN5250_DISPLAY_IND_FER);
      tn5250_dbuffer_cursor_set (This->display_buffers,
	    tn5250_field_end_row (field),
	    tn5250_field_end_col (field));
   } else {
      tn5250_display_field_adjust (This, field);
      if (tn5250_field_is_auto_enter (field)) {
	 tn5250_display_do_aidkey (This, TN5250_SESSION_AID_ENTER);
	 return;
      }
      tn5250_display_set_cursor_next_field (This);
   }
}

/****f* lib5250/tn5250_display_indicator_set
 * NAME
 *    tn5250_display_indicator_set
 * SYNOPSIS
 *    tn5250_display_indicator_set (This, inds);
 * INPUTS
 *    Tn5250Display *      This       - 
 *    int                  inds       - 
 * DESCRIPTION
 *    Set the specified indicators and set a flag noting that the indicators
 *    must be refreshed.
 *****/
void tn5250_display_indicator_set (Tn5250Display *This, int inds)
{
   This->indicators |= inds;
   This->indicators_dirty = 1;
}

/****f* lib5250/tn5250_display_indicator_clear
 * NAME
 *    tn5250_display_indicator_clear
 * SYNOPSIS
 *    tn5250_display_indicator_clear (This, inds);
 * INPUTS
 *    Tn5250Display *      This       - 
 *    int                  inds       - 
 * DESCRIPTION
 *    Clear the specified indicators and set a flag noting that the indicators
 *    must be refreshed.
 *****/
void tn5250_display_indicator_clear (Tn5250Display *This, int inds)
{
   This->indicators &= ~inds;
   This->indicators_dirty = 1;

   /* Restore the message line if we are clearing the X II indicator */
   if ((inds & TN5250_DISPLAY_IND_INHIBIT) != 0 && This->saved_msg_line != NULL) {
      int l = tn5250_dbuffer_msg_line (This->display_buffers);
      memcpy (This->display_buffers->data + l * tn5250_display_width (This),
	    This->saved_msg_line, tn5250_display_width (This));
      free (This->saved_msg_line);
      This->saved_msg_line = NULL;
   }
}

/****f* lib5250/tn5250_display_clear_unit
 * NAME
 *    tn5250_display_clear_unit
 * SYNOPSIS
 *    tn5250_display_clear_unit (This);
 * INPUTS
 *    Tn5250Display *      This       - 
 * DESCRIPTION
 *    Clear the display and set the display size to 24x80.
 *****/
void tn5250_display_clear_unit (Tn5250Display *This)
{
   tn5250_dbuffer_set_size(This->display_buffers, 24, 80);
   tn5250_display_indicator_set(This, TN5250_DISPLAY_IND_X_SYSTEM);
   tn5250_display_indicator_clear(This,
	 TN5250_DISPLAY_IND_INSERT | TN5250_DISPLAY_IND_INHIBIT
	 | TN5250_DISPLAY_IND_FER);
   This->pending_insert = 0;
   tn5250_dbuffer_set_ic(This->display_buffers, 0, 0);
   if (This->saved_msg_line != NULL) {
      free(This->saved_msg_line);
      This->saved_msg_line = NULL;
   }
}

/****f* lib5250/tn5250_display_clear_unit_alternate
 * NAME
 *    tn5250_display_clear_unit_alternate
 * SYNOPSIS
 *    tn5250_display_clear_unit_alternate (This);
 * INPUTS
 *    Tn5250Display *      This       - 
 * DESCRIPTION
 *    Clear the display and set the display size to 27x132.
 *****/
void tn5250_display_clear_unit_alternate (Tn5250Display *This)
{
   tn5250_dbuffer_set_size(This->display_buffers, 27, 132);
   tn5250_display_indicator_set(This, TN5250_DISPLAY_IND_X_SYSTEM);
   tn5250_display_indicator_clear(This,
	 TN5250_DISPLAY_IND_INSERT | TN5250_DISPLAY_IND_INHIBIT
	 | TN5250_DISPLAY_IND_FER);
   This->pending_insert = 0;
   tn5250_dbuffer_set_ic(This->display_buffers, 0, 0);
   if (This->saved_msg_line != NULL) {
      free(This->saved_msg_line);
      This->saved_msg_line = NULL;
   }
}

/****f* lib5250/tn5250_display_clear_format_table
 * NAME
 *    tn5250_display_clear_format_table
 * SYNOPSIS
 *    tn5250_display_clear_format_table (This);
 * INPUTS
 *    Tn5250Display *      This       - 
 * DESCRIPTION
 *    Clear the format table.
 *****/
void tn5250_display_clear_format_table (Tn5250Display *This)
{
   tn5250_dbuffer_clear_table(This->display_buffers);
   tn5250_display_set_cursor(This, 0, 0);
   tn5250_display_indicator_set(This, TN5250_DISPLAY_IND_X_SYSTEM);
   tn5250_display_indicator_clear(This, TN5250_DISPLAY_IND_INSERT);
}

/****f* lib5250/tn5250_display_kf_backspace
 * NAME
 *    tn5250_display_kf_backspace
 * SYNOPSIS
 *    tn5250_display_kf_backspace (This);
 * INPUTS
 *    Tn5250Display *      This       - 
 * DESCRIPTION
 *    Move the cursor left one position unless on the first position of
 *    field, in which case we move to the last position of the previous
 *    field. (Or inhibit if we aren't on a field).
 *****/
void tn5250_display_kf_backspace (Tn5250Display *This)
{
   Tn5250Field *field = tn5250_display_current_field (This);
   if (field == NULL) {
      /* FIXME: Inform user why. */
      tn5250_display_inhibit (This);
      return;
   }

   /* If in first position of field, set cursor position to last position
    * of previous field. */
   if (tn5250_display_cursor_x (This) == tn5250_field_start_col (field) &&
	 tn5250_display_cursor_y (This) == tn5250_field_start_row (field)) {
      field = tn5250_display_prev_field (This);
      if (field == NULL)
	 return; /* Should never happen */
      tn5250_display_set_cursor_field (This, field);
      if (tn5250_field_length (field) - 1 > 0)
	 tn5250_dbuffer_right (This->display_buffers,
	       tn5250_field_length (field) - 1);
      return;
   }

   tn5250_dbuffer_left (This->display_buffers);
}

/****f* lib5250/tn5250_display_kf_left
 * NAME
 *    tn5250_display_kf_left
 * SYNOPSIS
 *    tn5250_display_kf_left (This);
 * INPUTS
 *    Tn5250Display *      This       - 
 * DESCRIPTION
 *    Move the cursor left.
 *****/
void tn5250_display_kf_left (Tn5250Display *This)
{
   tn5250_dbuffer_left (This->display_buffers);
}

/****f* lib5250/tn5250_display_kf_right
 * NAME
 *    tn5250_display_kf_right
 * SYNOPSIS
 *    tn5250_display_kf_right (This);
 * INPUTS
 *    Tn5250Display *      This       - 
 * DESCRIPTION
 *    Move the cursor right.
 *****/
void tn5250_display_kf_right (Tn5250Display *This)
{
   tn5250_dbuffer_right (This->display_buffers, 1);
}

/****f* lib5250/tn5250_display_kf_up
 * NAME
 *    tn5250_display_kf_up
 * SYNOPSIS
 *    tn5250_display_kf_up (This);
 * INPUTS
 *    Tn5250Display *      This       - 
 * DESCRIPTION
 *    Move the cursor up.
 *****/
void tn5250_display_kf_up (Tn5250Display *This)
{
   tn5250_dbuffer_up (This->display_buffers);
}

/****f* lib5250/tn5250_display_kf_down
 * NAME
 *    tn5250_display_kf_down
 * SYNOPSIS
 *    tn5250_display_kf_down (This);
 * INPUTS
 *    Tn5250Display *      This       - 
 * DESCRIPTION
 *    Move the cursor down.
 *****/
void tn5250_display_kf_down (Tn5250Display *This)
{
   tn5250_dbuffer_down (This->display_buffers);
}

/****f* lib5250/tn5250_display_kf_insert
 * NAME
 *    tn5250_display_kf_insert
 * SYNOPSIS
 *    tn5250_display_kf_insert (This);
 * INPUTS
 *    Tn5250Display *      This       - 
 * DESCRIPTION
 *    Toggle the insert indicator.
 *****/
void tn5250_display_kf_insert (Tn5250Display *This)
{
   if ((tn5250_display_indicators(This) & TN5250_DISPLAY_IND_INSERT) != 0)
      tn5250_display_indicator_clear(This, TN5250_DISPLAY_IND_INSERT);
   else
      tn5250_display_indicator_set(This, TN5250_DISPLAY_IND_INSERT);
}

/****f* lib5250/tn5250_display_kf_tab
 * NAME
 *    tn5250_display_kf_tab
 * SYNOPSIS
 *    tn5250_display_kf_tab (This);
 * INPUTS
 *    Tn5250Display *      This       - 
 * DESCRIPTION
 *    Tab function.
 *****/
void tn5250_display_kf_tab (Tn5250Display *This)
{
   tn5250_display_set_cursor_next_field (This);
}

/****f* lib5250/tn5250_display_kf_backtab
 * NAME
 *    tn5250_display_kf_backtab
 * SYNOPSIS
 *    tn5250_display_kf_backtab (This);
 * INPUTS
 *    Tn5250Display *      This       - 
 * DESCRIPTION
 *    Backwards tab function.
 *****/
void tn5250_display_kf_backtab (Tn5250Display *This)
{
   /* Backtab: Move to start of this field, or start of previous field if
    * already there. */
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

/****f* lib5250/tn5250_display_kf_end
 * NAME
 *    tn5250_display_kf_end
 * SYNOPSIS
 *    tn5250_display_kf_end (This);
 * INPUTS
 *    Tn5250Display *      This       - 
 * DESCRIPTION
 *    End key function.
 *****/
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

/****f* lib5250/tn5250_display_kf_home
 * NAME
 *    tn5250_display_kf_home
 * SYNOPSIS
 *    tn5250_display_kf_home (This);
 * INPUTS
 *    Tn5250Display *      This       - 
 * DESCRIPTION
 *    Home key function.
 *****/
void tn5250_display_kf_home (Tn5250Display *This)
{
   Tn5250Field *field;
   int gx, gy;

   if (This->pending_insert) {
      gy = This->display_buffers->tcy;
      gx = This->display_buffers->tcx;
   } else {
      field = tn5250_dbuffer_first_non_bypass (This->display_buffers);
      if (field != NULL) {
	 gy = tn5250_field_start_row(field);
	 gx = tn5250_field_start_col(field);
      } else
	 gx = gy = 0;
   }

   if (gy == tn5250_display_cursor_y(This)
	 && gx == tn5250_display_cursor_x(This))
      tn5250_display_do_aidkey (This, TN5250_SESSION_AID_RECORD_BS);
   else
      tn5250_display_set_cursor(This, gy, gx);
}

/****f* lib5250/tn5250_display_kf_delete
 * NAME
 *    tn5250_display_kf_delete
 * SYNOPSIS
 *    tn5250_display_kf_delete (This);
 * INPUTS
 *    Tn5250Display *      This       - 
 * DESCRIPTION
 *    Delete key function.
 *****/
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

/****f* lib5250/tn5250_display_save_msg_line
 * NAME
 *    tn5250_display_save_msg_line
 * SYNOPSIS
 *    tn5250_display_save_msg_line (This);
 * INPUTS
 *    Tn5250Display *      This       - 
 * DESCRIPTION
 *    Save the current message line.
 *****/
void tn5250_display_save_msg_line (Tn5250Display *This)
{
   int i, l;

   if (This->saved_msg_line != NULL)
      free (This->saved_msg_line);
   This->saved_msg_line = (unsigned char *)malloc (tn5250_display_width (This));
   TN5250_ASSERT (This->saved_msg_line != NULL);

   l = tn5250_dbuffer_msg_line (This->display_buffers);
   memcpy (This->saved_msg_line, This->display_buffers->data +
	 tn5250_display_width (This) * l,
	 tn5250_display_width (This));
}

/****f* lib5250/tn5250_display_set_char_map
 * NAME
 *    tn5250_display_set_char_map
 * SYNOPSIS
 *    tn5250_display_set_char_map (display, "37");
 * INPUTS
 *    Tn5250Display *      display    - Pointer to the display object.
 *    const char *         name       - Name of translation map to use.
 * DESCRIPTION
 *    Save the current message line.
 *****/
void tn5250_display_set_char_map (Tn5250Display *This, const char *name)
{
   Tn5250CharMap *map = tn5250_char_map_new (name);
   TN5250_ASSERT (map != NULL);
   if (This->map != NULL)
      tn5250_char_map_destroy (This->map);
   This->map = map;
}

/* vi:set sts=3 sw=3: */
