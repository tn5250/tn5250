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

static void tn5250_display_add_dbuffer (Tn5250Display * display,
					Tn5250DBuffer * dbuffer);
void tn5250_display_set_cursor_next_progression_field (Tn5250Display *
						       This, int nextfield);
void tn5250_display_set_cursor_prev_progression_field (Tn5250Display *
						       This,
						       int currentfield);
void tn5250_display_wordwrap_delete (Tn5250Display * This);
void tn5250_display_wordwrap_insert (Tn5250Display * This, unsigned char c,
				     int shiftcount);
void tn5250_display_wordwrap_addch (Tn5250Display * This, unsigned char c);
int display_check_pccmd(Tn5250Display *This);


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
Tn5250Display *
tn5250_display_new ()
{
  Tn5250Display *This;

  if ((This = tn5250_new (Tn5250Display, 1)) == NULL)
    {
      return NULL;
    }
  This->display_buffers = NULL;
  This->macro = NULL;
  This->terminal = NULL;
  This->config = NULL;
  This->indicators = 0;
  This->indicators_dirty = 0;
  This->pending_insert = 0;
  This->sign_key_hack = 1;
  This->field_minus_in_char = 0;
  This->uninhibited = 0;
  This->session = NULL;
  This->key_queue_head = This->key_queue_tail = 0;
  This->saved_msg_line = NULL;
  This->msg_line = NULL;
  This->map = NULL;
  This->keystate = TN5250_KEYSTATE_UNLOCKED;
  This->keySRC = TN5250_KBDSRC_NONE;
  This->allow_strpccmd = 0;

  tn5250_display_add_dbuffer (This, tn5250_dbuffer_new (80, 24));
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
void
tn5250_display_destroy (Tn5250Display * This)
{
  Tn5250DBuffer *diter, *dnext;

  if ((diter = This->display_buffers) != NULL)
    {
      do
	{
	  dnext = diter->next;
	  tn5250_dbuffer_destroy (diter);
	  diter = dnext;
	}
      while (diter != This->display_buffers);
    }
  if (This->terminal != NULL)
    {
      tn5250_terminal_destroy (This->terminal);
    }
  if (This->saved_msg_line != NULL)
    {
      free (This->saved_msg_line);
    }
  if (This->msg_line != NULL)
    {
      free (This->msg_line);
    }
  if (This->config != NULL)
    {
      tn5250_config_unref (This->config);
    }
  free (This);
  return;
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
int
tn5250_display_config (Tn5250Display * This, Tn5250Config * config)
{
  const char *v;
  const char *termtype;

  tn5250_config_ref (config);
  if (This->config != NULL)
    {
      tn5250_config_unref (This->config);
    }
  This->config = config;

  /* check if the +/- sign keyboard hack should be enabled */
  if (tn5250_config_get (config, "sign_key_hack"))
    {
      This->sign_key_hack = tn5250_config_get_bool (config, "sign_key_hack");
    }

  /* check if Input Inhibited should be cleared automatically */
  if (tn5250_config_get (config, "uninhibited"))
    {
      This->uninhibited = tn5250_config_get_bool (config, "uninhibited");
    }

  /* check if user wants to allow the host to run commands via strpccmd */
  if (tn5250_config_get (config, "allow_strpccmd"))
    {
      This->allow_strpccmd = tn5250_config_get_bool (config, "allow_strpccmd");
    }

  /* Should field minus act like field exit in a char field? */
  if (tn5250_config_get (config, "field_minus_in_char"))
    {
      This->field_minus_in_char = 
           tn5250_config_get_bool (config, "field_minus_in_char");
    }

  /* Set a terminal type if necessary */
  termtype = tn5250_config_get (config, "env.TERM");

  if (termtype == NULL)
    {
      tn5250_config_set (config, "env.TERM", "IBM-3179-2");
    }

  /* Set the new character map. */
  if (This->map != NULL)
    {
      tn5250_char_map_destroy (This->map);
    }
  if ((v = tn5250_config_get (config, "map")) == NULL)
    {
      tn5250_config_set (config, "map", "37");
      v = tn5250_config_get (config, "map");
    }

  This->map = tn5250_char_map_new (v);
  if (This->map == NULL)
    {
      return -1;		/* FIXME: An error message would be nice. */
    }

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
void
tn5250_display_set_session (Tn5250Display * This, struct _Tn5250Session *s)
{
  This->session = s;
  if (This->session != NULL)
    {
      This->session->display = This;
    }
  return;
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
Tn5250DBuffer *
tn5250_display_push_dbuffer (Tn5250Display * This)
{
  Tn5250DBuffer *dbuf;

  dbuf = tn5250_dbuffer_copy (This->display_buffers);
  tn5250_display_add_dbuffer (This, dbuf);
  return dbuf;			/* Pointer is used as unique identifier in data stream. */
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
void
tn5250_display_restore_dbuffer (Tn5250Display * This, Tn5250DBuffer * id)
{
  Tn5250DBuffer *iter;

  /* Sanity check to make sure that the display buffer is for real and
   * that it isn't the one which is currently active. */
  if ((iter = This->display_buffers) != NULL)
    {
      do
	{
	  if (iter == id && iter != This->display_buffers)
	    {
	      break;
	    }
	  iter = iter->next;
	}
      while (iter != This->display_buffers);

      if (iter != id || iter == This->display_buffers)
	{
	  return;
	}
    }
  else
    {
      return;
    }

  This->display_buffers->prev->next = This->display_buffers->next;
  This->display_buffers->next->prev = This->display_buffers->prev;
  tn5250_dbuffer_destroy (This->display_buffers);
  This->display_buffers = iter;
  return;
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
static void
tn5250_display_add_dbuffer (Tn5250Display * This, Tn5250DBuffer * dbuffer)
{
  TN5250_ASSERT (dbuffer != NULL);

  if (This->display_buffers == NULL)
    {
      This->display_buffers = dbuffer;
      dbuffer->next = dbuffer->prev = dbuffer;
    }
  else
    {
      dbuffer->next = This->display_buffers;
      dbuffer->prev = This->display_buffers->prev;
      dbuffer->next->prev = dbuffer;
      dbuffer->prev->next = dbuffer;
    }
  return;
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
void
tn5250_display_set_terminal (Tn5250Display * This, Tn5250Terminal * term)
{
  if (This->terminal != NULL)
    {
      tn5250_terminal_destroy (This->terminal);
    }
  This->terminal = term;
  This->indicators_dirty = 1;
  tn5250_terminal_update (This->terminal, This);
  tn5250_terminal_update_indicators (This->terminal, This);
  return;
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
void
tn5250_display_update (Tn5250Display * This)
{
  if (This->msg_line != NULL)
    {
      int l;
      l = tn5250_dbuffer_msg_line (This->display_buffers);
      memcpy (This->display_buffers->data +
	      tn5250_display_width (This) * l, This->msg_line, This->msg_len);
    }
  if (display_check_pccmd(This) == 0) 
    {
      if (This->terminal != NULL)
        {
          tn5250_terminal_update (This->terminal, This);
          if (This->indicators_dirty)
    	    {
	       tn5250_terminal_update_indicators (This->terminal, This);
	       This->indicators_dirty = 0;
	   }
       }
    }
  return;
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
int
tn5250_display_waitevent (Tn5250Display * This)
{
  int is_x_system, r, handled_key = 0;

  if (This->terminal == NULL)
    {
      return 0;
    }

  while (1)
    {
      is_x_system = (This->keystate == TN5250_KEYSTATE_LOCKED);

      /* Handle keys from our key queue if we aren't X SYSTEM. */
      if (This->key_queue_head != This->key_queue_tail && !is_x_system)
	{
	  TN5250_LOG (("Handling buffered key.\n"));
	  tn5250_display_do_key (This, This->key_queue[This->key_queue_head]);
	  if (++This->key_queue_head == TN5250_DISPLAY_KEYQ_SIZE)
	    {
	      This->key_queue_head = 0;
	    }
	  handled_key = 1;
	  continue;
	}

      /* don't make the user press HELP to see what the error is */
      if (This->keystate == TN5250_KEYSTATE_PREHELP)
	{
	  tn5250_display_do_key (This, K_HELP);
	  handled_key = 1;
	}

      if (handled_key)
	{
	  tn5250_display_update (This);
	  handled_key = 0;
	}
      r = tn5250_terminal_waitevent (This->terminal);
      if ((r & TN5250_TERMINAL_EVENT_KEY) != 0)
	{
	  tn5250_display_do_keys (This);
	}

      if ((r & ~TN5250_TERMINAL_EVENT_KEY) != 0)
	{
	  return r;
	}
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
int
tn5250_display_getkey (Tn5250Display * This)
{
  if (This->terminal == NULL)
    {
      return -1;
    }
  return tn5250_terminal_getkey (This->terminal);
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
void
tn5250_display_beep (Tn5250Display * This)
{
  const char *cmd;
  int ec;

  if ((cmd = tn5250_config_get (This->config, "beep_command")) != NULL)
    {
      ec = system (cmd);
      if (ec == -1)
	{
	  TN5250_LOG (("system() for beep command failed: %s\n",
		       strerror (errno)));
	}
      else if (ec != 0)
	{
	  TN5250_LOG (("beep command exited with errno %d\n", ec));
	}
      return;
    }

  if (This->terminal == NULL)
    return;

  tn5250_terminal_beep (This->terminal);
  return;
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
void
tn5250_display_set_pending_insert (Tn5250Display * This, int y, int x)
{
  This->pending_insert = 1;
  tn5250_dbuffer_set_ic (This->display_buffers, y, x);
  return;
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
Tn5250Field *
tn5250_display_field_at (Tn5250Display * This, int y, int x)
{
  return tn5250_dbuffer_field_yx (This->display_buffers, y, x);
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
Tn5250Field *
tn5250_display_current_field (Tn5250Display * This)
{
  return tn5250_display_field_at (This,
				  tn5250_display_cursor_y (This),
				  tn5250_display_cursor_x (This));
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
Tn5250Field *
tn5250_display_next_field (Tn5250Display * This)
{
  Tn5250Field *iter = NULL, *next;
  int y, x;

  y = tn5250_display_cursor_y (This);
  x = tn5250_display_cursor_x (This);

  iter = tn5250_display_field_at (This, y, x);
  if (iter == NULL)
    {
      /* Find the first field on the display after the cursor, wrapping if we
       * hit the bottom of the display. */
      while (iter == NULL)
	{
	  if ((iter = tn5250_display_field_at (This, y, x)) == NULL)
	    {
	      if (++x == tn5250_dbuffer_width (This->display_buffers))
		{
		  x = 0;
		  if (++y == tn5250_dbuffer_height (This->display_buffers))
		    {
		      y = 0;
		    }
		}
	      if (y == tn5250_display_cursor_y (This) &&
		  x == tn5250_display_cursor_x (This))
		{
		  return NULL;	/* No fields on display */
		}
	    }
	}
    }
  else
    {
      iter = iter->next;
    }

  next = iter;
  while (tn5250_field_is_bypass (next))
    {
      next = next->next;	/* Hehe */
      if (next == iter && tn5250_field_is_bypass (next))
	{
	  return NULL;		/* No non-bypass fields. */
	}
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
Tn5250Field *
tn5250_display_prev_field (Tn5250Display * This)
{
  Tn5250Field *iter = NULL, *prev;
  int y, x;

  y = tn5250_display_cursor_y (This);
  x = tn5250_display_cursor_x (This);

  iter = tn5250_display_field_at (This, y, x);
  if (iter == NULL)
    {
      /* Find the first field on the display after the cursor, wrapping if we
       * hit the bottom of the display. */
      while (iter == NULL)
	{
	  if ((iter = tn5250_display_field_at (This, y, x)) == NULL)
	    {
	      if (x-- == 0)
		{
		  x = tn5250_dbuffer_width (This->display_buffers) - 1;
		  if (y-- == 0)
		    {
		      y = tn5250_dbuffer_height (This->display_buffers) - 1;
		    }
		}
	      if (y == tn5250_display_cursor_y (This) &&
		  x == tn5250_display_cursor_x (This))
		{
		  return NULL;	/* No fields on display */
		}
	    }
	}
    }
  else
    {
      iter = iter->prev;
    }

  prev = iter;
  while (tn5250_field_is_bypass (prev))
    {
      prev = prev->prev;	/* Hehe */
      if (prev == iter && tn5250_field_is_bypass (prev))
	{
	  return NULL;		/* No non-bypass fields. */
	}
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
void
tn5250_display_set_cursor_home (Tn5250Display * This)
{
  if (This->pending_insert)
    {
      tn5250_dbuffer_goto_ic (This->display_buffers);
      /*     This->pending_insert = 0; */
    }
  else
    {
      int y = 0, x = 0;
      Tn5250Field *iter = This->display_buffers->field_list;
      if (iter != NULL)
	{
	  do
	    {
	      if (!tn5250_field_is_bypass (iter))
		{
		  y = tn5250_field_start_row (iter);
		  x = tn5250_field_start_col (iter);
		  break;
		}
	      iter = iter->next;
	    }
	  while (iter != This->display_buffers->field_list);
	}
      tn5250_display_set_cursor (This, y, x);
    }
  return;
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
void
tn5250_display_set_cursor_field (Tn5250Display * This, Tn5250Field * field)
{
  if (field == NULL)
    {
      tn5250_display_set_cursor_home (This);
      return;
    }
  tn5250_dbuffer_cursor_set (This->display_buffers,
			     tn5250_field_start_row (field),
			     tn5250_field_start_col (field));
  return;
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
void
tn5250_display_set_cursor_next_field (Tn5250Display * This)
{
  Tn5250Field *currentfield = tn5250_display_current_field (This);
  Tn5250Field *field;

  if ((currentfield != NULL) && (currentfield->nextfieldprogressionid != 0))
    {
      tn5250_display_set_cursor_next_progression_field (This,
							currentfield->
							nextfieldprogressionid);
    }
  else
    {
      field = tn5250_display_next_field (This);
      tn5250_display_set_cursor_field (This, field);
    }
  return;
}


/****f* lib5250/tn5250_display_set_cursor_next_progression_field
 * NAME
 *    tn5250_display_set_cursor_next_progression_field
 * SYNOPSIS
 *    tn5250_display_set_cursor_next_progression_field (This);
 * INPUTS
 *    Tn5250Display *      This       - 
 * DESCRIPTION
 *    Move the cursor to the next specified field.  This will move the
 *    cursor to the next non-bypass field if no field is specified.
 *****/
void
tn5250_display_set_cursor_next_progression_field (Tn5250Display * This,
						  int nextfield)
{
  Tn5250Field *field;

  if (nextfield == 0)
    {
      tn5250_display_set_cursor_next_field (This);
      return;
    }

  while ((field = tn5250_display_next_field (This)) != NULL)
    {
      tn5250_display_set_cursor_field (This, field);

      if (field->entry_id == nextfield)
	{
	  break;
	}
    }
  return;
}


/****f* lib5250/tn5250_display_set_cursor_next_logical_field
 * NAME
 *    tn5250_display_set_cursor_next_logical_field
 * SYNOPSIS
 *    tn5250_display_set_cursor_next_logical_field (This);
 * INPUTS
 *    Tn5250Display *      This       - 
 * DESCRIPTION
 *    Move the cursor to the next non-bypass field not of the same 
 *    continuous field set.  This will move the cursor to the home
 *    position if there are no non-bypass fields.
 *****/
void
tn5250_display_set_cursor_next_logical_field (Tn5250Display * This)
{
  Tn5250Field *currentfield = tn5250_display_current_field (This);
  int currentid, origid;

  if (currentfield != NULL)
    {
      currentid = currentfield->entry_id;
      origid = currentfield->id;

      while (currentid == currentfield->entry_id)
	{
	  tn5250_display_set_cursor_next_field (This);
	  currentfield = tn5250_display_current_field (This);

	  if ((currentfield == NULL) || (origid == currentfield->id))
	    {
	      break;
	    }
	}
    }
  else
    {
      tn5250_display_set_cursor_next_field (This);
    }
  return;
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
void
tn5250_display_set_cursor_prev_field (Tn5250Display * This)
{
  Tn5250Field *currentfield = tn5250_display_current_field (This);
  Tn5250Field *field;

  if ((currentfield != NULL) && (currentfield->entry_id != 0))
    {
      tn5250_display_set_cursor_prev_progression_field (This,
							currentfield->
							entry_id);
    }
  else
    {
      field = tn5250_display_prev_field (This);
      tn5250_display_set_cursor_field (This, field);
    }
  return;
}


/****f* lib5250/tn5250_display_set_cursor_prev_progression_field
 * NAME
 *    tn5250_display_set_cursor_prev_progression_field
 * SYNOPSIS
 *    tn5250_display_set_cursor_prev_progression_field (This);
 * INPUTS
 *    Tn5250Display *      This       - 
 * DESCRIPTION
 *    Move the cursor to the previous progression field.
 *
 *****/
void
tn5250_display_set_cursor_prev_progression_field (Tn5250Display * This,
						  int currentfield)
{
  Tn5250Field *field;
  Tn5250Field *origfield;
  int orig_id;
  int differentfieldfound;

  if (currentfield == 0)
    {
      return;
    }

  origfield = tn5250_display_current_field (This);

  if (tn5250_field_is_bypass (origfield))
    {
      field = tn5250_display_prev_field (This);
      tn5250_display_set_cursor_field (This, field);
      return;
    }

  orig_id = origfield->id;
  differentfieldfound = 0;

  while ((field = tn5250_display_prev_field (This)) != NULL)
    {
      tn5250_display_set_cursor_field (This, field);

      if (field->entry_id == currentfield)
	{
	  if (field->id == orig_id)
	    {
	      field = tn5250_display_prev_field (This);
	      tn5250_display_set_cursor_field (This, field);
	      break;
	    }
	  if (!differentfieldfound)
	    {
	      break;
	    }
	}
      else
	{
	  differentfieldfound = 1;
	}

      if (field->nextfieldprogressionid == currentfield)
	{
	  break;
	}
    }
  return;
}


/****f* lib5250/tn5250_display_set_cursor_prev_logical_field
 * NAME
 *    tn5250_display_set_cursor_prev_logical_field
 * SYNOPSIS
 *    tn5250_display_set_cursor_prev_logical_field (This);
 * INPUTS
 *    Tn5250Display *      This       - 
 * DESCRIPTION
 *    Move the cursor to the previous non-bypass field not of the same
 *    continuous field set.  This will move the cursor to the home
 *    position if there are no non-bypass fields.
 *****/
void
tn5250_display_set_cursor_prev_logical_field (Tn5250Display * This)
{
  Tn5250Field *currentfield;
  int currentid, origid;

  tn5250_display_set_cursor_prev_field (This);
  currentfield = tn5250_display_current_field (This);

  if (currentfield == NULL)
    {
      return;
    }

  currentid = currentfield->entry_id;
  origid = currentfield->id;

  while (currentid == currentfield->entry_id)
    {
      tn5250_display_set_cursor_prev_field (This);
      currentfield = tn5250_display_current_field (This);

      if ((currentfield == NULL) || (origid == currentfield->id))
	{
	  break;
	}
    }
  tn5250_display_set_cursor_next_field (This);
  return;
}


/*
 *    Reconstruct WTD data as it might be sent from a host.  We use this
 *    to save our format table and display buffer.  We assume the buffer
 *    has been initialized, and we append to it.
 */
void
tn5250_display_make_wtd_data (Tn5250Display * This, Tn5250Buffer * buf,
			      Tn5250DBuffer * src_dbuffer)
{
  Tn5250WTDContext *ctx;

  if ((ctx = tn5250_wtd_context_new (buf, src_dbuffer,
				     This->display_buffers)) == NULL)
    {
      return;
    }

  /* 
     These coordinates will be used in an IC order when the screen is 
     restored
   */
  tn5250_wtd_context_set_ic (ctx,
			     tn5250_display_cursor_y (This) + 1,
			     tn5250_display_cursor_x (This) + 1);

  tn5250_wtd_context_convert (ctx);
  tn5250_wtd_context_destroy (ctx);
  return;
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
void
tn5250_display_interactive_addch (Tn5250Display * This, unsigned char ch)
{
  Tn5250Field *field = tn5250_display_current_field (This);
  Tn5250Field *contfield;
  int end_of_field = 0;
  int nextfieldprogressionid = 0;

  if (field == NULL || tn5250_field_is_bypass (field))
    {
      This->keystate = TN5250_KEYSTATE_PREHELP;
      This->keySRC = TN5250_KBDSRC_PROTECT;
      tn5250_display_inhibit (This);
      return;
    }
  /* Upcase the character if this is a monocase field. */
  if (tn5250_field_is_monocase (field) && isalpha (ch))
    {
      ch = toupper (ch);
    }

  /* '+' and '-' keys activate field exit/field minus for numeric fields. */
  if (This->sign_key_hack && (tn5250_field_is_num_only (field)
			      || tn5250_field_is_signed_num (field)))
    {
      switch (ch)
	{
	case '+':
	  tn5250_display_kf_field_plus (This);
	  return;

	case '-':
	  tn5250_display_kf_field_minus (This);
	  return;
	}
    }

  /* Make sure this is a valid data character for this field type. */
  if (!tn5250_field_valid_char (field, ch, &(This->keySRC)))
    {
      TN5250_LOG (("Inhibiting: invalid character for field type.\n"));
      This->keystate = TN5250_KEYSTATE_PREHELP;
      tn5250_display_inhibit (This);
      return;
    }
  /* Are we at the last character of the field? */
  if (tn5250_display_cursor_y (This) == tn5250_field_end_row (field) &&
      tn5250_display_cursor_x (This) == tn5250_field_end_col (field))
    {
      end_of_field = 1;

      if (field->nextfieldprogressionid != 0)
	{
	  nextfieldprogressionid = field->nextfieldprogressionid;
	}
    }

  /* Don't allow the user to enter data in the sign portion of a signed
   * number field. */
  if (end_of_field && tn5250_field_is_signed_num (field))
    {
      TN5250_LOG (("Inhibiting: last character of signed num field.\n"));
      This->keystate = TN5250_KEYSTATE_PREHELP;
      This->keySRC = TN5250_KBDSRC_SIGNPOS;
      tn5250_display_inhibit (This);
      return;
    }

  /* Add or insert the character (depending on whether insert mode is on). */
  if ((tn5250_display_indicators (This) & TN5250_DISPLAY_IND_INSERT) != 0)
    {
      int ofs = tn5250_field_length (field) - 1;
      unsigned char *data = tn5250_display_field_data (This, field);

      if (tn5250_field_is_continued (field))
	{
	  contfield = field;
	  while (!tn5250_field_is_continued_last (contfield))
	    {
	      contfield = contfield->next;
	    }
	  ofs = tn5250_field_length (contfield) - 1;
	  data = tn5250_display_field_data (This, contfield);
	}

      if (tn5250_field_is_signed_num (field))
	{
	  ofs--;
	}

      if ((data[ofs] != '\0')
	  && (tn5250_char_map_to_local (This->map, data[ofs]) != ' ')
	  && (data[ofs] != TN5250_DISPLAY_WORD_WRAP_SPACE))
	{
	  This->keystate = TN5250_KEYSTATE_PREHELP;
	  This->keySRC = TN5250_KBDSRC_NOROOM;
	  tn5250_display_inhibit (This);
	  return;
	}

      if (tn5250_field_is_wordwrap (field))
	{
	  tn5250_display_wordwrap_insert (This,
					  tn5250_char_map_to_remote (This->
								     map, ch),
					  tn5250_field_count_right (field,
								    tn5250_display_cursor_y
								    (This),
								    tn5250_display_cursor_x
								    (This)));
	}
      else
	{
	  tn5250_dbuffer_ins (This->display_buffers, field->id,
			      tn5250_char_map_to_remote (This->map, ch),
			      tn5250_field_count_right (field,
							tn5250_display_cursor_y
							(This),
							tn5250_display_cursor_x
							(This)));
	}
    }
  else
    {
      if (tn5250_field_is_wordwrap (field)
	  || (tn5250_field_is_continued_last (field)
	      && tn5250_field_is_wordwrap (field->prev)))
	{
	  tn5250_display_wordwrap_addch (This,
					 tn5250_char_map_to_remote (This->map,
								    ch));
	}
      else
	{
	  if (This->terminal->putkey != NULL)
	    {
	      tn5250_terminal_putkey (This->terminal, This, ch,
				      tn5250_display_cursor_y (This),
				      tn5250_display_cursor_x (This));
	    }

	  tn5250_dbuffer_addch (This->display_buffers,
				tn5250_char_map_to_remote (This->map, ch));
	}
    }

  tn5250_field_set_mdt (field);

  /* If at the end of the field and not a fer field and not a word wrap
   * field advance to the next field.
   */
  if (end_of_field && !tn5250_field_is_wordwrap (field))
    {
      if (tn5250_field_is_fer (field))
	{
	  tn5250_display_indicator_set (This, TN5250_DISPLAY_IND_FER);
	  tn5250_display_set_cursor (This,
				     tn5250_field_end_row (field),
				     tn5250_field_end_col (field));
	}
      else
	{
	  tn5250_display_field_adjust (This, field);
	  if (tn5250_field_is_auto_enter (field))
	    {
	      tn5250_display_do_aidkey (This, TN5250_SESSION_AID_ENTER);
	      return;
	    }
	  if (nextfieldprogressionid != 0)
	    {
	      tn5250_display_set_cursor_next_progression_field (This,
								nextfieldprogressionid);
	    }
	  else
	    {
	      /* If we are at the end of the field tn5250_dbuffer_addch()
	       * above may have moved the cursor beyond the end of the
	       * field.  That screws us up in the case of continuous fields
	       * because each individual field does not have a progression
	       * ID set.  Since continuous fields may not be the next
	       * field to the right we need to be sure that the cursor
	       * is currently in the field, not to the right of it.  Doing
	       * so will ensure that the following call puts us in the
	       * next continuous field.  This has the side effect that
	       * inserting into a single character field does not move
	       * the cursor to the next field.  Maybe that's a bug, maybe
	       * that's a feature  :)
	       */
	      tn5250_dbuffer_left (This->display_buffers);
	      tn5250_display_set_cursor_next_field (This);
	    }
	}
    }
  return;
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
void
tn5250_display_shift_right (Tn5250Display * This, Tn5250Field * field,
			    unsigned char fill)
{
  int n, end;
  unsigned char *ptr;

  ptr = tn5250_display_field_data (This, field);
  end = tn5250_field_length (field) - 1;

  tn5250_field_set_mdt (field);

  /* Don't adjust the sign position of signed num type fields. */
  if (tn5250_field_is_signed_num (field))
    {
      end--;
    }

  /* Left fill the field until the first non-null or non-blank character. */
  for (n = 0; n <= end && (ptr[n] == 0 || ptr[n] == 0x40); n++)
    {
      ptr[n] = fill;
    }

  /* If the field is entirely blank and we don't do this, we spin forever. */
  if (n > end)
    {
      return;
    }

  /* Shift the contents of the field right one place and put the fill char in
   * position 0 until the data is right-justified in the field. */
  while (ptr[end] == 0 || ptr[end] == 0x40)
    {
      for (n = end; n > 0; n--)
	{
	  ptr[n] = ptr[n - 1];
	}
      ptr[0] = fill;
    }
  return;
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
void
tn5250_display_field_adjust (Tn5250Display * This, Tn5250Field * field)
{
  int mand_fill_type;

  /* Because of special processing during transmit and other weirdness,
   * we need to shift signed number fields right regardless. */
  mand_fill_type = tn5250_field_mand_fill_type (field);
  if (tn5250_field_type (field) == TN5250_FIELD_SIGNED_NUM)
    {
      mand_fill_type = TN5250_FIELD_RIGHT_BLANK;
    }

  switch (mand_fill_type)
    {
    case TN5250_FIELD_NO_ADJUST:
    case TN5250_FIELD_MANDATORY_FILL:
      break;
    case TN5250_FIELD_RIGHT_ZERO:
      tn5250_display_shift_right (This, field,
				  tn5250_char_map_to_remote (This->map, '0'));
      break;
    case TN5250_FIELD_RIGHT_BLANK:
      tn5250_display_shift_right (This, field,
				  tn5250_char_map_to_remote (This->map, ' '));
      break;
    }

  tn5250_field_set_mdt (field);
  return;
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
void
tn5250_display_do_keys (Tn5250Display * This)
{
  int cur_key;
  char Last;
  int dokey;

  TN5250_LOG (("display_do_keys!\n"));

  do
    {
      cur_key = tn5250_macro_getkey (This, &Last);

      if (Last)
	{
	  tn5250_display_indicator_clear (This, TN5250_DISPLAY_IND_MACRO);
	}

      if (cur_key == 0)
	{
	  cur_key = tn5250_display_getkey (This);
	}

      if (cur_key != -1)
	{
	  tn5250_macro_reckey (This, cur_key);
	  dokey = 0;

	  switch (This->keystate)
	    {
	    case TN5250_KEYSTATE_UNLOCKED:
	      dokey = 1;
	      break;
	    case TN5250_KEYSTATE_HARDWARE:
	      if (cur_key == K_RESET)
		{
		  TN5250_LOG (("doing key %d in hw error state.\n", cur_key));
		}
	      dokey = 1;
	      break;
	    case TN5250_KEYSTATE_LOCKED:
	      switch (cur_key)
		{
		case K_SYSREQ:
		case K_ATTENTION:
		  TN5250_LOG (("doing key %d in locked state.\n", cur_key));
		  dokey = 1;
		  break;
		}
	      break;
	    case TN5250_KEYSTATE_PREHELP:
	      switch (cur_key)
		{
		case K_RESET:
		case K_HELP:
		case K_ATTENTION:
		  dokey = 1;
		  TN5250_LOG (("Doing key %d in prehelp state\n", cur_key));
		  break;
		}
	      break;
	      break;
	    case TN5250_KEYSTATE_POSTHELP:
	      switch (cur_key)
		{
		case K_RESET:
		case K_ATTENTION:
		  TN5250_LOG (("Doing key %d in posthelp state.\n", cur_key));
		  dokey = 1;
		  break;
		}
	    }

	  if (!dokey)
	    {
	      if ((This->key_queue_tail + 1 == This->key_queue_head) ||
		  (This->key_queue_head == 0 &&
		   This->key_queue_tail == TN5250_DISPLAY_KEYQ_SIZE - 1))
		{
		  TN5250_LOG (("Beep: Key queue full.\n"));
		  tn5250_display_beep (This);
		}
	      This->key_queue[This->key_queue_tail] = cur_key;
	      if (++This->key_queue_tail == TN5250_DISPLAY_KEYQ_SIZE)
		{
		  This->key_queue_tail = 0;
		}
	    }
	  else
	    {
	      /* if we're hitting a special keypress (such as error reset)
	         in a state where typeahead is not allowed, then clear
	         the key queue */
	      if (This->key_queue_head != This->key_queue_tail)
		{
		  This->key_queue_head = This->key_queue_tail = 0;
		}
	      tn5250_display_do_key (This, cur_key);
	    }
	}
    }
  while (cur_key != -1);

  tn5250_display_update (This);
  return;
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
void
tn5250_display_do_key (Tn5250Display * This, int key)
{
  int pre_FER_clear = 0;

  TN5250_LOG (("@key %d\n", key));

  /* FIXME: Translate from terminal key via keyboard map to 5250 key. */

  switch (This->keystate)
    {
    case TN5250_KEYSTATE_UNLOCKED:
      TN5250_ASSERT (!tn5250_display_inhibited (This));
      /* normal processing can continue */
      break;
    case TN5250_KEYSTATE_HARDWARE:
      if (key != K_RESET)
	{
	  TN5250_LOG (("Denying key %d in hw err state.\n", key));
	  tn5250_display_beep (This);
	  return;
	}
      break;
    case TN5250_KEYSTATE_LOCKED:
      if (key != K_SYSREQ && key != K_PRINT && key != K_ATTENTION)
	{
	  TN5250_LOG (("Denying key %d in locked state.\n", key));
	  tn5250_display_beep (This);
	  return;
	}
      break;
    case TN5250_KEYSTATE_PREHELP:
      if (key != K_RESET && key != K_HELP
	  && key != K_PRINT && key != K_ATTENTION)
	{
	  TN5250_LOG (("Denying key %d in prehelp state.\n", key));
	  tn5250_display_beep (This);
	  return;
	}
      break;
    case TN5250_KEYSTATE_POSTHELP:
      if (This->uninhibited
          && (key == K_ENTER || key == K_TAB || key == K_BACKTAB
              || key == K_ROLLDN || key == K_ROLLUP
              || (key >= K_FIRST_SPECIAL && key <= K_F24)))
        {
          TN5250_LOG (("Resetting posthelp state for key %d.\n", key));
          tn5250_display_uninhibit (This);
          This->keystate = TN5250_KEYSTATE_UNLOCKED;
        }
      else if (key != K_RESET && key != K_ATTENTION)
	{
	  TN5250_LOG (("Denying key %d in posthelp state.\n", key));
	  tn5250_display_beep (This);
	  return;
	}
      break;
    }


  /* In the case we are in the field exit required state, we inhibit
   * on everything except left arrow, backspace, field exit, field+,
   * field-, and help */
  if ((tn5250_display_indicators (This) & TN5250_DISPLAY_IND_FER) != 0)
    {
      switch (key)
	{
	case K_LEFT:
	case K_BACKSPACE:
	  tn5250_display_indicator_clear (This, TN5250_DISPLAY_IND_FER);
	  return;

	case K_UP:
	case K_DOWN:
	case K_RIGHT:
	  tn5250_display_indicator_clear (This, TN5250_DISPLAY_IND_FER);
	  break;

	case K_ENTER:
	case K_FIELDEXIT:
	case K_FIELDMINUS:
	case K_FIELDPLUS:
	case K_TAB:
	case K_BACKTAB:
	case K_RESET:
	case K_HELP:
	  pre_FER_clear = 1;
	  break;

	default:
	  if (This->uninhibited && key >= K_F1 && key <= K_F24)
	    {
	      pre_FER_clear = 1;
	    }
	  else
	    {
	      This->keystate = TN5250_KEYSTATE_PREHELP;
	      This->keySRC = TN5250_KBDSRC_FER;
	      tn5250_display_inhibit (This);
	      TN5250_LOG (("Denying key %d in FER state.\n", key));
	      return;
	    }
	  break;
	}
    }

  switch (key)
    {
    case K_RESET:
      tn5250_display_uninhibit (This);
      This->keystate = TN5250_KEYSTATE_UNLOCKED;
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

    case K_TESTREQ:
      tn5250_display_do_aidkey (This, TN5250_SESSION_AID_TESTREQ);
      break;

    case K_SYSREQ:
      tn5250_display_do_aidkey (This, TN5250_SESSION_AID_SYSREQ);
      break;

    case K_ATTENTION:
      tn5250_display_uninhibit (This);
      This->keystate = TN5250_KEYSTATE_UNLOCKED;
      tn5250_display_do_aidkey (This, TN5250_SESSION_AID_ATTN);
      break;

    case K_PRINT:
      tn5250_display_do_aidkey (This, TN5250_SESSION_AID_PRINT);
      break;

    case K_DUPLICATE:
      tn5250_display_kf_dup (This);
      break;

    case K_NEXTWORD:
      tn5250_display_kf_nextword (This);
      break;

    case K_PREVWORD:
      tn5250_display_kf_prevword (This);
      break;

    case K_NEXTFLD:
      tn5250_display_kf_nextfld (This);
      break;

    case K_PREVFLD:
      tn5250_display_kf_prevfld (This);
      break;

    case K_FIELDHOME:
      tn5250_display_kf_fieldhome (This);
      break;

    case K_NEWLINE:
      tn5250_display_kf_newline (This);
      break;

    case K_MEMO:
      tn5250_display_kf_macro (This, K_MEMO);
      break;

    case K_EXEC:
      tn5250_display_kf_macro (This, K_EXEC);
      break;

    default:
      /* Handle function/command keys. */
      if (key >= K_F1 && key <= K_F24)
	{
	  if ((!tn5250_macro_recfunct (This, key)) &&
	      (!tn5250_macro_execfunct (This, key)))
	    {
	      if (key <= K_F12)
		{
		  TN5250_LOG (("Key = %d; K_F1 = %d; Key - K_F1 = %d\n",
			       key, K_F1, key - K_F1));
		  TN5250_LOG (("AID_F1 = %d; Result = %d\n",
			       TN5250_SESSION_AID_F1,
			       key - K_F1 + TN5250_SESSION_AID_F1));
		  tn5250_display_do_aidkey (This,
					    key - K_F1 +
					    TN5250_SESSION_AID_F1);
		}
	      else
		{
		  tn5250_display_do_aidkey (This,
					    key - K_F13 +
					    TN5250_SESSION_AID_F13);
		}
	    }
	  break;
	}

      /* Handle data keys. */
      if (key >= ' ' && key <= 255)
	{
	  TN5250_LOG (("HandleKey: key = %c\n", key));
	  tn5250_display_interactive_addch (This, key);
	}
      else
	{
	  TN5250_LOG (("HandleKey: Weird key ignored: %d\n", key));
	}
    }
  if (pre_FER_clear)
    {
      tn5250_display_indicator_clear (This, TN5250_DISPLAY_IND_FER);
    }
  return;
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
void
tn5250_display_field_pad_and_adjust (Tn5250Display * This,
				     Tn5250Field * field)
{
  Tn5250Field *iter;
  unsigned char *data;
  int i, l;

  /* NULL out remainder of field from cursor position.  For signed numeric
   * fields, we do *not* want to null out the sign position - Field+ and
   * Field- will do this for us.  We do not do this when the FER indicator
   * is set.
   */
  if ((tn5250_display_indicators (This) & TN5250_DISPLAY_IND_FER) == 0)
    {
      data = tn5250_display_field_data (This, field);
      i = tn5250_field_count_left (field, tn5250_display_cursor_y (This),
				   tn5250_display_cursor_x (This));
      l = tn5250_field_length (field);

      if (tn5250_field_is_signed_num (field))
	{
	  l--;
	}
      for (; i < l; i++)
	{
	  data[i] = 0;
	}

      if (tn5250_field_is_continued (field)
	  && (!tn5250_field_is_continued_last (field)))
	{
	  iter = field->next;

	  while (tn5250_field_is_continued (iter))
	    {
	      data = tn5250_display_field_data (This, iter);
	      l = tn5250_field_length (iter);

	      for (i = 0; i < l; i++)
		{
		  data[i] = 0;
		}

	      if (tn5250_field_is_continued_last (iter))
		{
		  break;
		}
	      iter = iter->next;
	    }
	}
    }

  tn5250_display_field_adjust (This, field);
  return;
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
void
tn5250_display_kf_field_exit (Tn5250Display * This)
{
  Tn5250Field *field;

  field = tn5250_display_current_field (This);

  if (field == NULL || tn5250_field_is_bypass (field))
    {
      This->keystate = TN5250_KEYSTATE_PREHELP;
      This->keySRC = TN5250_KBDSRC_PROTECT;
      tn5250_display_inhibit (This);
      return;
    }

  tn5250_display_field_pad_and_adjust (This, field);

  if (tn5250_field_is_auto_enter (field))
    {
      tn5250_display_do_aidkey (This, TN5250_SESSION_AID_ENTER);
      return;
    }

  tn5250_display_set_cursor_next_logical_field (This);
  return;
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
void
tn5250_display_kf_field_plus (Tn5250Display * This)
{
  Tn5250Field *field;
  unsigned char *data;

  TN5250_LOG (("Field+ entered.\n"));

  field = tn5250_display_current_field (This);

  if (field == NULL || tn5250_field_is_bypass (field))
    {
      This->keystate = TN5250_KEYSTATE_PREHELP;
      This->keySRC = TN5250_KBDSRC_PROTECT;
      tn5250_display_inhibit (This);
      return;
    }

  tn5250_display_field_pad_and_adjust (This, field);

  /* NOTE: Field+ should act like field exit on a non-numeric field. */
  if ((field != NULL) &&
      ((tn5250_field_type (field) == TN5250_FIELD_SIGNED_NUM) ||
       (tn5250_field_type (field) == TN5250_FIELD_NUM_ONLY)))
    {

      /* We don't do anything for number only fields.  For signed numeric
       * fields, we change the sign position to a '+'. */
      data = tn5250_display_field_data (This, field);

      if (tn5250_field_type (field) != TN5250_FIELD_NUM_ONLY)
	{
	  data[tn5250_field_length (field) - 1] = 0;
	}
    }

  if (tn5250_field_is_auto_enter (field))
    {
      tn5250_display_do_aidkey (This, TN5250_SESSION_AID_ENTER);
      return;
    }

  tn5250_display_set_cursor_next_logical_field (This);
  return;
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
void
tn5250_display_kf_field_minus (Tn5250Display * This)
{
  Tn5250Field *field;
  unsigned char *data;

  TN5250_LOG (("Field- entered.\n"));

  field = tn5250_display_current_field (This);

  if ((field == NULL) ||
      ((tn5250_field_type (field) != TN5250_FIELD_SIGNED_NUM) &&
       (tn5250_field_type (field) != TN5250_FIELD_NUM_ONLY)))
    {
      if (This->field_minus_in_char) {
         tn5250_display_kf_field_exit(This);
         return;
      }
      This->keystate = TN5250_KEYSTATE_PREHELP;
      This->keySRC = TN5250_KBDSRC_FLDM_DISALLOWED;
      tn5250_display_inhibit (This);
      return;
    }

  tn5250_display_field_pad_and_adjust (This, field);

  /* For numeric only fields, we shift the data one character to the
   * left and change the zone to negative.  i.e. 0xf0 becomes 0xd0,
   * 0xf1 becomes 0xd1, etc.  in the rightmost position.  For
   * signed numeric fields, we change the sign position to a '-'. */
  data = tn5250_display_field_data (This, field);

  if (tn5250_field_type (field) == TN5250_FIELD_NUM_ONLY)
    {
      int i = tn5250_field_length (field) - 1;
      data[i] = (data[i] & 0x0F) | 0xd0;
    }
  else
    {
      data[tn5250_field_length (field) - 1] =
	tn5250_char_map_to_remote (This->map, '-');
    }

  if (tn5250_field_is_auto_enter (field))
    {
      tn5250_display_do_aidkey (This, TN5250_SESSION_AID_ENTER);
      return;
    }

  tn5250_display_set_cursor_next_logical_field (This);
  return;
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
void
tn5250_display_do_aidkey (Tn5250Display * This, int aidcode)
{
  TN5250_LOG (("tn5250_display_do_aidkey (0x%02X) called.\n", aidcode));

  /* Aidcodes less than zero are pseudo-aid-codes (see session.h) */
  if (This->session->read_opcode || aidcode < 0)
    {
      /* FIXME: If this returns zero, we need to stop processing. */
      (*(This->session->handle_aidkey)) (This->session, aidcode);
    }
  return;
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
void
tn5250_display_kf_dup (Tn5250Display * This)
{
  int i;
  Tn5250Field *field;
  unsigned char *data;

  field = tn5250_display_current_field (This);
  if (field == NULL || tn5250_field_is_bypass (field))
    {
      This->keystate = TN5250_KEYSTATE_PREHELP;
      This->keySRC = TN5250_KBDSRC_PROTECT;
      tn5250_display_inhibit (This);
      return;
    }

  tn5250_field_set_mdt (field);

  if (!tn5250_field_is_dup_enable (field))
    {
      This->keystate = TN5250_KEYSTATE_PREHELP;
      This->keySRC = TN5250_KBDSRC_DUP_DISALLOWED;
      tn5250_display_inhibit (This);
      return;
    }

  i = tn5250_field_count_left (field, tn5250_display_cursor_y (This),
			       tn5250_display_cursor_x (This));
  data = tn5250_display_field_data (This, field);
  for (; i < tn5250_field_length (field); i++)
    {
      data[i] = 0x1c;
    }

  if (tn5250_field_is_fer (field))
    {
      tn5250_display_indicator_set (This, TN5250_DISPLAY_IND_FER);
      tn5250_dbuffer_cursor_set (This->display_buffers,
				 tn5250_field_end_row (field),
				 tn5250_field_end_col (field));
    }
  else
    {
      tn5250_display_field_adjust (This, field);
      if (tn5250_field_is_auto_enter (field))
	{
	  tn5250_display_do_aidkey (This, TN5250_SESSION_AID_ENTER);
	  return;
	}
      tn5250_display_set_cursor_next_field (This);
    }
  return;
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
void
tn5250_display_indicator_set (Tn5250Display * This, int inds)
{
  This->indicators |= inds;
  This->indicators_dirty = 1;
  return;
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
void
tn5250_display_indicator_clear (Tn5250Display * This, int inds)
{
  This->indicators &= ~inds;
  This->indicators_dirty = 1;

  /* Restore the message line if we are clearing the X II indicator */
  if ((inds & TN5250_DISPLAY_IND_INHIBIT) != 0
      && This->saved_msg_line != NULL)
    {
      int l = tn5250_dbuffer_msg_line (This->display_buffers);
      memcpy (This->display_buffers->data +
	      l * tn5250_display_width (This), This->saved_msg_line,
	      tn5250_display_width (This));
      free (This->saved_msg_line);
      This->saved_msg_line = NULL;
      free (This->msg_line);
      This->msg_line = NULL;
    }
  return;
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
void
tn5250_display_clear_unit (Tn5250Display * This)
{
  tn5250_dbuffer_set_size (This->display_buffers, 24, 80);
  tn5250_display_indicator_set (This, TN5250_DISPLAY_IND_X_SYSTEM);
  This->keystate = TN5250_KEYSTATE_LOCKED;
  tn5250_display_indicator_clear (This,
				  TN5250_DISPLAY_IND_INSERT |
				  TN5250_DISPLAY_IND_INHIBIT |
				  TN5250_DISPLAY_IND_FER);
  This->pending_insert = 0;
  tn5250_dbuffer_set_ic (This->display_buffers, 0, 0);
  if (This->saved_msg_line != NULL)
    {
      free (This->saved_msg_line);
      This->saved_msg_line = NULL;
    }
  if (This->msg_line != NULL)
    {
      free (This->msg_line);
      This->msg_line = NULL;
    }
  return;
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
void
tn5250_display_clear_unit_alternate (Tn5250Display * This)
{
  tn5250_dbuffer_set_size (This->display_buffers, 27, 132);
  tn5250_display_indicator_set (This, TN5250_DISPLAY_IND_X_SYSTEM);
  This->keystate = TN5250_KEYSTATE_LOCKED;
  tn5250_display_indicator_clear (This,
				  TN5250_DISPLAY_IND_INSERT |
				  TN5250_DISPLAY_IND_INHIBIT |
				  TN5250_DISPLAY_IND_FER);
  This->pending_insert = 0;
  tn5250_dbuffer_set_ic (This->display_buffers, 0, 0);
  if (This->saved_msg_line != NULL)
    {
      free (This->saved_msg_line);
      This->saved_msg_line = NULL;
    }
  if (This->msg_line != NULL)
    {
      free (This->msg_line);
      This->msg_line = NULL;
    }
  return;
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
void
tn5250_display_clear_format_table (Tn5250Display * This)
{
  tn5250_dbuffer_clear_table (This->display_buffers);
  tn5250_display_set_cursor (This, 0, 0);
  tn5250_display_indicator_set (This, TN5250_DISPLAY_IND_X_SYSTEM);
  This->keystate = TN5250_KEYSTATE_LOCKED;
  tn5250_display_indicator_clear (This, TN5250_DISPLAY_IND_INSERT);
  return;
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
void
tn5250_display_kf_backspace (Tn5250Display * This)
{
  Tn5250Field *field = tn5250_display_current_field (This);
  if (field == NULL)
    {
      This->keystate = TN5250_KEYSTATE_PREHELP;
      This->keySRC = TN5250_KBDSRC_PROTECT;
      tn5250_display_inhibit (This);
      return;
    }

  /* If in first position of field, set cursor position to last position
   * of previous field. */
  if (tn5250_display_cursor_x (This) == tn5250_field_start_col (field) &&
      tn5250_display_cursor_y (This) == tn5250_field_start_row (field))
    {
      field = tn5250_display_prev_field (This);
      if (field == NULL)
	{
	  return;		/* Should never happen */
	}
      tn5250_display_set_cursor_field (This, field);
      if (tn5250_field_length (field) - 1 > 0)
	{
	  tn5250_dbuffer_right (This->display_buffers,
				tn5250_field_length (field) - 1);
	}
      return;
    }

  tn5250_dbuffer_left (This->display_buffers);
  return;
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
void
tn5250_display_kf_left (Tn5250Display * This)
{
  tn5250_dbuffer_left (This->display_buffers);
  return;
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
void
tn5250_display_kf_right (Tn5250Display * This)
{
  tn5250_dbuffer_right (This->display_buffers, 1);
  return;
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
void
tn5250_display_kf_up (Tn5250Display * This)
{
  tn5250_dbuffer_up (This->display_buffers);
  return;
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
void
tn5250_display_kf_down (Tn5250Display * This)
{
  tn5250_dbuffer_down (This->display_buffers);
  return;
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
void
tn5250_display_kf_insert (Tn5250Display * This)
{
  if ((tn5250_display_indicators (This) & TN5250_DISPLAY_IND_INSERT) != 0)
    {
      tn5250_display_indicator_clear (This, TN5250_DISPLAY_IND_INSERT);
    }
  else
    {
      tn5250_display_indicator_set (This, TN5250_DISPLAY_IND_INSERT);
    }
  return;
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
void
tn5250_display_kf_tab (Tn5250Display * This)
{
  tn5250_display_set_cursor_next_logical_field (This);
  return;
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
void
tn5250_display_kf_backtab (Tn5250Display * This)
{
  /* Backtab: Move to start of this field, or start of previous field if
   * already there.
   */
  Tn5250Field *field = tn5250_display_current_field (This);

  if (field == NULL || tn5250_field_count_left (field,
						tn5250_display_cursor_y
						(This),
						tn5250_display_cursor_x
						(This)) == 0)
    {
      tn5250_display_set_cursor_prev_logical_field (This);
      return;
    }

  if (field != NULL)
    {
      tn5250_display_set_cursor_field (This, field);
    }
  else
    {
      tn5250_display_set_cursor_home (This);
    }
  return;
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
void
tn5250_display_kf_end (Tn5250Display * This)
{
  Tn5250Field *field = tn5250_display_current_field (This);
  if (field != NULL && !tn5250_field_is_bypass (field))
    {
      unsigned char *data = tn5250_display_field_data (This, field);
      int i = tn5250_field_length (field) - 1;
      int y = tn5250_field_start_row (field);
      int x = tn5250_field_start_col (field);

      if (data[i] == '\0')
	{
	  while (i > 0 && data[i] == '\0')
	    {
	      i--;
	    }
	  while (i >= 0)
	    {
	      if (++x == tn5250_display_width (This))
		{
		  x = 0;
		  if (++y == tn5250_display_height (This))
		    {
		      y = 0;
		    }
		}
	      i--;
	    }
	}
      else
	{
	  y = tn5250_field_end_row (field);
	  x = tn5250_field_end_col (field);
	}
      tn5250_display_set_cursor (This, y, x);
    }
  else
    {
      This->keystate = TN5250_KEYSTATE_PREHELP;
      This->keySRC = TN5250_KBDSRC_PROTECT;
      tn5250_display_inhibit (This);
    }
  return;
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
void
tn5250_display_kf_home (Tn5250Display * This)
{
  Tn5250Field *field;
  int gx, gy;

  if (This->pending_insert)
    {
      gy = This->display_buffers->tcy;
      gx = This->display_buffers->tcx;
    }
  else
    {
      field = tn5250_dbuffer_first_non_bypass (This->display_buffers);
      if (field != NULL)
	{
	  gy = tn5250_field_start_row (field);
	  gx = tn5250_field_start_col (field);
	}
      else
	{
	  gx = gy = 0;
	}
    }

  if (gy == tn5250_display_cursor_y (This)
      && gx == tn5250_display_cursor_x (This))
    {
      tn5250_display_do_aidkey (This, TN5250_SESSION_AID_RECORD_BS);
    }
  else
    {
      tn5250_display_set_cursor (This, gy, gx);
    }
  return;
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
void
tn5250_display_kf_delete (Tn5250Display * This)
{
  Tn5250Field *field = tn5250_display_current_field (This);

  if (field == NULL || tn5250_field_is_bypass (field))
    {
      This->keystate = TN5250_KEYSTATE_PREHELP;
      This->keySRC = TN5250_KBDSRC_PROTECT;
      tn5250_display_inhibit (This);
    }
  else
    {
      tn5250_field_set_mdt (field);

      /* If this field is word wrap handle it differently */
      if (tn5250_field_is_wordwrap (field))
	{
	  tn5250_display_wordwrap_delete (This);
	  return;
	}

      tn5250_dbuffer_del (This->display_buffers, field->id,
			  tn5250_field_count_right (field,
						    tn5250_display_cursor_y
						    (This),
						    tn5250_display_cursor_x
						    (This)));
    }
  return;
}


/****f* lib5250/tn5250_display_kf_prevword
 * NAME
 *    tn5250_display_kf_prevword
 * SYNOPSIS
 *    tn5250_display_kf_prevword (This);
 * INPUTS
 *    Tn5250Display *      This       - 
 * DESCRIPTION
 *    Move cursor to the last non-blank character.
 *****/
void
tn5250_display_kf_prevword (Tn5250Display * This)
{
  tn5250_dbuffer_prevword (This->display_buffers);
  return;
}


/****f* lib5250/tn5250_display_kf_nextword
 * NAME
 *    tn5250_display_kf_nextword
 * SYNOPSIS
 *    tn5250_display_kf_nextword (This);
 * INPUTS
 *    Tn5250Display *      This       - 
 * DESCRIPTION
 *    Move cursor to the next non-blank character.
 *****/
void
tn5250_display_kf_nextword (Tn5250Display * This)
{
  tn5250_dbuffer_nextword (This->display_buffers);
  return;
}


/****f* lib5250/tn5250_display_kf_prevfld
 * NAME
 *    tn5250_display_kf_prevfld
 * SYNOPSIS
 *    tn5250_display_kf_prevfld (This);
 * INPUTS
 *    Tn5250Display *      This       -
 * DESCRIPTION
 *    Move the cursor backward to the beginning of the previous non-blank
 *    or input field.  The intent is to emulate the dbuffer_prevword function
 *    and add the additional functionality of stopping at a (possibly blank)
 *    input-capable field as well.
 *****/
void
tn5250_display_kf_prevfld (Tn5250Display * This)
{
  int state = 0;
  int maxiter;
  Tn5250Field *field;

  TN5250_LOG (("dbuffer_prevfld: entered.\n"));

  maxiter = (This->display_buffers->w * This->display_buffers->h);
  TN5250_ASSERT (maxiter > 0);

  while (--maxiter)
    {
      tn5250_dbuffer_left (This->display_buffers);

      /* If at the start of a field, exit */
      field = tn5250_display_current_field (This);
      if ((field != NULL) &&
	  (tn5250_field_start_row (field) == This->display_buffers->cy) &&
	  (tn5250_field_start_col (field) == This->display_buffers->cx))
	{
	  break;
	}

      switch (state)
	{
	case 0:
	  if (This->display_buffers->data[This->display_buffers->cy *
					  This->display_buffers->w +
					  This->display_buffers->cx] <= 0x40)
	    {
	      state++;
	    }
	  break;
	case 1:
	  if (This->display_buffers->data[This->display_buffers->cy *
					  This->display_buffers->w +
					  This->display_buffers->cx] > 0x40)
	    {
	      state++;
	    }
	  break;
	case 2:
	  if (This->display_buffers->data[This->display_buffers->cy *
					  This->display_buffers->w +
					  This->display_buffers->cx] <= 0x40)
	    {
	      tn5250_dbuffer_right (This->display_buffers, 1);
	      return;
	    }
	  break;
	}
    }
  return;
}


/****f* lib5250/tn5250_display_kf_nextfld
 * NAME
 *    tn5250_display_kf_nextfld
 * SYNOPSIS
 *    tn5250_display_kf_nextfld (This);
 * INPUTS
 *    Tn5250Display *      This       -
 * DESCRIPTION
 *    Move the cursor forward to the beginning of the next non-blank
 *    or input field.  The intent is to emulate the dbuffer_nextword function
 *    and add the additional functionality of stopping at a (possibly blank)
 *    input-capable field as well.
 *****/
void
tn5250_display_kf_nextfld (Tn5250Display * This)
{
  int foundblank = 0;
  int maxiter;
  Tn5250Field *field;

  TN5250_LOG (("dbuffer_nextfld: entered.\n"));

  maxiter = (This->display_buffers->w * This->display_buffers->h);
  TN5250_ASSERT (maxiter > 0);

  while (--maxiter)
    {
      tn5250_dbuffer_right (This->display_buffers, 1);
      if (This->display_buffers->data[This->display_buffers->cy *
				      This->display_buffers->w +
				      This->display_buffers->cx] <= 0x40)
	{
	  foundblank++;
	}

      /* If found a blank previously and a non-blank now, exit */
      if ((foundblank)
	  && (This->display_buffers->
	      data[This->display_buffers->cy * This->display_buffers->w +
		   This->display_buffers->cx] > 0x40))
	{
	  break;
	}

      /* If at the start of a field, exit */
      field = tn5250_display_current_field (This);
      if ((field != NULL) &&
	  (tn5250_field_start_row (field) == This->display_buffers->cy) &&
	  (tn5250_field_start_col (field) == This->display_buffers->cx))
	{
	  break;
	}
    }
  return;
}


/****f* lib5250/tn5250_display_kf_fieldhome
 * NAME
 *    tn5250_display_kf_fieldhome
 * SYNOPSIS
 *    tn5250_display_kf_fieldhome (This);
 * INPUTS
 *    Tn5250Display *      This       - 
 * DESCRIPTION
 *    Move cursor to the next non-blank character.
 *****/
void
tn5250_display_kf_fieldhome (Tn5250Display * This)
{
  Tn5250Field *field = tn5250_display_current_field (This);
  if (field != NULL && !tn5250_field_is_bypass (field))
    {
      int y = tn5250_field_start_row (field);
      int x = tn5250_field_start_col (field);
      tn5250_display_set_cursor (This, y, x);
    }
  else
    {
      This->keystate = TN5250_KEYSTATE_PREHELP;
      This->keySRC = TN5250_KBDSRC_PROTECT;
      tn5250_display_inhibit (This);
    }
  return;
}


/****f* lib5250/tn5250_display_kf_newline
 * NAME
 *    tn5250_display_kf_newline
 * SYNOPSIS
 *    tn5250_display_kf_newline (This);
 * INPUTS
 *    Tn5250Display *      This       - 
 * DESCRIPTION
 *    Move cursor to the first field on the next line.
 *****/
void
tn5250_display_kf_newline (Tn5250Display * This)
{
  int y;
  Tn5250Field *f;

  y = tn5250_display_cursor_y (This);
  tn5250_display_set_cursor (This, y, 0);
  tn5250_dbuffer_down (This->display_buffers);
  f = tn5250_display_current_field (This);
  if (f == NULL || tn5250_field_is_bypass (f))
    {
      tn5250_display_set_cursor_next_field (This);
    }
  return;
}


/****f* lib5250/tn5250_display_kf_macro
 * NAME
 *    tn5250_display_kf_macro
 * SYNOPSIS
 *    tn5250_display_kf_macro (This);
 * INPUTS
 *    Tn5250Display *      This       - 
 *    int		   Ch	    - choice : K_MEMO | K_EXEC
 * DESCRIPTION
 *    Toggle the memo/exec indicator.
 *****/
void
tn5250_display_kf_macro (Tn5250Display * This, int Ch)
{
  TN5250_LOG (("K_MEMO/EXEC\n"));

  if ((Ch == K_MEMO) && (!tn5250_macro_estate (This)))
    {
      if (!tn5250_macro_rstate (This))
	{
	  if (tn5250_macro_startdef (This))
	    {
	      tn5250_display_indicator_set (This, TN5250_DISPLAY_IND_MACRO);
	    }
	}
      else
	{
	  tn5250_macro_enddef (This);
	  tn5250_display_indicator_clear (This, TN5250_DISPLAY_IND_MACRO);
	}
    }

  if ((Ch == K_EXEC) && (!tn5250_macro_rstate (This)))
    {
      if (!tn5250_macro_estate (This))
	{
	  if (tn5250_macro_startexec (This))
	    {
	      tn5250_display_indicator_set (This, TN5250_DISPLAY_IND_MACRO);
	    }
	}
      else
	{
	  tn5250_macro_endexec (This);
	  tn5250_display_indicator_clear (This, TN5250_DISPLAY_IND_MACRO);
	}
    }
  return;
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
void
tn5250_display_save_msg_line (Tn5250Display * This)
{
  int l;

  if (This->saved_msg_line != NULL)
    {
      free (This->saved_msg_line);
    }
  This->saved_msg_line =
    (unsigned char *) malloc (tn5250_display_width (This));

  l = tn5250_dbuffer_msg_line (This->display_buffers);
  memcpy (This->saved_msg_line, This->display_buffers->data +
	  tn5250_display_width (This) * l, tn5250_display_width (This));
  return;
}


/****f* lib5250/tn5250_display_set_msg_line
 * NAME
 *    tn5250_display_set_msg_line
 * SYNOPSIS
 *    tn5250_display_set_msg_line (This);
 * INPUTS
 *    Tn5250Display *      This       -
 *    const unsigned char *msgline    -
 *    int                  msglen     -
 * DESCRIPTION
 *    Set a new message line.
 *****/
void
tn5250_display_set_msg_line (Tn5250Display * This,
			     const unsigned char *msgline, int msglen)
{
  int l;

  if (This->msg_line != NULL)
    {
      free (This->msg_line);
    }
  This->msg_line = (unsigned char *) malloc (tn5250_display_width (This));
  memset (This->msg_line, 0x00, tn5250_display_width (This));

  memcpy (This->msg_line, msgline, msglen);
  This->msg_len = msglen;

  l = tn5250_dbuffer_msg_line (This->display_buffers);
  memcpy (This->display_buffers->data + tn5250_display_width (This) * l,
	  This->msg_line, This->msg_len);
  return;
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
void
tn5250_display_set_char_map (Tn5250Display * This, const char *name)
{
  Tn5250CharMap *map = tn5250_char_map_new (name);
  TN5250_ASSERT (map != NULL);
  if (This->map != NULL)
    {
      tn5250_char_map_destroy (This->map);
    }
  This->map = map;
  return;
}


/****f* lib5250/tn5250_display_erase_region
 * NAME
 *    tn5250_display_erase_region
 * SYNOPSIS
 *    tn5250_display_erase_region (This, startrow, startcol, endrow, endcol,
 *                                 leftedge, rightedge);
 * INPUTS
 *    Tn5250Display *      This       - 
 *    unsigned int         startrow   - 
 *    unsigned int         startcol   - 
 *    unsigned int         endrow     - 
 *    unsigned int         endcol     - 
 *    unsigned int         leftedge   - 
 *    unsigned int         rightedge  - 
 * DESCRIPTION
 *    Erase a region of the buffer
 *****/
void
tn5250_display_erase_region (Tn5250Display * This, unsigned int startrow,
			     unsigned int startcol, unsigned int endrow,
			     unsigned int endcol, unsigned int leftedge,
			     unsigned int rightedge)
{
  int i, j;

  if (startrow == endrow)
    {
      for (j = startcol - 1; j < endcol; j++)
	{
	  This->display_buffers->
	    data[((startrow - 1) * This->display_buffers->w) + j] =
	    tn5250_char_map_to_remote (This->map, ' ');
	}
    }
  else
    {
      for (i = startrow - 1; i < endrow; i++)
	{
	  if (i == (startrow - 1))
	    {
	      for (j = startcol - 1; j < rightedge; j++)
		{
		  This->display_buffers->data[(i * This->display_buffers->w) +
					      j] =
		    tn5250_char_map_to_remote (This->map, ' ');
		}
	    }
	  else if (i == (endrow - 1))
	    {
	      for (j = leftedge - 1; j < endcol; j++)
		{
		  This->display_buffers->data[(i * This->display_buffers->w) +
					      j] =
		    tn5250_char_map_to_remote (This->map, ' ');
		}
	    }
	  else
	    {
	      for (j = leftedge - 1; j < rightedge; j++)
		{
		  This->display_buffers->data[(i * This->display_buffers->w) +
					      j] =
		    tn5250_char_map_to_remote (This->map, ' ');
		}
	    }
	}
    }
  return;
}


/****f* lib5250/tn5250_display_wordwrap_delete
 * NAME
 *    tn5250_display_wordwrap_delete
 * SYNOPSIS
 *    tn5250_display_wordwrap_delete (This);
 * INPUTS
 *    Tn5250Display *      This       - 
 * DESCRIPTION
 *    Delete key function.
 *****/
void
tn5250_display_wordwrap_delete (Tn5250Display * This)
{
  Tn5250Field *field = tn5250_display_current_field (This);
  Tn5250Field *iter;
  int buflen;
  unsigned char *text, *ptr;
  unsigned char *data;
  unsigned char espace = TN5250_DISPLAY_WORD_WRAP_SPACE;


  /* Use code from x5250 (with permission).  The basic idea here is to
   * copy all the fields starting with the field we are currently in
   * until the last field in this wordwrap group (the last field isn't
   * marked continuous - but will always be the next field, or so say
   * the docs).  Delete the character at the current cursor location for
   * only the current field.  Then copy the result and concatenate the
   * remaining fields into a single buffer.  Once that is done, word wrap
   * the result and copy it back into the field buffers.
   */

  tn5250_dbuffer_del_this_field_only (This->display_buffers,
				      tn5250_field_count_right (field,
								tn5250_display_cursor_y
								(This),
								tn5250_display_cursor_x
								(This)));

  /* First allocate enough space to do the copying.  This will be sum of
   * the lengths of the word wrap fields in this group starting from the
   * current field.
   */

  /* Find out how much to allocate (do this separately so we can do the
   * actual allocation in one step).
   */
  buflen = 0;
  for (iter = field; tn5250_field_is_wordwrap (iter); iter = iter->next)
    {
      buflen = buflen + tn5250_field_length (iter) + 1;
    }
  buflen = buflen + tn5250_field_length (iter);

  /* Now allocate */
  text = (unsigned char *) malloc (buflen * sizeof (unsigned char));
  ptr = text;

  /* Now repeat the loop above and this time copy the data over.  Insert
   * a space after the end of each field since spaces are implied at the
   * ends of word wrap fields.
   */
  for (iter = field; tn5250_field_is_wordwrap (iter); iter = iter->next)
    {
      data = tn5250_display_field_data (This, iter);
      memcpy (ptr, data, tn5250_field_length (iter) * sizeof (unsigned char));
      ptr = ptr + (tn5250_field_length (iter) * sizeof (unsigned char));
      memcpy (ptr, &espace, sizeof (unsigned char));
      ptr = ptr + sizeof (unsigned char);
    }
  data = tn5250_display_field_data (This, iter);
  memcpy (ptr, data, tn5250_field_length (iter) * sizeof (unsigned char));
  tn5250_display_wordwrap (This, text, buflen, tn5250_field_length (field),
			   field);

  free (text);
  return;
}


/****f* lib5250/tn5250_display_wordwrap_insert
 * NAME
 *    tn5250_display_wordwrap_insert
 * SYNOPSIS
 *    tn5250_display_wordwrap_insert (This);
 * INPUTS
 *    Tn5250Display *      This       - 
 * DESCRIPTION
 *    Insert key function.
 *****/
void
tn5250_display_wordwrap_insert (Tn5250Display * This, unsigned char c,
				int shiftcount)
{
  Tn5250Field *field = tn5250_display_current_field (This);
  Tn5250Field *iter;
  int x, y, i;
  unsigned char c2;
  int buflen;
  unsigned char *text, *ptr;
  unsigned char *data;
  unsigned char espace = TN5250_DISPLAY_WORD_WRAP_SPACE;

  /* First allocate enough space to do the copying.  This will be sum of
   * the lengths of the word wrap fields in this group starting from the
   * current field.
   */

  /* Find out how much to allocate (do this separately so we can do the
   * actual allocation in one step).
   */
  buflen = 0;

  /* If we just inserted a space near the beginning of a word wrap field
   * that is not the first of a group, we may need to word wrap the prior
   * field as well since we may have just made a word short enough that it
   * should be on the previous line.  All word wrap fields are continuous
   * fields so a check to see if the previous field is a middle field
   * should do the trick.
   */
  if (!tn5250_field_is_continued_first (field))
    {
      iter = field->prev;
    }
  else
    {
      iter = field;
    }
  for (; tn5250_field_is_wordwrap (iter); iter = iter->next)
    {
      buflen = buflen + tn5250_field_length (iter) + 1;
    }
  buflen = buflen + tn5250_field_length (iter);

  /* Now allocate */
  text = (unsigned char *) malloc (buflen * sizeof (unsigned char));
  ptr = text;

  if (!tn5250_field_is_continued_first (field))
    {
      iter = field->prev;
      data = tn5250_display_field_data (This, iter);
      memcpy (ptr, data, tn5250_field_length (iter) * sizeof (unsigned char));
      ptr = ptr + (tn5250_field_length (iter) * sizeof (unsigned char));
      memcpy (ptr, &espace, sizeof (unsigned char));
      ptr = ptr + sizeof (unsigned char);
    }

  /* Use our own version of tn5250_dbuffer_ins().  We can't use the real
   * version because the insert may cause the current field data to be
   * wider than the field.  That's ok because we're going to word wrap it
   * and that will fit the data to the field(s).
   *
   * We need to put the entire field into the buffer, so start at the
   * beginning of the field, then copy the inserted data.
   */
  x = tn5250_field_start_col (field);
  y = tn5250_field_start_row (field);

  for (i = 0; i < tn5250_field_length (field) - shiftcount - 1; i++)
    {
      c2 = This->display_buffers->data[y * This->display_buffers->w + x];
      memcpy (ptr, &c2, sizeof (unsigned char));
      ptr = ptr + sizeof (unsigned char);
      if (++x == This->display_buffers->w)
	{
	  x = 0;
	  y++;
	}
    }

  x = This->display_buffers->cx;
  y = This->display_buffers->cy;

  for (; i < tn5250_field_length (field); i++)
    {
      c2 = This->display_buffers->data[y * This->display_buffers->w + x];
      memcpy (ptr, &c, sizeof (unsigned char));
      ptr = ptr + sizeof (unsigned char);
      c = c2;
      if (++x == This->display_buffers->w)
	{
	  x = 0;
	  y++;
	}
    }
  memcpy (ptr, &c, sizeof (unsigned char));
  ptr = ptr + sizeof (unsigned char);
  memcpy (ptr, &espace, sizeof (unsigned char));
  ptr = ptr + sizeof (unsigned char);

  /* Now repeat the loop above starting with the next field and this time
   * copy the data over.  Insert a space after the end of each field since
   * spaces are implied at the ends of word wrap fields.
   */
  for (iter = field->next; tn5250_field_is_wordwrap (iter); iter = iter->next)
    {
      data = tn5250_display_field_data (This, iter);
      memcpy (ptr, data, tn5250_field_length (iter) * sizeof (unsigned char));
      ptr = ptr + (tn5250_field_length (iter) * sizeof (unsigned char));
      memcpy (ptr, &espace, sizeof (unsigned char));
      ptr = ptr + sizeof (unsigned char);
    }
  data = tn5250_display_field_data (This, iter);
  memcpy (ptr, data, tn5250_field_length (iter) * sizeof (unsigned char));

  if (!tn5250_field_is_continued_first (field))
    {
      tn5250_display_wordwrap (This, text, buflen,
			       tn5250_field_length (field), field->prev);
    }
  else
    {
      tn5250_display_wordwrap (This, text, buflen,
			       tn5250_field_length (field), field);
    }

  tn5250_dbuffer_right (This->display_buffers, 1);

  /* We have to adjust the cursor position ourselves in the case of the
   * cursor being outside the end of the field.
   *
   * Note that we *have* to handle all cursor placement ourselves because
   * tn5250_display_wordwrap() can possibly put the cursor anywhere
   * (don't worry, it is supposed to).  But our caller doesn't have enough
   * info to intelligently place the cursor.
   */
  if (tn5250_display_cursor_x (This) > tn5250_field_end_col (field))
    {
      tn5250_dbuffer_left (This->display_buffers);
      tn5250_display_set_cursor_next_field (This);
    }

  free (text);
  return;
}


/****f* lib5250/tn5250_display_wordwrap_addch
 * NAME
 *    tn5250_display_wordwrap_addch
 * SYNOPSIS
 *    tn5250_display_wordwrap_addch (This);
 * INPUTS
 *    Tn5250Display *      This       - 
 * DESCRIPTION
 *    Addch key function.
 *****/
void
tn5250_display_wordwrap_addch (Tn5250Display * This, unsigned char c)
{
  Tn5250Field *field = tn5250_display_current_field (This);
  Tn5250Field *iter;
  int buflen;
  unsigned char *text, *ptr;
  unsigned char *data;
  unsigned char espace = TN5250_DISPLAY_WORD_WRAP_SPACE;


  /* Use our own version of tn5250_dbuffer_addch().  We can't use the real
   * version because we don't want to advance the cursor position.
   */
  This->display_buffers->
    data[(This->display_buffers->cy * This->display_buffers->w) +
	 This->display_buffers->cx] = c;

  /* First allocate enough space to do the copying.  This will be sum of
   * the lengths of the word wrap fields in this group starting from the
   * current field.
   */

  /* Find out how much to allocate (do this separately so we can do the
   * actual allocation in one step).
   */
  buflen = 0;

  /* If we just added a space near the beginning of a word wrap field
   * that is not the first of a group, we may need to word wrap the prior
   * field as well since we may have just made a word short enough that it
   * should be on the previous line.  All word wrap fields are continuous
   * fields so a check to see if the previous field is a middle field
   * should do the trick.
   */
  if (!tn5250_field_is_continued_first (field))
    {
      iter = field->prev;
    }
  else
    {
      iter = field;
    }
  for (; tn5250_field_is_wordwrap (iter); iter = iter->next)
    {
      buflen = buflen + tn5250_field_length (iter) + 1;
    }
  buflen = buflen + tn5250_field_length (iter);

  /* Now allocate */
  text = (unsigned char *) malloc (buflen * sizeof (unsigned char));
  ptr = text;

  if (!tn5250_field_is_continued_first (field))
    {
      data = tn5250_display_field_data (This, field->prev);
      memcpy (ptr, data,
	      tn5250_field_length (field->prev) * sizeof (unsigned char));
      ptr =
	ptr + (tn5250_field_length (field->prev) * sizeof (unsigned char));
      memcpy (ptr, &espace, sizeof (unsigned char));
      ptr = ptr + sizeof (unsigned char);
    }

  /* Now repeat the loop above starting with the next field and this time
   * copy the data over.  Insert a space after the end of each field since
   * spaces are implied at the ends of word wrap fields.
   */
  for (iter = field; tn5250_field_is_wordwrap (iter); iter = iter->next)
    {
      data = tn5250_display_field_data (This, iter);
      memcpy (ptr, data, tn5250_field_length (iter) * sizeof (unsigned char));
      ptr = ptr + (tn5250_field_length (iter) * sizeof (unsigned char));
      memcpy (ptr, &espace, sizeof (unsigned char));
      ptr = ptr + sizeof (unsigned char);
    }
  data = tn5250_display_field_data (This, iter);
  memcpy (ptr, data, tn5250_field_length (iter) * sizeof (unsigned char));

  if (!tn5250_field_is_continued_first (field))
    {
      tn5250_display_wordwrap (This, text, buflen,
			       tn5250_field_length (field), field->prev);
    }
  else
    {
      tn5250_display_wordwrap (This, text, buflen,
			       tn5250_field_length (field), field);
    }

  tn5250_dbuffer_right (This->display_buffers, 1);

  /* We have to adjust the cursor position ourselves in the case of the
   * cursor being outside the end of the field.
   *
   * Note that we *have* to handle all cursor placement ourselves because
   * tn5250_display_wordwrap() can possibly put the cursor anywhere
   * (don't worry, it is supposed to).  But our caller doesn't have enough
   * info to intelligently place the cursor.
   */
  if (tn5250_display_cursor_x (This) > tn5250_field_end_col (field))
    {
      tn5250_dbuffer_left (This->display_buffers);
      tn5250_display_set_cursor_next_field (This);
    }

  free (text);
  return;
}


void
tn5250_display_wordwrap (Tn5250Display * This, unsigned char *text,
			 int totallen, int fieldlen, Tn5250Field * field)
{
  Tn5250Field *iter;
  int origcursorcol = tn5250_dbuffer_cursor_x (This->display_buffers);
  int nonnullcheck, nonnullcount = 0;
  int cursorset;
  int origentryid = field->entry_id;
  int i, j;
  unsigned char c, c2;
  int curlinelength = 0;
  int wordlength = 0;
  char word[3564 + 1] = { '\0' };
  char line[3564 + 1] = { '\0' };

  /* First find out how many non-null character are between the start
   * of the field and the current cursor position.  This will be used
   * later to set the cursor to the right place after wrapping.
   *
   * Note that the 'i < fieldlen + 1' in the for loop below is because
   * we add an extra character to each field before calling this function.
   */
  j = 0;
  for (iter = field; iter != tn5250_display_current_field (This);
       iter = iter->next)
    {
      for (i = 0; i < fieldlen + 1; i++)
	{
	  if (text[i + j] != TN5250_DISPLAY_WORD_WRAP_SPACE)
	    {
	      nonnullcount++;
	    }
	}
      j = i;
    }

  for (i = 0; i < origcursorcol - tn5250_field_start_col (iter); i++)
    {
      if (text[i + j] != TN5250_DISPLAY_WORD_WRAP_SPACE)
	{
	  nonnullcount++;
	}
    }

  /* After playing a bit with a real IBM terminal I discovered that it
   * treats every space as a word.  Thus everytime we encounter a space
   * complete the current word and add it and a space to the buffer.
   */

  /* We need to be able to distinguish between spaces the user typed in
   * and spaces we add here to pad the end of lines.  Spaces the user
   * typed in should always remain and are treated as one word per space.
   * Spaces we add here for padding should be adjustable.
   *
   * Right now the best way I can think of to do this is to use a special
   * character for spaces we pad with.  Since TN5250_DISPLAY_WORD_WRAP_SPACE
   * is not a valid character in EBCDIC or ASCII I'll use that here.
   */
  iter = field;
  for (i = 0; i < totallen; i++)
    {
      c = text[i];

      if (c != TN5250_DISPLAY_WORD_WRAP_SPACE)
	{
	  c2 = tn5250_char_map_to_local (This->map, c);
	}

      if ((c2 != ' ') && (c != TN5250_DISPLAY_WORD_WRAP_SPACE))
	{
	  word[wordlength] = c2;
	  wordlength++;
	  word[wordlength] = '\0';
	  curlinelength++;
	}
      else
	{
	  if (strlen (line) == 0)
	    {
	      if (c == TN5250_DISPLAY_WORD_WRAP_SPACE)
		{
		  sprintf (line, "%s", word);
		}
	      else
		{
		  sprintf (line, "%s ", word);
		}
	    }
	  else
	    {
	      if ((curlinelength + 1) > fieldlen)
		{
		  tn5250_dbuffer_cursor_set (This->display_buffers,
					     tn5250_field_start_row (iter),
					     tn5250_field_start_col (iter));
		  for (j = 0; j < strlen (line); j++)
		    {
		      tn5250_dbuffer_addch (This->display_buffers,
					    tn5250_char_map_to_remote (This->
								       map,
								       line
								       [j]));
		    }
		  for (; j < tn5250_field_length (iter); j++)
		    {
		      tn5250_dbuffer_addch (This->display_buffers,
					    TN5250_DISPLAY_WORD_WRAP_SPACE);
		    }
		  if (tn5250_field_is_wordwrap (iter))
		    {
		      iter = iter->next;
		    }
		  memset (line, '\0', 133);
		  if (c == TN5250_DISPLAY_WORD_WRAP_SPACE)
		    {
		      sprintf (line, "%s", word);
		    }
		  else
		    {
		      sprintf (line, "%s ", word);
		    }
		  curlinelength = strlen (line);
		}
	      else
		{
		  if (c == TN5250_DISPLAY_WORD_WRAP_SPACE)
		    {
		      sprintf (line, "%s%s", line, word);
		    }
		  else
		    {
		      sprintf (line, "%s%s ", line, word);
		    }
		  curlinelength = strlen (line);
		}
	    }
	  memset (word, '\0', 133);
	  wordlength = 0;
	}
    }

  /* Add or clear any trailing text */
  tn5250_dbuffer_cursor_set (This->display_buffers,
			     tn5250_field_start_row (iter),
			     tn5250_field_start_col (iter));
  if (strlen (word) > 0)
    {
      sprintf (line, "%s%s", line, word);
    }
  for (j = 0; j < strlen (line); j++)
    {
      tn5250_dbuffer_addch (This->display_buffers,
			    tn5250_char_map_to_remote (This->map, line[j]));
    }
  for (; j < tn5250_field_length (iter); j++)
    {
      tn5250_dbuffer_addch (This->display_buffers,
			    TN5250_DISPLAY_WORD_WRAP_SPACE);
    }

  /* And finally clear any old text that might be in fields after the last
   * field we just wrote to in this same word wrap group.
   *
   * It would probably be easier to use the field entry ID here instead of
   * checking to see if the field is continuous and not outside the current
   * word wrap group.
   */
  if (tn5250_field_is_continued_last (iter->next)
      || (tn5250_field_is_wordwrap (iter->next)
	  && !tn5250_field_is_continued_first (iter->next)))
    {
      for (iter = iter->next; tn5250_field_is_wordwrap (iter);
	   iter = iter->next)
	{
	  tn5250_dbuffer_cursor_set (This->display_buffers,
				     tn5250_field_start_row (iter),
				     tn5250_field_start_col (iter));
	  for (j = 0; j < tn5250_field_length (iter); j++)
	    {
	      tn5250_dbuffer_addch (This->display_buffers,
				    TN5250_DISPLAY_WORD_WRAP_SPACE);
	    }
	}
      if (tn5250_field_is_continued_last (iter))
	{
	  tn5250_dbuffer_cursor_set (This->display_buffers,
				     tn5250_field_start_row (iter),
				     tn5250_field_start_col (iter));
	  for (j = 0; j < tn5250_field_length (iter); j++)
	    {
	      tn5250_dbuffer_addch (This->display_buffers,
				    TN5250_DISPLAY_WORD_WRAP_SPACE);
	    }
	}
    }

  /* And finally put the cursor where it should be. */
  nonnullcheck = 0;
  cursorset = 0;
  for (iter = field; iter->entry_id == origentryid; iter = iter->next)
    {
      i = tn5250_field_start_row (iter);

      for (j = tn5250_field_start_col (iter);
	   j <= tn5250_field_end_col (iter); j++)
	{
	  if (j == This->display_buffers->w)
	    {
	      j = 0;
	      i++;
	    }
	  if (tn5250_display_char_at (This, i, j) !=
	      TN5250_DISPLAY_WORD_WRAP_SPACE)
	    {
	      if (nonnullcheck >= nonnullcount)
		{
		  tn5250_dbuffer_cursor_set (This->display_buffers, i, j);
		  cursorset = 1;
		  break;
		}
	      nonnullcheck++;
	    }
	}
      if (cursorset)
	{
	  break;
	}
    }

  return;
}

/****f* lib5250/display_check_pccmd
 * NAME
 *    display_check_pccmd
 * SYNOPSIS
 *    display_check_pccmd (This);
 * INPUTS
 *    Tn5250Display *      This       - 
 * DESCRIPTION
 *    Check the display buffer to see if it is a command sent via
 *    the STRPCCMD command.
 *****/
int display_check_pccmd(Tn5250Display *This) {

   int b,c;
   int header[5];
   int wait = 0;
   char cmdstr[124];

   if (This->allow_strpccmd == 0) 
     return 0;

   /* A command string will have an attribute of 0x27 (nondisplay) in
      col 0, row 0, and the letters 'PCO ' starting in col 3 of row 0. */

   if ( tn5250_display_char_at(This, 0, 0) != 0x27    /* non display */
     || tn5250_display_char_at(This, 0, 2) != 0xfc    /* ?? always 0xfc */
     || tn5250_display_char_at(This, 0, 3) != 0xd7    /* P */
     || tn5250_display_char_at(This, 0, 4) != 0xc3    /* C */
     || tn5250_display_char_at(This, 0, 5) != 0xd6    /* O */
     || tn5250_display_char_at(This, 0, 6) != 0x40) { /* space */
        return 0;
   }


   /* Col 1, Row 0 will contain 0x80 if there's a command to be run, or
            0x00 if the command has already been run.  This might be the
            length of the command (5 bytes header, 123 bytes command)? */

   c = tn5250_display_char_at(This, 0, 1);
   if (c == 0x00) {
        tn5250_display_do_aidkey (This, TN5250_SESSION_AID_ENTER);
        return 1;
   }
   else if (c != 0x80) {
        return 0;
   }


   /* The next 5 bytes are header bytes. If anyone has some docs on what
      they do, please let me know!!  -SK */

   header[0] = This->display_buffers->data[7];
   header[1] = This->display_buffers->data[8];
   header[2] = This->display_buffers->data[9];
   header[3] = This->display_buffers->data[10];
   header[4] = This->display_buffers->data[11];

   TN5250_LOG(("PCO Header Bytes: %x %x %x %x %x\n",
         header[0], header[1], header[2], header[3], header[4]));


   /* The last bit of the 5th header byte will be 0 if the emulator should
      wait for the command, or 1 otherwise. */

   if (header[4] & 0x01) 
        wait = 0;
   else
        wait = 1;


   /* The header bytes are followed by a 123 character fixed-length command
      string. */

   memcpy(cmdstr, This->display_buffers->data + 12, 123);
   cmdstr[123] = '\0';


   /* Strip any trailing blanks from the command string */

   for (b=0; b<123; b++) {
       cmdstr[b] = tn5250_char_map_to_local(
                      tn5250_display_char_map(This), cmdstr[b]);
   }

   b=122;
   while (b && cmdstr[b] == ' ') {
      cmdstr[b] = '\0';
      b--;
   }

   TN5250_LOG(("PCO Command: wait=%d, String: \"%s\"\n", wait,cmdstr));


   /* Run the command, then mark the display buffer with 0x00 so we know 
      that it has been run */

   tn5250_run_cmd(cmdstr, wait);
   This->display_buffers->data[1] = 0x00;


   /* Send back the ENTER key to tell the host that the command was run */

   tn5250_display_do_aidkey (This, TN5250_SESSION_AID_ENTER);
   return 1;
}
