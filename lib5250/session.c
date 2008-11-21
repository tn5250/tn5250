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


static void tn5250_session_send_error (Tn5250Session * This,
				       unsigned long errorcode);
static void tn5250_session_handle_receive (Tn5250Session * This);
static void tn5250_session_invite (Tn5250Session * This);
static void tn5250_session_cancel_invite (Tn5250Session * This);
static void tn5250_session_send_fields (Tn5250Session * This, int aidcode);
static void tn5250_session_send_field (Tn5250Session * This,
				       Tn5250Buffer * buf,
				       Tn5250Field * field);
static void tn5250_session_process_stream (Tn5250Session * This);
static void tn5250_session_write_error_code (Tn5250Session * This, int readop);
static void tn5250_session_write_to_display (Tn5250Session * This);
static void tn5250_session_clear_unit (Tn5250Session * This);
static void tn5250_session_clear_unit_alternate (Tn5250Session * This);
static void tn5250_session_clear_format_table (Tn5250Session * This);
static void tn5250_session_read_immediate (Tn5250Session * This);
/*static void tn5250_session_home (Tn5250Session * This);*/
/*static void tn5250_session_print (Tn5250Session * This);*/
static void tn5250_session_output_only (Tn5250Session * This);
static void tn5250_session_save_screen (Tn5250Session * This);
static void tn5250_session_save_partial_screen (Tn5250Session * This);
static void tn5250_session_roll (Tn5250Session * This);
static void tn5250_session_start_of_field (Tn5250Session * This);
static void tn5250_session_start_of_header (Tn5250Session * This);
static void tn5250_session_set_buffer_address (Tn5250Session * This);
static void tn5250_session_write_extended_attribute (Tn5250Session * This);
static void tn5250_session_write_structured_field (Tn5250Session * This);
static void tn5250_session_write_display_structured_field (Tn5250Session *
							   This);
static void tn5250_session_transparent_data (Tn5250Session * This);
static void tn5250_session_move_cursor (Tn5250Session * This);
static void tn5250_session_insert_cursor (Tn5250Session * This);
static void tn5250_session_erase_to_address (Tn5250Session * This);
static void tn5250_session_repeat_to_address (Tn5250Session * This);
static void tn5250_session_read_screen_immediate (Tn5250Session * This);
static void tn5250_session_read_cmd (Tn5250Session * This, int readop);
/*static int tn5250_session_valid_wtd_data_char (unsigned char c);*/
static int tn5250_session_handle_aidkey (Tn5250Session * This, int key);
static void tn5250_session_handle_cc1 (Tn5250Session * This,
				       unsigned char cc1);
static void tn5250_session_handle_cc2 (Tn5250Session * This,
				       unsigned char cc2);
static void tn5250_session_query_reply (Tn5250Session * This);
static void tn5250_session_define_selection_field (Tn5250Session * This,
						   int length);
static void tn5250_session_remove_gui_selection_field (Tn5250Session * This,
						       int length);
static void tn5250_session_define_selection_item (Tn5250Session * This,
						  Tn5250Menubar * menubar,
						  int length, int count,
						  short createnew);
static void tn5250_session_create_window_structured_field (Tn5250Session *
							   This, int length);
static void tn5250_session_define_scrollbar (Tn5250Session * This,
					     int length);
static void
tn5250_session_remove_gui_window_structured_field (Tn5250Session * This,
						   int length);
static void
tn5250_session_remove_all_gui_constructs_structured_field (Tn5250Session *
							   This, int length);
static void tn5250_session_write_data_structured_field (Tn5250Session *
							This, int length);


/****f* lib5250/tn5250_session_new
 * NAME
 *    tn5250_session_new
 * SYNOPSIS
 *    ret = tn5250_session_new ();
 * INPUTS
 *    None
 * DESCRIPTION
 *    DOCUMENT ME!!!
 *****/
Tn5250Session *
tn5250_session_new ()
{
  Tn5250Session *This;

  This = tn5250_new (Tn5250Session, 1);
  if (This == NULL)
    {
      return NULL;
    }

  This->record = tn5250_record_new ();
  if (This->record == NULL)
    {
      free (This);
      return NULL;
    }

  This->config = NULL;
  This->stream = NULL;
  This->invited = 1;
  This->read_opcode = 0;

  This->handle_aidkey = tn5250_session_handle_aidkey;
  This->display = NULL;
  return This;
}


/****f* lib5250/tn5250_session_destroy
 * NAME
 *    tn5250_session_destroy
 * SYNOPSIS
 *    tn5250_session_destroy (This);
 * INPUTS
 *    Tn5250Session *      This       - 
 * DESCRIPTION
 *    DOCUMENT ME!!!
 *****/
void
tn5250_session_destroy (Tn5250Session * This)
{
  if (This->stream != NULL)
    {
      tn5250_stream_destroy (This->stream);
      This->stream = NULL;
    }
  if (This->record != NULL)
    {
      tn5250_record_destroy (This->record);
      This->record = NULL;
    }
  if (This->config != NULL)
    {
      tn5250_config_unref (This->config);
      This->config = NULL;
    }
  free (This);
  return;
}


/****f* lib5250/tn5250_session_config
 * NAME
 *    tn5250_session_config
 * SYNOPSIS
 *    tn5250_session_config (This);
 * INPUTS
 *    Tn5250Session *      This       - The session to configure.
 *    Tn5250Config *       config     - The configuration object to use.
 * DESCRIPTION
 *    DOCUMENT ME!!!
 *****/
int
tn5250_session_config (Tn5250Session * This, Tn5250Config * config)
{
  tn5250_config_ref (config);
  if (This->config != NULL)
    {
      tn5250_config_unref (This->config);
    }
  This->config = config;
  /* FIXME: Validate */
  return 0;
}


/****f* lib5250/tn5250_session_set_stream
 * NAME
 *    tn5250_session_set_stream
 * SYNOPSIS
 *    tn5250_session_set_stream (This, newstream);
 * INPUTS
 *    Tn5250Session *      This       - 
 *    Tn5250Stream *       newstream  - 
 * DESCRIPTION
 *    DOCUMENT ME!!!
 *****/
void
tn5250_session_set_stream (Tn5250Session * This, Tn5250Stream * newstream)
{
  if ((This->stream = newstream) != NULL)
    {
      tn5250_display_update (This->display);
    }
  return;
}


/****f* lib5250/tn5250_session_main_loop
 * NAME
 *    tn5250_session_main_loop
 * SYNOPSIS
 *    tn5250_session_main_loop (This);
 * INPUTS
 *    Tn5250Session *      This       - 
 * DESCRIPTION
 *    DOCUMENT ME!!!
 *****/
void
tn5250_session_main_loop (Tn5250Session * This)
{
  int r;

  while (1)
    {
      r = tn5250_display_waitevent (This->display);
      if ((r & TN5250_TERMINAL_EVENT_QUIT) != 0)
	{
	  return;
	}
      if ((r & TN5250_TERMINAL_EVENT_DATA) != 0)
	{
	  if (!tn5250_stream_handle_receive (This->stream))
	    {
	      return;
	    }
	  tn5250_session_handle_receive (This);
	}
    }
  return;
}


void
tn5250_session_send_error (Tn5250Session * This, unsigned long errorcode)
{
  StreamHeader header;

  errorcode = htonl (errorcode);

  TN5250_LOG (("Sending negative response = %x", errorcode));
  header.h5250.flowtype = TN5250_RECORD_FLOW_DISPLAY;
  header.h5250.flags = TN5250_RECORD_H_ERR;
  header.h5250.opcode = TN5250_RECORD_OPCODE_NO_OP;

  tn5250_stream_send_packet (This->stream, 4, header,
			     (unsigned char *) &errorcode);

  tn5250_record_skip_to_end (This->record);
  return;
}


/****i* lib5250/tn5250_session_handle_receive
 * NAME
 *    tn5250_session_handle_receive
 * SYNOPSIS
 *    tn5250_session_handle_receive (This);
 * INPUTS
 *    Tn5250Session *      This       - 
 * DESCRIPTION
 *    Tell the socket to receive as much data as possible, then, if there are
 *    full packets to handle, handle them.
 *****/
static void
tn5250_session_handle_receive (Tn5250Session * This)
{
  int atn;
  int cur_opcode;

  TN5250_LOG (("HandleReceive: entered.\n"));
  while (tn5250_stream_record_count (This->stream) > 0)
    {
      if (This->record != NULL)
	{
	  tn5250_record_destroy (This->record);
	}
      This->record = tn5250_stream_get_record (This->stream);
      cur_opcode = tn5250_record_opcode (This->record);
      atn = tn5250_record_attention (This->record);

      TN5250_LOG (("HandleReceive: cur_opcode = 0x%02X %d\n", cur_opcode,
		   atn));

      switch (cur_opcode)
	{
	case TN5250_RECORD_OPCODE_PUT_GET:
	case TN5250_RECORD_OPCODE_INVITE:
	  tn5250_session_invite (This);
	  break;

	case TN5250_RECORD_OPCODE_OUTPUT_ONLY:
	  tn5250_session_output_only (This);
	  break;

	case TN5250_RECORD_OPCODE_CANCEL_INVITE:
	  tn5250_session_cancel_invite (This);
	  break;

	case TN5250_RECORD_OPCODE_MESSAGE_ON:
	  tn5250_display_indicator_set (This->display,
					TN5250_DISPLAY_IND_MESSAGE_WAITING);
	  tn5250_display_beep (This->display);
	  break;

	case TN5250_RECORD_OPCODE_MESSAGE_OFF:
	  tn5250_display_indicator_clear (This->display,
					  TN5250_DISPLAY_IND_MESSAGE_WAITING);
	  break;

	case TN5250_RECORD_OPCODE_NO_OP:
	case TN5250_RECORD_OPCODE_SAVE_SCR:
	case TN5250_RECORD_OPCODE_RESTORE_SCR:
	case TN5250_RECORD_OPCODE_READ_IMMED:
	case TN5250_RECORD_OPCODE_READ_SCR:
	  break;

	default:
	  TN5250_LOG (("Error: unknown opcode %2.2X\n", cur_opcode));
	  TN5250_ASSERT (0);
	}

      if (!tn5250_record_is_chain_end (This->record))
	tn5250_session_process_stream (This);
    }
  tn5250_display_update (This->display);
  return;
}


/****i* lib5250/tn5250_session_invite
 * NAME
 *    tn5250_session_invite
 * SYNOPSIS
 *    tn5250_session_invite (This);
 * INPUTS
 *    Tn5250Session *      This       - 
 * DESCRIPTION
 *    DOCUMENT ME!!!
 *****/
static void
tn5250_session_invite (Tn5250Session * This)
{
  TN5250_LOG (("Invite: entered.\n"));
  This->invited = 1;
  tn5250_display_indicator_clear (This->display, TN5250_DISPLAY_IND_X_CLOCK);
  return;
}


/****i* lib5250/tn5250_session_cancel_invite
 * NAME
 *    tn5250_session_cancel_invite
 * SYNOPSIS
 *    tn5250_session_cancel_invite (This);
 * INPUTS
 *    Tn5250Session *      This       - 
 * DESCRIPTION
 *    DOCUMENT ME!!!
 *****/
static void
tn5250_session_cancel_invite (Tn5250Session * This)
{
  StreamHeader header;

  header.h5250.flowtype = TN5250_RECORD_FLOW_DISPLAY;
  header.h5250.flags = TN5250_RECORD_H_NONE;
  header.h5250.opcode = TN5250_RECORD_OPCODE_CANCEL_INVITE;

  TN5250_LOG (("CancelInvite: entered.\n"));
  tn5250_display_indicator_set (This->display, TN5250_DISPLAY_IND_X_CLOCK);
  tn5250_stream_send_packet (This->stream, 0, header, NULL);
  This->invited = 0;
  return;
}


/****i* lib5250/tn5250_session_send_fields
 * NAME
 *    tn5250_session_send_fields
 * SYNOPSIS
 *    tn5250_session_send_fields (This, aidcode);
 * INPUTS
 *    Tn5250Session *      This       - 
 *    int                  aidcode    - 
 * DESCRIPTION
 *    DOCUMENT ME!!!
 *****/
static void
tn5250_session_send_fields (Tn5250Session * This, int aidcode)
{
  Tn5250Buffer field_buf;
  Tn5250Field *field;
  Tn5250DBuffer *dbuffer;
  int X, Y;
  StreamHeader header;


  X = tn5250_display_cursor_x (This->display);
  Y = tn5250_display_cursor_y (This->display);

  dbuffer = tn5250_display_dbuffer (This->display);
  TN5250_ASSERT (dbuffer != NULL);
  TN5250_LOG (("SendFields: Number of fields: %d\n",
	       tn5250_dbuffer_field_count (dbuffer)));

  tn5250_buffer_init (&field_buf);
  tn5250_buffer_append_byte (&field_buf, Y + 1);
  tn5250_buffer_append_byte (&field_buf, X + 1);

  /* We can have an aidcode of 0 if we are doing a Read Immediate */
  tn5250_buffer_append_byte (&field_buf, aidcode);

  TN5250_LOG (("SendFields: row = %d; col = %d; aid = 0x%02x\n", Y, X,
	       aidcode));

  /* FIXME: Implement field resequencing. */
  switch (This->read_opcode)
    {
    case CMD_READ_INPUT_FIELDS:
      TN5250_ASSERT (aidcode != 0);
      if (tn5250_dbuffer_mdt (dbuffer)
	  && tn5250_dbuffer_send_data_for_aid_key (dbuffer, aidcode))
	{
	  field = dbuffer->field_list;
	  if (field != NULL)
	    {
	      do
		{
		  tn5250_session_send_field (This, &field_buf, field);
		  field = field->next;
		}
	      while (field != dbuffer->field_list);
	    }
	}
      break;

    case CMD_READ_IMMEDIATE:
      if (tn5250_dbuffer_mdt (dbuffer))
	{
	  field = dbuffer->field_list;
	  if (field != NULL)
	    {
	      do
		{
		  tn5250_session_send_field (This, &field_buf, field);
		  field = field->next;
		}
	      while (field != dbuffer->field_list);
	    }
	}
      break;

    case CMD_READ_MDT_FIELDS:
    case CMD_READ_MDT_FIELDS_ALT:
      TN5250_ASSERT (aidcode != 0);

    case CMD_READ_IMMEDIATE_ALT:
      if (tn5250_dbuffer_send_data_for_aid_key (dbuffer, aidcode))
	{
	  field = dbuffer->field_list;
	  if (field != NULL)
	    {
	      do
		{
		  if (tn5250_field_mdt (field))
		    {
		      tn5250_session_send_field (This, &field_buf, field);
		    }
		  field = field->next;
		}
	      while (field != dbuffer->field_list);
	    }
	}
      break;

    default:			/* Huh? */
      TN5250_LOG (("BUG!!! Trying to transmit fields when This->read_opcode = 0x%02X.\n", This->read_opcode));
      TN5250_ASSERT (0);
    }

  This->read_opcode = 0;	/* No longer in a read command. */
  tn5250_display_indicator_set (This->display, TN5250_DISPLAY_IND_X_SYSTEM);
  This->display->keystate = TN5250_KEYSTATE_LOCKED;
  tn5250_display_indicator_clear (This->display, TN5250_DISPLAY_IND_INSERT);
  tn5250_display_update (This->display);

  header.h5250.flowtype = TN5250_RECORD_FLOW_DISPLAY;
  header.h5250.flags = TN5250_RECORD_H_NONE;
  header.h5250.opcode = TN5250_RECORD_OPCODE_PUT_GET;

  tn5250_stream_send_packet (This->stream,
			     tn5250_buffer_length (&field_buf),
			     header, tn5250_buffer_data (&field_buf));
  tn5250_buffer_free (&field_buf);
  return;
}


/****i* lib5250/tn5250_session_send_field
 * NAME
 *    tn5250_session_send_field
 * SYNOPSIS
 *    tn5250_session_send_field (This, buf, field);
 * INPUTS
 *    Tn5250Session *      This       - 
 *    Tn5250Buffer *       buf        - 
 *    Tn5250Field *        field      - 
 * DESCRIPTION
 *    Append a single field to the transmit buffer in the manner required
 *    by the current read opcode.
 *****/
static void
tn5250_session_send_field (Tn5250Session * This, Tn5250Buffer * buf,
			   Tn5250Field * field)
{
  int size, n;
  unsigned char *data;
  unsigned char c;
  Tn5250Field *iter;

  size = tn5250_field_length (field);
  data = tn5250_display_field_data (This->display, field);

  TN5250_LOG (("Sending:\n"));
  tn5250_field_dump (field);

  TN5250_ASSERT (!tn5250_field_is_continued_middle (field) &&
		 !tn5250_field_is_continued_last (field));


  /* find the following fields with the continuous flag set
   * until we got them all.
   *
   * Assumes for now that all the continued field are one after the
   * other and not distributed among other fields.
   *
   * Which appears to be a perfectly valid assumption since the "Functions
   * Reference" manual says they have to be.
   */
  if (field->continuous)
    {
      /* We also must only send back data for the first subfield of a
       * continuous field.  All subfields are treated as one and are sent
       * as part of the first subfield.
       */
      if (tn5250_field_is_continued_first (field))
	{
	  /* The trick is to construct a temporary data which hold the entire
	   * reconstructed field
	   */
	  int i = 0;

	  /* 1st loop: Guess the full size */
	  for (iter = field->next; iter->continuous; iter = iter->next)
	    {
	      size += tn5250_field_length (iter);
	      if (tn5250_field_is_continued_last (iter))
		{
		  break;
		}
	    }

	  data = malloc (size);
	  /* 2nd loop: Copy the data in the temporary buffer */
	  for (iter = field; iter->continuous; iter = iter->next)
	    {
	      memcpy (data + i,
		      tn5250_display_field_data (This->display, iter),
		      tn5250_field_length (iter));
	      i += tn5250_field_length (iter);
	      if (tn5250_field_is_continued_last (iter))
		{
		  break;
		}
	    }
	}
      else
	{
	  return;
	}
    }

  switch (This->read_opcode)
    {
    case CMD_READ_INPUT_FIELDS:
    case CMD_READ_IMMEDIATE:
      if (tn5250_field_is_signed_num (field))
	{
	  for (n = 0; n < size - 1; n++)
	    {
	      tn5250_buffer_append_byte (buf, data[n] == 0 ? 0x40 : data[n]);
	    }
	  c = data[size - 2];
	  tn5250_buffer_append_byte (buf,
				     tn5250_char_map_to_local
				     (tn5250_display_char_map (This->display),
				      data[size - 1]) ==
				     '-' ? (0xd0 | (0x0f & c)) : c);
	}
      else
	{
	  for (n = 0; n < size; n++)
	    {
	      tn5250_buffer_append_byte (buf, data[n] == 0 ? 0x40 : data[n]);
	    }
	}
      break;

    case CMD_READ_MDT_FIELDS:
    case CMD_READ_MDT_FIELDS_ALT:
    case CMD_READ_IMMEDIATE_ALT:
      tn5250_buffer_append_byte (buf, SBA);
      tn5250_buffer_append_byte (buf, tn5250_field_start_row (field) + 1);
      tn5250_buffer_append_byte (buf, tn5250_field_start_col (field) + 1);

      /* For signed numeric fields, if the second-last character is a digit
       * and the last character is a '-', zone shift the second-last char.
       * In any case, don't send the sign position. */
      c = data[size - 1];
      if (tn5250_field_is_signed_num (field))
	{
	  size--;
	  c = size > 0 ? data[size - 1] : 0;
	  if (size > 1
	      && data[size] ==
	      tn5250_char_map_to_remote (tn5250_display_char_map
					 (This->display), '-')
	      &&
	      isdigit (tn5250_char_map_to_local
		       (tn5250_display_char_map (This->display), c)))
	    {
	      c = (0xd0 | (0x0f & c));
	    }
	}

      /* Strip trailing NULs */
      while (size > 0 && data[size - 1] == 0)
	{
	  size--;
	  c = size > 0 ? data[size - 1] : 0;
	}

      /* Send all but the last character, then send the last character.
       * This is because we don't want to modify the display buffer's data
       *
       * For Read MDT Fields, we translate leading and embedded NULs to
       * blanks, for the 'Alternate' commands, we do not.
       */
      for (n = 0; n < size - 1; n++)
	{
	  if (This->read_opcode != CMD_READ_MDT_FIELDS)
	    {
	      tn5250_buffer_append_byte (buf, data[n]);
	    }
	  else
	    {
	      tn5250_buffer_append_byte (buf, data[n] == 0 ? 0x40 : data[n]);
	    }
	}
      if (size > 0)
	{
	  if (This->read_opcode != CMD_READ_MDT_FIELDS)
	    {
	      tn5250_buffer_append_byte (buf, c);
	    }
	  else
	    {
	      tn5250_buffer_append_byte (buf, c == 0 ? 0x40 : c);
	    }
	}
      break;
    }

  if (field->continuous)
    {
      free (data);
    }
  return;
}


/****i* lib5250/tn5250_session_process_stream
 * NAME
 *    tn5250_session_process_stream
 * SYNOPSIS
 *    tn5250_session_process_stream (This);
 * INPUTS
 *    Tn5250Session *      This       - 
 * DESCRIPTION
 *    DOCUMENT ME!!!
 *****/
static void
tn5250_session_process_stream (Tn5250Session * This)
{
  int cur_command;
  unsigned long errorcode;

  TN5250_LOG (("ProcessStream: entered.\n"));
  while (!tn5250_record_is_chain_end (This->record))
    {
      cur_command = tn5250_record_get_byte (This->record);
      if (cur_command != ESC)
	{
	  TN5250_LOG (("cur_command != ESC; cur_pos = %d\n",
		       This->record->cur_pos));
	  TN5250_LOG (("Ignoring record!\n"));
	  return;
	  TN5250_ASSERT (0);
	}
      cur_command = tn5250_record_get_byte (This->record);
      TN5250_LOG (("ProcessStream: cur_command = 0x%02X\n", cur_command));

      switch (cur_command)
	{
	case CMD_CLEAR_UNIT:
	  tn5250_session_clear_unit (This);
	  break;
	case CMD_CLEAR_UNIT_ALTERNATE:
	  tn5250_session_clear_unit_alternate (This);
	  break;
	case CMD_CLEAR_FORMAT_TABLE:
	  tn5250_session_clear_format_table (This);
	  break;
	case CMD_WRITE_TO_DISPLAY:
	  tn5250_session_write_to_display (This);
	  break;
	case CMD_WRITE_ERROR_CODE:
	case CMD_WRITE_ERROR_CODE_WINDOW:
	  tn5250_session_write_error_code (This, cur_command);
	  break;
	case CMD_READ_INPUT_FIELDS:
	case CMD_READ_MDT_FIELDS:
	case CMD_READ_MDT_FIELDS_ALT:
	  tn5250_session_read_cmd (This, cur_command);
	  break;
	case CMD_READ_SCREEN_IMMEDIATE:
	  tn5250_session_read_screen_immediate (This);
	  break;
	case CMD_READ_SCREEN_EXTENDED:
	  /*tn5250_session_read_screen_extended (This);*/
	  TN5250_LOG (("ReadScreenExtended (ignored)\n"));
	  break;
	case CMD_READ_SCREEN_PRINT:
	  /*tn5250_session_read_screen_print (This);*/
	  TN5250_LOG (("ReadScreenPrint (ignored)\n"));
	  break;
	case CMD_READ_SCREEN_PRINT_EXTENDED:
	  /*tn5250_session_read_screen_print_extended (This);*/
	  TN5250_LOG (("ReadScreenPrintExtended (ignored)\n"));
	  break;
	case CMD_READ_SCREEN_PRINT_GRID:
	  /*tn5250_session_read_screen_print_grid (This);*/
	  TN5250_LOG (("ReadScreenPrintGrid (ignored)\n"));
	  break;
	case CMD_READ_SCREEN_PRINT_EXT_GRID:
	  /*tn5250_session_read_screen_print_extended_grid (This);*/
	  TN5250_LOG (("ReadScreenPrintExtendedGrid (ignored)\n"));
	  break;
	case CMD_READ_IMMEDIATE:
	  tn5250_session_read_immediate (This);
	  break;
	case CMD_READ_IMMEDIATE_ALT:
	  /*tn5250_session_read_immediate_alt (This);*/
	  TN5250_LOG (("ReadImmediateAlt (ignored)\n"));
	  break;
	case CMD_SAVE_SCREEN:
	  tn5250_session_save_screen (This);
	  break;
	case CMD_SAVE_PARTIAL_SCREEN:
	  tn5250_session_save_partial_screen (This);
	  break;
	case CMD_RESTORE_SCREEN:
	  /* Ignored, the data following this should be a valid
	   * Write To Display command. */
	  TN5250_LOG (("RestoreScreen (ignored)\n"));
	  break;
	case CMD_RESTORE_PARTIAL_SCREEN:
	  /* Ignored, the data following this should be a valid
	   * Write To Display command because we do basically the
	   * the same thing for SAVE PARTIAL SCREEN as we do for
	   * SAVE SCREEN.
	   */
	  TN5250_LOG (("RestorePartialScreen (ignored)\n"));
	  break;
	case CMD_ROLL:
	  tn5250_session_roll (This);
	  break;
	case CMD_WRITE_STRUCTURED_FIELD:
	  tn5250_session_write_structured_field (This);
	  break;
	case 0x0a:
	  TN5250_LOG (("Ignoring record!\n"));
	  break;
	default:
	  TN5250_LOG (("Error: Unknown command 0x%02X.\n", cur_command));

	  errorcode = TN5250_NR_INVALID_COMMAND;

	  tn5250_session_send_error (This, errorcode);
	}
    }
  return;
}


/****i* lib5250/tn5250_session_write_error_code
 * NAME
 *    tn5250_session_write_error_code
 * SYNOPSIS
 *    tn5250_session_write_error_code (This, readop);
 * INPUTS
 *    Tn5250Session *      This       - 
 * DESCRIPTION
 *    DOCUMENT ME!!!
 *****/
static void
tn5250_session_write_error_code (Tn5250Session * This, int readop)
{
  unsigned char startwin, endwin;
  unsigned char c;
  int end_x, end_y;
  int have_ic = 0;
  unsigned char *tempmsg;
  int msglen;

  TN5250_LOG (("WriteErrorCode: entered.\n"));

  if (readop == CMD_WRITE_ERROR_CODE_WINDOW)
    {
      startwin = tn5250_record_get_byte (This->record);
      endwin = tn5250_record_get_byte (This->record);
    }

  end_x = tn5250_display_cursor_x (This->display);
  end_y = tn5250_display_cursor_y (This->display);

  /* Message line is restored next time the X II indicator is
   * cleared. */
  tn5250_display_save_msg_line (This->display);
  tn5250_display_set_cursor (This->display,
			     tn5250_display_msg_line (This->display), 0);

  tempmsg = malloc (tn5250_display_width (This->display));
  msglen = 0;

  while (1)
    {
      if (tn5250_record_is_chain_end (This->record))
	{
	  break;
	}
      c = tn5250_record_get_byte (This->record);
      if (c == ESC)
	{
	  tn5250_record_unget_byte (This->record);
	  break;
	}

      /* In this context, IC does NOT change the Insert Cursor position,
       * it just moves the cursor.  This is described as the case for the
       * Write Error Code command. */
      if (c == IC)
	{
	  have_ic = 1;
	  end_y = tn5250_record_get_byte (This->record) - 1;
	  end_x = tn5250_record_get_byte (This->record) - 1;
	  continue;
	}

      if (tn5250_char_map_printable_p
	  (tn5250_display_char_map (This->display), c))
	{
	  tempmsg[msglen] = c;
	  msglen++;
	  continue;
	}

      TN5250_LOG (("Error: Unknown order -- %2.2X --\n", c));
      TN5250_ASSERT (0);
    }

  tn5250_display_set_msg_line (This->display, tempmsg, msglen);
  free (tempmsg);

  tn5250_display_set_cursor (This->display, end_y, end_x);

  This->display->keystate = TN5250_KEYSTATE_POSTHELP;
  tn5250_display_inhibit (This->display);
  return;
}


/****i* lib5250/tn5250_session_handle_cc1
 * NAME
 *    tn5250_session_handle_cc1
 * SYNOPSIS
 *    tn5250_session_handle_cc1 (This, cc1);
 * INPUTS
 *    Tn5250Session *      This       - 
 *    unsigned char        cc1        - 
 * DESCRIPTION
 *    Process the first byte of the CC from the WTD command.
 *****/
static void
tn5250_session_handle_cc1 (Tn5250Session * This, unsigned char cc1)
{
  int lock_kb = 1;
  int reset_non_bypass_mdt = 0;
  int reset_all_mdt = 0;
  int null_non_bypass_mdt = 0;
  int null_non_bypass = 0;
  Tn5250Field *iter;

  switch (cc1 & 0xE0)
    {
    case 0x00:
      lock_kb = 0;
      break;

    case 0x40:
      reset_non_bypass_mdt = 1;
      break;

    case 0x60:
      reset_all_mdt = 1;
      break;

    case 0x80:
      null_non_bypass_mdt = 1;
      break;

    case 0xA0:
      reset_non_bypass_mdt = 1;
      null_non_bypass = 1;
      break;

    case 0xC0:
      reset_non_bypass_mdt = 1;
      null_non_bypass_mdt = 1;
      break;

    case 0xE0:
      reset_all_mdt = 1;
      null_non_bypass = 1;
      break;
    }

  if (lock_kb)
    {
      TN5250_LOG (("tn5250_session_handle_cc1: Locking keyboard.\n"));
      tn5250_display_indicator_set (This->display,
				    TN5250_DISPLAY_IND_X_SYSTEM);
      This->display->keystate = TN5250_KEYSTATE_LOCKED;
    }
  TN5250_ASSERT (This->display != NULL
		 && tn5250_display_dbuffer (This->display) != NULL);
  if ((iter = tn5250_display_dbuffer (This->display)->field_list) != NULL)
    {
      do
	{
	  if (!tn5250_field_is_bypass (iter))
	    {
	      if ((null_non_bypass_mdt && tn5250_field_mdt (iter))
		  || null_non_bypass)
		{
		  unsigned char *data;
		  data = tn5250_display_field_data (This->display, iter);
		  memset (data, 0, tn5250_field_length (iter));
		}
	    }
	  if (reset_all_mdt
	      || (reset_non_bypass_mdt && !tn5250_field_is_bypass (iter)))
	    {
	      tn5250_field_clear_mdt (iter);
	    }
	  iter = iter->next;
	}
      while (iter != This->display->display_buffers->field_list);
    }
  return;
}


/****i* lib5250/tn5250_session_write_to_display
 * NAME
 *    tn5250_session_write_to_display
 * SYNOPSIS
 *    tn5250_session_write_to_display (This);
 * INPUTS
 *    Tn5250Session *      This       - 
 * DESCRIPTION
 *    DOCUMENT ME!!!
 *****/
static void
tn5250_session_write_to_display (Tn5250Session * This)
{
  unsigned char cur_order;
  unsigned char CC1;
  unsigned char CC2;
  int done = 0;
  unsigned char end_x = 0xff, end_y = 0xff;
  int old_x = tn5250_display_cursor_x (This->display);
  int old_y = tn5250_display_cursor_y (This->display);
  int is_x_system;
  int will_be_unlocked;
  int cur_opcode;

  TN5250_LOG (("WriteToDisplay: entered.\n"));

  CC1 = tn5250_record_get_byte (This->record);
  CC2 = tn5250_record_get_byte (This->record);
  TN5250_LOG (("WriteToDisplay: 0x%02X:0x%02X\n", CC1, CC2));

  tn5250_session_handle_cc1 (This, CC1);

  while (!done)
    {
      if (tn5250_record_is_chain_end (This->record))
	{
	  done = 1;
	}
      else
	{
	  cur_order = tn5250_record_get_byte (This->record);
#ifndef NDEBUG
	  if (cur_order > 0 && cur_order < 0x40)
	    TN5250_LOG (("\n"));
#endif
	  switch (cur_order)
	    {

	    case WEA:
	      tn5250_session_write_extended_attribute (This);
	      break;

	    case TD:
	      tn5250_session_transparent_data (This);
	      break;

	    case WDSF:
	      tn5250_session_write_display_structured_field (This);
	      break;

	    case MC:
	      tn5250_session_move_cursor (This);
	      break;

	    case IC:
	      tn5250_session_insert_cursor (This);
	      break;

	    case EA:
	      tn5250_session_erase_to_address (This);
	      break;

	    case RA:
	      tn5250_session_repeat_to_address (This);
	      break;

	    case SBA:
	      tn5250_session_set_buffer_address (This);
	      break;

	    case SF:
	      tn5250_session_start_of_field (This);
	      break;

	    case SOH:
	      tn5250_session_start_of_header (This);
	      break;

	    case ESC:
	      done = 1;
	      tn5250_record_unget_byte (This->record);
	      break;

	    default:
	      if (tn5250_char_map_printable_p
		  (tn5250_display_char_map (This->display), cur_order))
		{
		  tn5250_display_addch (This->display, cur_order);
#ifndef NDEBUG
		  if (tn5250_char_map_attribute_p
		      (tn5250_display_char_map (This->display), cur_order))
		    {
		      TN5250_LOG (("(0x%02X) ", cur_order));
		    }
		  else
		    {
		      TN5250_LOG (("%c (0x%02X) ",
				   tn5250_char_map_to_local
				   (tn5250_display_char_map (This->display),
				    cur_order), cur_order));
		    }
#endif
		}
	      else
		{
		  TN5250_LOG (("Error: Unknown order -- %2.2X --\n",
			       cur_order));
		  TN5250_ASSERT (0);
		}
	    }			/* end switch */
	}			/* end else */
    }				/* end while */
  TN5250_LOG (("\n"));

  /* If we've gotten an MC or IC order, set the cursor to that position.
   * Otherwise set the cursor to the home position (which could be the IC
   * position from a prior IC if the unit hasn't been cleared since then,
   * but is probably the first position of the first non-bypass field). */

  is_x_system = (This->display->keystate != TN5250_KEYSTATE_UNLOCKED);
  will_be_unlocked = ((CC2 & TN5250_SESSION_CTL_UNLOCK) != 0);
  cur_opcode = tn5250_record_opcode (This->record);
  if (end_y != 0xff && end_x != 0xff && !(CC2 & TN5250_SESSION_CTL_IC_ULOCK))
    {
      tn5250_display_set_cursor (This->display, end_y, end_x);
    }
  else if ((will_be_unlocked && !(CC2 & TN5250_SESSION_CTL_IC_ULOCK)) ||
	   cur_opcode == TN5250_RECORD_OPCODE_RESTORE_SCR)
    {
      tn5250_display_set_cursor_home (This->display);
    }
  else
    {
      tn5250_display_set_cursor (This->display, old_y, old_x);
    }

  tn5250_session_handle_cc2 (This, CC2);
  return;
}


/****i* lib5250/tn5250_session_handle_cc2
 * NAME
 *    tn5250_session_handle_cc2
 * SYNOPSIS
 *    tn5250_session_handle_cc2 (This, CC2);
 * INPUTS
 *    Tn5250Session *      This       - 
 *    unsigned char        CC2        - 
 * DESCRIPTION
 *    DOCUMENT ME!!!
 *****/
static void
tn5250_session_handle_cc2 (Tn5250Session * This, unsigned char CC2)
{
  TN5250_LOG (("Processing CC2 0x%02X.\n", (int) CC2));
  if (CC2 & TN5250_SESSION_CTL_MESSAGE_ON)
    {
      tn5250_display_indicator_set (This->display,
				    TN5250_DISPLAY_IND_MESSAGE_WAITING);
    }
  if ((CC2 & TN5250_SESSION_CTL_MESSAGE_OFF)
      && !(CC2 & TN5250_SESSION_CTL_MESSAGE_ON))
    {
      tn5250_display_indicator_clear (This->display,
				      TN5250_DISPLAY_IND_MESSAGE_WAITING);
    }

  if ((CC2 & TN5250_SESSION_CTL_CLR_BLINK) != 0
      && (CC2 & TN5250_SESSION_CTL_SET_BLINK) == 0)
    {
      /* FIXME: Hand off to terminal */
    }
  if ((CC2 & TN5250_SESSION_CTL_SET_BLINK) != 0)
    {
      /* FIXME: Hand off to terminal */
    }
  if ((CC2 & TN5250_SESSION_CTL_ALARM) != 0)
    {
      TN5250_LOG (("TN5250_SESSION_CTL_ALARM was set.\n"));
      tn5250_display_beep (This->display);
    }
  if ((CC2 & TN5250_SESSION_CTL_UNLOCK) != 0)
    {
      tn5250_display_indicator_clear (This->display,
				      TN5250_DISPLAY_IND_X_SYSTEM);
      if (This->display->keystate == TN5250_KEYSTATE_LOCKED)
	{
	  This->display->keystate = TN5250_KEYSTATE_UNLOCKED;
	}
    }

  TN5250_LOG (("Done Processing CC2.\n"));
  return;
}


/****i* lib5250/tn5250_session_clear_unit
 * NAME
 *    tn5250_session_clear_unit
 * SYNOPSIS
 *    tn5250_session_clear_unit (This);
 * INPUTS
 *    Tn5250Session *      This       - 
 * DESCRIPTION
 *    DOCUMENT ME!!!
 *****/
static void
tn5250_session_clear_unit (Tn5250Session * This)
{
  TN5250_LOG (("ClearUnit: entered.\n"));

  if (This->display->display_buffers->window_count > 0)
    {
      Tn5250Window *iter, *next;

      if ((iter = This->display->display_buffers->window_list) != NULL)
	{
	  do
	    {
	      next = iter->next;
	      TN5250_LOG (("destroying window id: %d\n", iter->id));
	      tn5250_terminal_destroy_window (This->display->terminal,
					      This->display, iter);
	      iter = next;
	    }
	  while (iter != This->display->display_buffers->window_list);
	}

      This->display->display_buffers->window_list =
	tn5250_window_list_destroy (This->display->display_buffers->
				    window_list);
      This->display->display_buffers->window_count = 0;
    }

  if (This->display->display_buffers->menubar_count > 0)
    {
      Tn5250Menubar *iter, *next;

      if ((iter = This->display->display_buffers->menubar_list) != NULL)
	{
	  do
	    {
	      next = iter->next;
	      tn5250_terminal_destroy_menubar (This->display->terminal,
					       This->display, iter);
	      iter = next;
	    }
	  while (iter != This->display->display_buffers->menubar_list);
	}

      This->display->display_buffers->menubar_list =
	tn5250_menubar_list_destroy (This->display->display_buffers->
				     menubar_list);
      This->display->display_buffers->menubar_count = 0;
    }

  if (This->display->display_buffers->scrollbar_count > 0)
    {
      tn5250_terminal_destroy_scrollbar (This->display->terminal,
					 This->display);
      This->display->display_buffers->scrollbar_list =
	tn5250_scrollbar_list_destroy (This->display->display_buffers->
				       scrollbar_list);
      This->display->display_buffers->scrollbar_count = 0;
    }

  tn5250_display_clear_unit (This->display);
  This->read_opcode = 0;

  return;
}


/****i* lib5250/tn5250_session_clear_unit_alternate
 * NAME
 *    tn5250_session_clear_unit_alternate
 * SYNOPSIS
 *    tn5250_session_clear_unit_alternate (This);
 * INPUTS
 *    Tn5250Session *      This       - 
 * DESCRIPTION
 *    DOCUMENT ME!!!
 *****/
static void
tn5250_session_clear_unit_alternate (Tn5250Session * This)
{
  unsigned char c;
  unsigned long errorcode;

  TN5250_LOG (("tn5250_session_clear_unit_alternate entered.\n"));
  c = tn5250_record_get_byte (This->record);
  TN5250_LOG (("tn5250_session_clear_unit_alternate, parameter is 0x%02X.\n",
	       (int) c));

  if (c != 0x00 && c != 0x80)
    {
      errorcode = TN5250_NR_INVALID_CLEAR_UNIT_ALT;
      tn5250_session_send_error (This, errorcode);
      return;
    }

  if (This->display->display_buffers->window_count > 0)
    {
      Tn5250Window *iter, *next;

      if ((iter = This->display->display_buffers->window_list) != NULL)
	{
	  do
	    {
	      next = iter->next;
	      TN5250_LOG (("destroying window id: %d\n", iter->id));
	      tn5250_terminal_destroy_window (This->display->terminal,
					      This->display, iter);
	      iter = next;
	    }
	  while (iter != This->display->display_buffers->window_list);
	}

      This->display->display_buffers->window_list =
	tn5250_window_list_destroy (This->display->display_buffers->
				    window_list);
      This->display->display_buffers->window_count = 0;
    }

  if (This->display->display_buffers->scrollbar_count > 0)
    {
      tn5250_terminal_destroy_scrollbar (This->display->terminal,
					 This->display);
      This->display->display_buffers->scrollbar_list =
	tn5250_scrollbar_list_destroy (This->display->display_buffers->
				       scrollbar_list);
      This->display->display_buffers->scrollbar_count = 0;
    }

  tn5250_display_clear_unit_alternate (This->display);
  This->read_opcode = 0;

  return;
}


/****i* lib5250/tn5250_session_clear_format_table
 * NAME
 *    tn5250_session_clear_format_table
 * SYNOPSIS
 *    tn5250_session_clear_format_table (This);
 * INPUTS
 *    Tn5250Session *      This       - 
 * DESCRIPTION
 *    DOCUMENT ME!!!
 *****/
static void
tn5250_session_clear_format_table (Tn5250Session * This)
{
  TN5250_LOG (("ClearFormatTable: entered.\n"));
  tn5250_display_clear_format_table (This->display);
  This->read_opcode = 0;
  return;
}


/****i* lib5250/tn5250_session_read_immediate
 * NAME
 *    tn5250_session_read_immediate
 * SYNOPSIS
 *    tn5250_session_read_immediate (This);
 * INPUTS
 *    Tn5250Session *      This       - 
 * DESCRIPTION
 *    DOCUMENT ME!!!
 *****/
static void
tn5250_session_read_immediate (Tn5250Session * This)
{
  int old_opcode;

  TN5250_LOG (("ReadImmediate: entered.\n"));
  old_opcode = This->read_opcode;
  This->read_opcode = CMD_READ_IMMEDIATE;
  tn5250_session_send_fields (This, 0);
  This->read_opcode = old_opcode;
  return;
}


/****i* lib5250/tn5250_session_handle_aidkey
 * NAME
 *    tn5250_session_handle_aidkey
 * SYNOPSIS
 *    ret = tn5250_session_handle_aidkey (This, key);
 * INPUTS
 *    Tn5250Session *      This       - 
 *    int                  key        - 
 * DESCRIPTION
 *    DOCUMENT ME!!!
 *****/
static int
tn5250_session_handle_aidkey (Tn5250Session * This, int key)
{
  Tn5250Buffer buf;
  StreamHeader header;

  switch (key)
    {
    case TN5250_SESSION_AID_PRINT:
    case TN5250_SESSION_AID_RECORD_BS:
      tn5250_buffer_init (&buf);
      tn5250_buffer_append_byte (&buf,
				 tn5250_display_cursor_y (This->display) + 1);
      tn5250_buffer_append_byte (&buf,
				 tn5250_display_cursor_x (This->display) + 1);
      tn5250_buffer_append_byte (&buf, key);

      header.h5250.flowtype = TN5250_RECORD_FLOW_DISPLAY;
      header.h5250.flags = TN5250_RECORD_H_NONE;
      header.h5250.opcode = TN5250_RECORD_OPCODE_NO_OP;

      tn5250_stream_send_packet (This->stream, tn5250_buffer_length (&buf),
				 header, tn5250_buffer_data (&buf));
      tn5250_buffer_free (&buf);
      break;

    case TN5250_SESSION_AID_SYSREQ:

      header.h5250.flowtype = TN5250_RECORD_FLOW_DISPLAY;
      header.h5250.flags = TN5250_RECORD_H_SRQ;
      header.h5250.opcode = TN5250_RECORD_OPCODE_NO_OP;

      tn5250_display_indicator_set (This->display,
				    TN5250_DISPLAY_IND_X_SYSTEM);
      if (This->display->keystate == TN5250_KEYSTATE_UNLOCKED)
	{
	  This->display->keystate = TN5250_KEYSTATE_LOCKED;
	}
      tn5250_stream_send_packet (This->stream, 0, header, NULL);
      tn5250_display_indicator_clear (This->display,
				      TN5250_DISPLAY_IND_X_SYSTEM);
      if (This->display->keystate == TN5250_KEYSTATE_LOCKED)
	{
	  This->display->keystate = TN5250_KEYSTATE_UNLOCKED;
	}
      break;

    case TN5250_SESSION_AID_TESTREQ:
      header.h5250.flowtype = TN5250_RECORD_FLOW_DISPLAY;
      header.h5250.flags = TN5250_RECORD_H_TRQ;
      header.h5250.opcode = TN5250_RECORD_OPCODE_NO_OP;
      tn5250_stream_send_packet (This->stream, 0, header, NULL);
      break;

    case TN5250_SESSION_AID_ATTN:
      header.h5250.flowtype = TN5250_RECORD_FLOW_DISPLAY;
      header.h5250.flags = TN5250_RECORD_H_ATN;
      header.h5250.opcode = TN5250_RECORD_OPCODE_NO_OP;

      tn5250_display_indicator_set (This->display,
				    TN5250_DISPLAY_IND_X_SYSTEM);
      if (This->display->keystate == TN5250_KEYSTATE_UNLOCKED)
	{
	  This->display->keystate = TN5250_KEYSTATE_LOCKED;
	}
      tn5250_stream_send_packet (This->stream, 0, header, NULL);
      tn5250_display_indicator_clear (This->display,
				      TN5250_DISPLAY_IND_X_SYSTEM);
      if (This->display->keystate == TN5250_KEYSTATE_LOCKED)
	{
	  This->display->keystate = TN5250_KEYSTATE_UNLOCKED;
	}
      break;

    case TN5250_SESSION_AID_HELP:
      if (This->display->keystate == TN5250_KEYSTATE_PREHELP)
	{
	  unsigned char src[2];
	  src[0] = (This->display->keySRC >> 8) & 0xff;
	  src[1] = (This->display->keySRC) & 0xff;
	  header.h5250.flowtype = TN5250_RECORD_FLOW_DISPLAY;
	  header.h5250.flags = TN5250_RECORD_H_HLP;
	  header.h5250.opcode = TN5250_RECORD_OPCODE_NO_OP;
	  TN5250_LOG (("PreHelp HELP key: %02x %02x\n", src[0], src[1]));
	  tn5250_stream_send_packet (This->stream, 2, header, src);
	  This->display->keystate = TN5250_KEYSTATE_POSTHELP;
	  break;
	}
      /* FALLTHROUGH */



    default:
      /* This should exit the read and set the X SYSTEM indicator. */
      tn5250_session_send_fields (This, key);
      break;
    }

  return 1;			/* Continue processing. */
}


/****i* lib5250/tn5250_session_output_only
 * NAME
 *    tn5250_session_output_only
 * SYNOPSIS
 *    tn5250_session_output_only (This);
 * INPUTS
 *    Tn5250Session *      This       - 
 * DESCRIPTION
 *    I'm not sure what the actual behavior of this opcode is supposed 
 *    to be.  I'm just sort-of fudging it based on empirical testing.
 *****/
static void
tn5250_session_output_only (Tn5250Session * This)
{
  unsigned char temp[2];

  TN5250_LOG (("OutputOnly: entered.\n"));

  /* 
     We get this if the user picks something they shouldn't from the
     System Request Menu - such as transfer to previous system.
   */
  if (tn5250_record_sys_request (This->record))
    {
      temp[0] = tn5250_record_get_byte (This->record);
      temp[1] = tn5250_record_get_byte (This->record);
      TN5250_LOG (("OutputOnly: ?? = 0x%02X; ?? = 0x%02X\n", temp[0],
		   temp[1]));
    }
  else
    {
      /*  
         Otherwise it seems to mean the Attention key menu is being 
         displayed.
       */
    }
  return;
}


/****i* lib5250/tn5250_session_save_screen
 * NAME
 *    tn5250_session_save_screen
 * SYNOPSIS
 *    tn5250_session_save_screen (This);
 * INPUTS
 *    Tn5250Session *      This       - 
 * DESCRIPTION
 *    DOCUMENT ME!!!
 *****/
static void
tn5250_session_save_screen (Tn5250Session * This)
{
  Tn5250Buffer buffer;
  StreamHeader header;

  TN5250_LOG (("SaveScreen: entered.\n"));

  tn5250_buffer_init (&buffer);
  tn5250_display_make_wtd_data (This->display, &buffer, NULL);

  /* Okay, now if we were in a Read MDT Fields or a Read Input Fields,
   * we need to append a command which would put us back in the appropriate
   * read. */
  if (This->read_opcode != 0)
    {
      tn5250_buffer_append_byte (&buffer, ESC);
      tn5250_buffer_append_byte (&buffer, This->read_opcode);
      tn5250_buffer_append_byte (&buffer, 0x00);	/* FIXME: ? CC1 */
      tn5250_buffer_append_byte (&buffer, 0x00);	/* FIXME: ? CC2 */
    }

  header.h5250.flowtype = TN5250_RECORD_FLOW_DISPLAY;
  header.h5250.flags = TN5250_RECORD_H_NONE;
  header.h5250.opcode = TN5250_RECORD_OPCODE_SAVE_SCR;

  tn5250_stream_send_packet (This->stream, tn5250_buffer_length (&buffer),
			     header, tn5250_buffer_data (&buffer));
  tn5250_buffer_free (&buffer);
  return;
}


/****i* lib5250/tn5250_session_save_partial_screen
 * NAME
 *    tn5250_session_save_partial_screen
 * SYNOPSIS
 *    tn5250_session_save_partial_screen (This);
 * INPUTS
 *    Tn5250Session *      This       - 
 * DESCRIPTION
 *    DOCUMENT ME!!!
 *****/
static void
tn5250_session_save_partial_screen (Tn5250Session * This)
{
  Tn5250Buffer buffer;
  StreamHeader header;
  unsigned char flagbyte;
  int toprow, leftcol, windepth, winwidth;

  TN5250_LOG (("SavePartialScreen: entered.\n"));

  flagbyte = tn5250_record_get_byte (This->record);
  toprow = tn5250_record_get_byte (This->record);
  leftcol = tn5250_record_get_byte (This->record);
  windepth = tn5250_record_get_byte (This->record);
  winwidth = tn5250_record_get_byte (This->record);

  tn5250_buffer_init (&buffer);
  tn5250_display_make_wtd_data (This->display, &buffer, NULL);

  /* Okay, now if we were in a Read MDT Fields or a Read Input Fields,
   * we need to append a command which would put us back in the appropriate
   * read. */
  if (This->read_opcode != 0)
    {
      tn5250_buffer_append_byte (&buffer, ESC);
      tn5250_buffer_append_byte (&buffer, This->read_opcode);
      tn5250_buffer_append_byte (&buffer, 0x00);	/* FIXME: ? CC1 */
      tn5250_buffer_append_byte (&buffer, 0x00);	/* FIXME: ? CC2 */
    }

  header.h5250.flowtype = TN5250_RECORD_FLOW_DISPLAY;
  header.h5250.flags = TN5250_RECORD_H_NONE;
  header.h5250.opcode = TN5250_RECORD_OPCODE_SAVE_SCR;

  tn5250_stream_send_packet (This->stream, tn5250_buffer_length (&buffer),
			     header, tn5250_buffer_data (&buffer));
  tn5250_buffer_free (&buffer);
  return;
}


/****i* lib5250/tn5250_session_roll
 * NAME
 *    tn5250_session_roll
 * SYNOPSIS
 *    tn5250_session_roll (This);
 * INPUTS
 *    Tn5250Session *      This       - 
 * DESCRIPTION
 *    DOCUMENT ME!!!
 *****/
static void
tn5250_session_roll (Tn5250Session * This)
{
  unsigned char direction, top, bot;
  int lines;

  direction = tn5250_record_get_byte (This->record);
  top = tn5250_record_get_byte (This->record);
  bot = tn5250_record_get_byte (This->record);

  TN5250_LOG (("Roll: direction = 0x%02X; top = %d; bottom = %d\n",
	       (int) direction, (int) top, (int) bot));

  lines = (direction & 0x1f);
  if ((direction & 0x80) == 0)
    {
      lines = -lines;
    }

  TN5250_LOG (("Roll: lines = %d\n", lines));

  if (lines == 0)
    {
      return;
    }

  tn5250_display_roll (This->display, top, bot, lines);
  return;
}


/****i* lib5250/tn5250_session_start_of_field
 * NAME
 *    tn5250_session_start_of_field
 * SYNOPSIS
 *    tn5250_session_start_of_field (This);
 * INPUTS
 *    Tn5250Session *      This       - 
 * DESCRIPTION
 *    DOCUMENT ME!!!
 *****/
static void
tn5250_session_start_of_field (Tn5250Session * This)
{
  int Y, X;
  /* int done, curpos; */
  Tn5250Field *field;
  unsigned char FFW1, FFW2, FCW1, FCW2;
  Tn5250Uint16 FCW;
  unsigned char Attr;
  unsigned char Length1, Length2, cur_char;
  int input_field;
  int endrow, endcol;
  int width;
  int height;
  unsigned long errorcode;
  int startx;
  int starty;
  int length;
  int resequence = 0;
  short magstripe = 0;
  short lightpen = 0;
  short magandlight = 0;
  short lightandattn = 0;
  short ideographiconly = 0;
  short ideographicdatatype = 0;
  short ideographiceither = 0;
  short ideographicopen = 0;
  int transparency = 0;
  short forwardedge = 0;
  int continuous = 0;
  int cont_first = 0;
  int cont_middle = 0;
  int cont_last = 0;
  static int wordwrap = 0;
  int progressionid;
  unsigned char highlightentryattr = 0x00;
  unsigned char pointeraid = 0x00;
  short selfcheckmod11 = 0;
  short selfcheckmod10 = 0;

  TN5250_LOG (("StartOfField: entered.\n"));

  cur_char = tn5250_record_get_byte (This->record);

  if ((cur_char & 0xe0) != 0x20)
    {
      /*
       * Lock the keyboard and clear any pending aid bytes.  
       * Only for input fields 
       */
      tn5250_display_indicator_set (This->display,
				    TN5250_DISPLAY_IND_X_SYSTEM);
      if (This->display->keystate == TN5250_KEYSTATE_UNLOCKED)
	{
	  This->display->keystate = TN5250_KEYSTATE_LOCKED;
	}

      input_field = 1;
      FFW1 = cur_char;
      FFW2 = tn5250_record_get_byte (This->record);

      TN5250_LOG (("StartOfField: field format word = 0x%02X%02X\n", FFW1,
		   FFW2));
      cur_char = tn5250_record_get_byte (This->record);

      continuous = 0;
      cont_first = 0;
      cont_middle = 0;
      cont_last = 0;
      progressionid = 0;
      FCW1 = 0;
      FCW2 = 0;
      FCW = 0;
      while ((cur_char & 0xe0) != 0x20)
	{
	  FCW1 = cur_char;
	  FCW2 = tn5250_record_get_byte (This->record);
	  FCW = (FCW1 << 8) | FCW2;

	  TN5250_LOG (("StartOfField: field control word = 0x%02X%02X\n",
		       FCW1, FCW2));
	  cur_char = tn5250_record_get_byte (This->record);

	  if (FCW1 == 0x80)
	    {
	      resequence = FCW2;
	    }

	  if (FCW == 0x8101)
	    {
	      magstripe = 1;
	    }

	  if (FCW == 0x8102)
	    {
	      lightpen = 1;
	    }

	  if (FCW == 0x8103)
	    {
	      magandlight = 1;
	    }

	  if (FCW == 0x8106)
	    {
	      lightandattn = 1;
	    }

	  if (FCW == 0x8200)
	    {
	      ideographiconly = 1;
	    }

	  if (FCW == 0x8220)
	    {
	      ideographicdatatype = 1;
	    }

	  if (FCW == 0x8240)
	    {
	      ideographiceither = 1;
	    }

	  if ((FCW == 0x8280) || (FCW == 0x82C0))
	    {
	      ideographicopen = 1;
	    }

	  if (FCW1 == 0x84)
	    {
	      transparency = FCW2;
	    }

	  if (FCW == 0x8501)
	    {
	      forwardedge = 1;
	    }

	  if (FCW == 0x8601)
	    {
	      continuous = 1;
	      cont_first = 1;
	    }

	  if (FCW == 0x8603)
	    {
	      continuous = 1;
	      cont_middle = 1;
	    }

	  if (FCW == 0x8602)
	    {
	      continuous = 1;
	      cont_last = 1;
	    }

	  if (FCW == 0x8680)
	    {
	      wordwrap = 1;
	    }
	  if ((FCW == 0x8602) && (wordwrap == 1))
	    {
	      wordwrap = 0;
	    }

	  if (FCW1 == 0x88)
	    {
	      progressionid = FCW2;
	    }

	  if (FCW1 == 0x89)
	    {
	      highlightentryattr = FCW2;
	    }

	  if (FCW1 == 0x8A)
	    {
	      pointeraid = FCW2;
	    }

	  if (FCW == 0xB140)
	    {
	      selfcheckmod11 = 1;
	    }

	  if (FCW == 0xB1A0)
	    {
	      selfcheckmod10 = 1;
	    }
	}
    }
  else
    {
      /* Output only field. */
      input_field = 0;
      FFW1 = 0;
      FFW2 = 0;
      FCW1 = 0;
      FCW2 = 0;
    }

  TN5250_ASSERT ((cur_char & 0xe0) == 0x20);

  TN5250_LOG (("StartOfField: attribute = 0x%02X\n", cur_char));
  Attr = cur_char;
  tn5250_display_addch (This->display, cur_char);

  Length1 = tn5250_record_get_byte (This->record);
  Length2 = tn5250_record_get_byte (This->record);

  length = (Length1 << 8) | Length2;

  width = tn5250_display_width (This->display);
  height = tn5250_display_height (This->display);

  startx = tn5250_display_cursor_x (This->display) + 1;
  starty = tn5250_display_cursor_y (This->display) + 1;

  TN5250_LOG (("starty = %d width = %d startx = %d length = %d height = %d\n",
	       starty, width, startx, length, height));

  if (continuous)
    {
      TN5250_LOG (("field is continuous\n"));
    }
  if (wordwrap)
    {
      TN5250_LOG (("field has wordwrap\n"));
    }
  if (progressionid != 0)
    {
      TN5250_LOG (("field has cursor progression: %d\n", progressionid));
    }

  if (((starty - 1) * width + startx - 1 + length) > width * height)
    {
      errorcode = TN5250_NR_INVALID_ROW_COL_ADDR;
      tn5250_session_send_error (This, errorcode);
      return;
    }

  if (input_field)
    {
      /* FIXME: `This address is either determined by the contents of the
       * preceding SBA order or calculated from the field length parameter
       * in the last SF order.'  5494 Functions Reference.  -JMF */

      X = tn5250_display_cursor_x (This->display);
      Y = tn5250_display_cursor_y (This->display);

      if ((field = tn5250_display_field_at (This->display, Y, X)) != NULL)
	{
	  TN5250_LOG (("StartOfField: Modifying field.\n"));
	  if (tn5250_field_start_col (field) == X &&
	      tn5250_field_start_row (field) == Y)
	    {
	      field->FFW = (FFW1 << 8) | FFW2;
	      field->attribute = Attr;
	    }
	}
      else
	{
	  TN5250_LOG (("StartOfField: Adding field.\n"));
	  field = tn5250_field_new (tn5250_display_width (This->display));
	  TN5250_ASSERT (field != NULL);

	  field->FFW = (FFW1 << 8) | FFW2;
	  field->resequence = resequence;
	  field->magstripe = magstripe;
	  field->lightpen = lightpen;
	  field->magandlight = magandlight;
	  field->lightandattn = lightandattn;
	  field->ideographiconly = ideographiconly;
	  field->ideographicdatatype = ideographicdatatype;
	  field->ideographiceither = ideographiceither;
	  field->ideographicopen = ideographicopen;
	  field->transparency = transparency;
	  field->forwardedge = forwardedge;
	  field->continuous = continuous;
	  field->continued_first = cont_first;
	  field->continued_middle = cont_middle;
	  field->continued_last = cont_last;
	  field->wordwrap = wordwrap;
	  field->nextfieldprogressionid = progressionid;
	  field->highlightentryattr = highlightentryattr;
	  field->pointeraid = pointeraid;
	  field->selfcheckmod11 = selfcheckmod11;
	  field->selfcheckmod10 = selfcheckmod10;
	  field->attribute = Attr;
	  field->length = (Length1 << 8) | Length2;
	  field->start_row = Y;
	  field->start_col = X;

	  tn5250_dbuffer_add_field (tn5250_display_dbuffer (This->display),
				    field);
	}
    }
  else
    {
      TN5250_LOG (("StartOfField: Output only field.\n"));
      field = NULL;
    }

  if (input_field)
    {
      TN5250_ASSERT (field != NULL);

      endrow = tn5250_field_end_row (field);
      endcol = tn5250_field_end_col (field);

      if (endcol == tn5250_display_width (This->display) - 1)
	{
	  endcol = 0;
	  if (endrow == tn5250_display_height (This->display) - 1)
	    {
	      endrow = 0;
	    }
	  else
	    {
	      endrow++;
	    }
	}
      else
	{
	  endcol++;
	}

      TN5250_LOG (("StartOfField: endrow = %d; endcol = %d\n", endrow,
		   endcol));

#ifndef NDEBUG
      tn5250_field_dump (field);
#endif

      tn5250_display_set_cursor (This->display, endrow, endcol);
      tn5250_display_addch (This->display, 0x20);
      tn5250_display_set_cursor (This->display, Y, X);
    }
  return;
}


/****i* lib5250/tn5250_session_start_of_header
 * NAME
 *    tn5250_session_start_of_header
 * SYNOPSIS
 *    tn5250_session_start_of_header (This);
 * INPUTS
 *    Tn5250Session *      This       - 
 * DESCRIPTION
 *    Clear the format table, get the table header data, and copy it to the
 *    table.  This includes the operator error line (byte 4).
 *****/
static void
tn5250_session_start_of_header (Tn5250Session * This)
{
  int i, n;
  unsigned char *ptr = NULL;
  unsigned long errorcode;

  TN5250_LOG (("StartOfHeader: entered.\n"));

  tn5250_dbuffer_clear_table (tn5250_display_dbuffer (This->display));
  tn5250_display_clear_pending_insert (This->display);
  tn5250_display_indicator_set (This->display, TN5250_DISPLAY_IND_X_SYSTEM);
  if (This->display->keystate == TN5250_KEYSTATE_UNLOCKED)
    {
      This->display->keystate = TN5250_KEYSTATE_LOCKED;
    }

  n = tn5250_record_get_byte (This->record);
  if (n < 0 || n > 7)
    {
      errorcode = TN5250_NR_INVALID_SOH_LENGTH;
      tn5250_session_send_error (This, errorcode);
      return;
    }
  TN5250_ASSERT ((n > 0 && n <= 7));
  if (n > 0)
    {
      ptr = (unsigned char *) malloc (n);
    }
  for (i = 0; i < n; i++)
    {
      ptr[i] = tn5250_record_get_byte (This->record);
    }
  tn5250_display_set_header_data (This->display, ptr, n);
  if (ptr != NULL)
    {
      free (ptr);
    }
  return;
}


/****i* lib5250/tn5250_session_set_buffer_address
 * NAME
 *    tn5250_session_set_buffer_address
 * SYNOPSIS
 *    tn5250_session_set_buffer_address (This);
 * INPUTS
 *    Tn5250Session *      This       - 
 * DESCRIPTION
 *    FIXME: According to "Functions Reference",
 *    http://publib.boulder.ibm.com:80/cgi-bin/bookmgr/BOOKS/C02E2001/15.6.4,
 *    a zero in the column field (X) is acceptable if:
 *    - The rows field is 1.
 *    - The SBA order is followed by an SF (Start of Field) order.
 *    This is how to start a field at row 1, column 1.  We should also be
 *    able to handle a starting attribute there in the display buffer.
 *****/
static void
tn5250_session_set_buffer_address (Tn5250Session * This)
{
  int X, Y;
  int width;
  int height;
  unsigned long errorcode;

  Y = tn5250_record_get_byte (This->record);
  X = tn5250_record_get_byte (This->record);

  width = tn5250_display_width (This->display);
  height = tn5250_display_height (This->display);

  if (Y == 0 || Y > height || X == 0 || X > width)
    {
      errorcode = TN5250_NR_INVALID_ROW_COL_ADDR;
      tn5250_session_send_error (This, errorcode);
      return;
    }

  /* Since we can't handle it yet... */
  TN5250_ASSERT ((X == 0 && Y == 1) || (X > 0 && Y > 0));

  tn5250_display_set_cursor (This->display, Y - 1, X - 1);
  TN5250_LOG (("SetBufferAddress: row = %d; col = %d\n", Y, X));

  return;
}


static void
tn5250_session_write_extended_attribute (Tn5250Session * This)
{
  /* 
     This order is not really implemented.  It is just here for catching data 
     stream errors.
   */
  unsigned char attrtype;
  unsigned char attr;
  unsigned long errorcode;

  attrtype = tn5250_record_get_byte (This->record);
  attr = tn5250_record_get_byte (This->record);
  TN5250_LOG (("WEA order (type = 0x%02X, attribute = 0x%02X).\n",
	       attrtype, attr));

  if (attrtype != 0x01 && attrtype != 0x03 && attrtype != 0x05)
    {
      errorcode = TN5250_NR_INVALID_EXT_ATTR_TYPE;
      tn5250_session_send_error (This, errorcode);
      return;
    }
  return;
}


static void
tn5250_session_write_display_structured_field (Tn5250Session * This)
{
  /*
     This order is not really implemented.  We just do enough to checking so
     we can handle data stream errors.
   */
  unsigned char class;
  unsigned char type;
  unsigned long errorcode;
  int len;

  /* first two bytes are the length of the WDSF parameter (the length bytes
     are included in the count) */

  len = (tn5250_record_get_byte (This->record) << 8) |
    tn5250_record_get_byte (This->record);

  /* 3rd byte is the class...  0xd9, I think, always */

  class = tn5250_record_get_byte (This->record);

  if (class != 0xd9)
    {
      errorcode = TN5250_NR_INVALID_SF_CLASS_TYPE;
      tn5250_session_send_error (This, errorcode);
      return;
    }

  /* 4th byte is the type of structured field */

  type = tn5250_record_get_byte (This->record);

  len -= 4;

  switch (type)
    {
    case UNREST_WIN_CURS_MOVE:
    case PROGRAMMABLE_MOUSE_BUT:
    case REM_GUI_SCROLL_BAR_FIELD:
    case DRAW_ERASE_GRID_LINES:
    case CLEAR_GRID_LINE_BUFFER:
      TN5250_LOG (("Unhandled WDSF class=%02x type=%02x data=", class, type));
      while (len > 0)
	{
	  TN5250_LOG (("%02x", tn5250_record_get_byte (This->record)));
	  len--;
	}
      TN5250_LOG (("\n"));
      break;
    case DEFINE_SELECTION_FIELD:
      tn5250_session_define_selection_field (This, len);
      break;
    case REM_GUI_SEL_FIELD:
      tn5250_session_remove_gui_selection_field (This, len);
      break;
    case WRITE_DATA:
      tn5250_session_write_data_structured_field (This, len);
      break;
    case CREATE_WINDOW:
      tn5250_session_create_window_structured_field (This, len);
      break;
    case REM_GUI_WINDOW:
      tn5250_session_remove_gui_window_structured_field (This, len);
      break;
    case DEFINE_SCROLL_BAR_FIELD:
      tn5250_session_define_scrollbar (This, len);
      break;
    case REM_ALL_GUI_CONSTRUCTS:
      tn5250_session_remove_all_gui_constructs_structured_field (This, len);
      break;
    default:
      TN5250_LOG (("tn5250_write_display_structured_field: Invalid SF Class: %02x\n", type));
      errorcode = TN5250_NR_INVALID_SF_CLASS_TYPE;
      tn5250_session_send_error (This, errorcode);
      return;
    }
  return;
}


static void
tn5250_session_transparent_data (Tn5250Session * This)
{
  unsigned char l1, l2;
  unsigned td_len;
  int width;
  int height;
  int curx;
  int cury;
  int end;
  int errorcode;

  width = tn5250_display_width (This->display);
  height = tn5250_display_height (This->display);

  curx = tn5250_display_cursor_x (This->display);
  cury = tn5250_display_cursor_y (This->display);

  l1 = tn5250_record_get_byte (This->record);
  l2 = tn5250_record_get_byte (This->record);
  td_len = (l1 << 8) | l2;

  end = (cury - 1) * width + curx + td_len;

  TN5250_LOG (("TD order (length = X'%04X').\n", td_len));

  if (end > width * height)
    {
      errorcode = TN5250_NR_INVALID_ROW_COL_ADDR;
      tn5250_session_send_error (This, errorcode);
      return;
    }

  while (td_len > 0)
    {
      l1 = tn5250_record_get_byte (This->record);
      tn5250_display_addch (This->display, l1);
      td_len--;
    }
  return;
}


static void
tn5250_session_move_cursor (Tn5250Session * This)
{
  /*
     FIXME:  This function is not really implemented.  It is just here to
     catch errors in the data stream.
   */
  unsigned char x = 0xff, y = 0xff;
  int width;
  int height;
  unsigned long errorcode;

  y = tn5250_record_get_byte (This->record) - 1;
  x = tn5250_record_get_byte (This->record) - 1;
  TN5250_LOG (("MC order (y = X'%02X', x = X'%02X').\n", y, x));

  width = tn5250_display_width (This->display);
  height = tn5250_display_height (This->display);

  if (y == 0 || y > height || x == 0 || x > width)
    {
      errorcode = TN5250_NR_INVALID_ROW_COL_ADDR;
      tn5250_session_send_error (This, errorcode);
      return;
    }
  return;
}


static void
tn5250_session_insert_cursor (Tn5250Session * This)
{
  unsigned char x = 0xff, y = 0xff;
  int width;
  int height;
  unsigned long errorcode;

  y = tn5250_record_get_byte (This->record);
  x = tn5250_record_get_byte (This->record);

  width = tn5250_display_width (This->display);
  height = tn5250_display_height (This->display);

  if (y == 0 || y > height || x == 0 || x > width)
    {
      errorcode = TN5250_NR_INVALID_ROW_COL_ADDR;
      tn5250_session_send_error (This, errorcode);
      return;
    }

  y--;
  x--;

  TN5250_LOG (("IC order (y = X'%02X', x = X'%02X').\n", y, x));
  tn5250_display_set_pending_insert (This->display, y, x);
  return;
}


static void
tn5250_session_erase_to_address (Tn5250Session * This)
{
  /*
   * FIXME:  This function is not completely implemented.  It needs to
   * support erasing only those attributes listed in the attributes list.
   * Right now it only supports the case of erasing everything.
   *
   * From reading the docs it seems that erasing only attributes will only
   * happen after previously receiving a WEA order that puts extended
   * attributes in the Extended Character Buffer.  Since we don't support
   * the extended character buffer we shouldn't ever get a WEA order (the
   * QUERY order figures this out).  Thus it probably doesn't matter that we
   * don't support erasing only attributes.
   */

  int x;
  int y;
  int attribute;
  int length;
  int curx;
  int cury;
  unsigned long errorcode;
  int start;
  int end;
  int width;
  int height;

  TN5250_LOG (("EraseToAddress: entered.\n"));

  curx = tn5250_display_cursor_x (This->display) + 1;
  cury = tn5250_display_cursor_y (This->display) + 1;

  y = tn5250_record_get_byte (This->record);
  x = tn5250_record_get_byte (This->record);
  length = tn5250_record_get_byte (This->record);
  width = tn5250_display_width (This->display);
  height = tn5250_display_height (This->display);
  start = ((cury - 1) * width) + curx;
  end = ((y - 1) * width) + x;

  if (end < start || length < 2 || length > 5)
    {
      errorcode = TN5250_NR_INVALID_ROW_COL_ADDR;
      tn5250_session_send_error (This, errorcode);
      return;
    }

  TN5250_LOG (("Erase from %d, %d to %d, %d\n", curx, cury, x, y));
  TN5250_LOG (("Erase attribute type(s) ="));

  length--;
  while (length > 0)
    {
      attribute = tn5250_record_get_byte (This->record);
      TN5250_LOG ((" 0x%02X", attribute));
      length--;
    }
  TN5250_LOG (("\n"));

  if (attribute == 0xFF)
    {
      tn5250_display_erase_region (This->display, cury, curx, y, x, 1, width);
    }

  /* the docs say:
   * [the EA order] sets the current display address to the location
   * specified in the EA plus one.
   * But that doesn't seem to always work.  The address in the EA can be
   * the end of the screen, so we can't really set the cursor there.
   */
  if (x >= width)
    {
      x = 0;
    }
  if (y >= height)
    {
      y = 0;
    }
  tn5250_display_set_cursor (This->display, y, x);
  return;
}


/****i* lib5250/tn5250_session_repeat_to_address
 * NAME
 *    tn5250_session_repeat_to_address
 * SYNOPSIS
 *    tn5250_session_repeat_to_address (This);
 * INPUTS
 *    Tn5250Session *      This       - 
 * DESCRIPTION
 *    DOCUMENT ME!!!
 *****/
static void
tn5250_session_repeat_to_address (Tn5250Session * This)
{
  unsigned char temp[3];
  int x, y;
  /* These variables don't appear to be needed
   *   int ins_loc;
   *   Tn5250Field  *field;
   */
  unsigned long errorcode;
  int width;
  int height;
  int start;
  int end;

  TN5250_LOG (("RepeatToAddress: entered.\n"));

  temp[0] = tn5250_record_get_byte (This->record);
  temp[1] = tn5250_record_get_byte (This->record);
  temp[2] = tn5250_record_get_byte (This->record);

  y = tn5250_display_cursor_y (This->display) + 1;
  x = tn5250_display_cursor_x (This->display) + 1;
  width = tn5250_display_width (This->display);
  height = tn5250_display_height (This->display);
  start = (y - 1) * width + x;
  end = (temp[0] - 1) * width + temp[1];

  TN5250_LOG (("RepeatToAddress: row = %d; col = %d; char = 0x%02X\n",
	       temp[0], temp[1], temp[2]));

  if (temp[0] == 0 || temp[0] > height
      || temp[1] == 0 || temp[1] > width || end < start)
    {
      errorcode = TN5250_NR_INVALID_ROW_COL_ADDR;
      tn5250_session_send_error (This, errorcode);
      return;
    }

  while (1)
    {
      y = tn5250_display_cursor_y (This->display);
      x = tn5250_display_cursor_x (This->display);

      tn5250_display_addch (This->display, temp[2]);

      if (y == temp[0] - 1 && x == temp[1] - 1)
	{
	  break;
	}
    }
  return;
}


/****i* lib5250/tn5250_session_write_structured_field
 * NAME
 *    tn5250_session_write_structured_field
 * SYNOPSIS
 *    tn5250_session_write_structured_field (This);
 * INPUTS
 *    Tn5250Session *      This       - 
 * DESCRIPTION
 *    DOCUMENT ME!!!
 *****/
static void
tn5250_session_write_structured_field (Tn5250Session * This)
{
  unsigned char temp[5];
  unsigned long errorcode;

  TN5250_LOG (("WriteStructuredField: entered.\n"));

  temp[0] = tn5250_record_get_byte (This->record);
  temp[1] = tn5250_record_get_byte (This->record);
  temp[2] = tn5250_record_get_byte (This->record);
  temp[3] = tn5250_record_get_byte (This->record);
  temp[4] = tn5250_record_get_byte (This->record);

  TN5250_LOG (("WriteStructuredField: length = %d\n",
	       (temp[0] << 8) | temp[1]));
  TN5250_LOG (("WriteStructuredField: command class = 0x%02X\n", temp[2]));
  TN5250_LOG (("WriteStructuredField: command type = 0x%02X\n", temp[3]));

  if (temp[2] != 0xD9)
    {
      TN5250_LOG (("tn5250_write_structured_field: Invalid SF Class: %02x\n",
		   temp[2]));
      errorcode = TN5250_NR_INVALID_SF_CLASS_TYPE;
      tn5250_session_send_error (This, errorcode);
      return;
    }
  else
    {
      switch (temp[3])
	{
	case DEFINE_AUDIT_WINDOW_TABLE:
	case DEFINE_COMMAND_KEY_FUNCTION:
	case READ_TEXT_SCREEN:
	case DEFINE_PENDING_OPERATIONS:
	case DEFINE_TEXT_SCREEN_FORMAT:
	case DEFINE_SCALE_TIME:
	case WRITE_TEXT_SCREEN:
	case DEFINE_SPECIAL_CHARACTERS:
	case PENDING_DATA:
	case DEFINE_OPERATOR_ERROR_MSGS:
	case DEFINE_PITCH_TABLE:
	case DEFINE_FAKE_DP_CMD_KEY_FUNC:
	case PASS_THROUGH:
	case SF_5250_QUERY:
	case SF_5250_QUERY_STATION_STATE:
	  break;

	default:
	  TN5250_LOG (("tn5250_write_structured_field(2): Invalid SF Class: %02x\n", temp[3]));
	  errorcode = TN5250_NR_INVALID_SF_CLASS_TYPE;
	  tn5250_session_send_error (This, errorcode);
	  return;
	}
    }

  tn5250_session_query_reply (This);
  return;
}


/****i* lib5250/tn5250_session_read_screen_immediate
 * NAME
 *    tn5250_session_read_screen_immediate
 * SYNOPSIS
 *    tn5250_session_read_screen_immediate (This);
 * INPUTS
 *    Tn5250Session *      This       - 
 * DESCRIPTION
 *    DOCUMENT ME!!!
 *****/
static void
tn5250_session_read_screen_immediate (Tn5250Session * This)
{
  int row, col;
  int buffer_size;
  unsigned char *buffer;
  StreamHeader header;

  TN5250_LOG (("ReadScreenImmediate: entered.\n"));

  buffer_size = tn5250_display_width (This->display) *
    tn5250_display_height (This->display);

  buffer = (unsigned char *) malloc (buffer_size);

  for (row = 0; row < tn5250_display_height (This->display); row++)
    {
      for (col = 0; col < tn5250_display_width (This->display); col++)
	{
	  buffer[row * tn5250_display_width (This->display) + col]
	    = tn5250_display_char_at (This->display, row, col);
	}
    }

  header.h5250.flowtype = TN5250_RECORD_FLOW_DISPLAY;
  header.h5250.flags = TN5250_RECORD_H_NONE;
  header.h5250.opcode = TN5250_RECORD_OPCODE_NO_OP;

  tn5250_stream_send_packet (This->stream, buffer_size, header, buffer);

  free (buffer);
  return;
}


/****i* lib5250/tn5250_session_read_cmd
 * NAME
 *    tn5250_session_read_cmd
 * SYNOPSIS
 *    tn5250_session_read_cmd (This, readop);
 * INPUTS
 *    Tn5250Session *      This       - 
 *    int                  readop     - 
 * DESCRIPTION
 *    DOCUMENT ME!!!
 *****/
static void
tn5250_session_read_cmd (Tn5250Session * This, int readop)
{
  unsigned char CC1, CC2;

  TN5250_LOG (("tn5250_session_read_cmd: readop = 0x%02X.\n", readop));

  CC1 = tn5250_record_get_byte (This->record);
  tn5250_session_handle_cc1 (This, CC1);

  CC2 = tn5250_record_get_byte (This->record);
  tn5250_session_handle_cc2 (This, CC2);

  TN5250_LOG (("tn5250_session_read_cmd: CC1 = 0x%02X; CC2 = 0x%02X\n", CC1,
	       CC2));
  tn5250_display_indicator_clear (This->display,
				  TN5250_DISPLAY_IND_X_SYSTEM |
				  TN5250_DISPLAY_IND_X_CLOCK);

  /* The read command only uninhibits the keyboard if we were in a
     "normal locked" keystate.  (In other words, we weren't inhibited
     by an error, but by a normal condition) */
  if (This->display->keystate == TN5250_KEYSTATE_LOCKED)
    {
      tn5250_display_uninhibit (This->display);
      This->display->keystate = TN5250_KEYSTATE_UNLOCKED;
    }

  This->read_opcode = readop;
  return;
}


/****i* lib5250/tn5250_session_query_reply
 * NAME
 *    tn5250_session_query_reply
 * SYNOPSIS
 *    tn5250_session_query_reply (This);
 * INPUTS
 *    Tn5250Session *      This       - 
 * DESCRIPTION
 *    DOCUMENT ME!!!
 *****/
static void
tn5250_session_query_reply (Tn5250Session * This)
{
  unsigned char temp[67];
  const char *scan;
  int dev_type, dev_model, i, enhanced;
  StreamHeader header;

  TN5250_LOG (("Sending QueryReply.\n"));


  if (tn5250_terminal_enhanced (This->display->terminal))
    {
      enhanced = tn5250_config_get_bool (This->config, "enhanced");
    }
  else
    {
      enhanced = 0;
    }

  if (enhanced)
    {
      TN5250_LOG (("turning on enhanced 5250 features\n"));
    }

  temp[0] = 0x00;		/* Cursor Row/Column (set to zero) */
  temp[1] = 0x00;

  temp[2] = 0x88;		/* Inbound Write Structured Field Aid */

  /* Note that the IBM docs show this length as X'0044'. */
  temp[3] = 0x00;		/* Length of Query Reply */

  if (enhanced)
    {
      temp[4] = 0x40;
    }
  else
    {
      temp[4] = 0x3A;
    }

  temp[5] = 0xD9;		/* Command class */

  temp[6] = 0x70;		/* Command type - Query */

  temp[7] = 0x80;		/* Flag byte */

  /* The following bytes are supposed to correspond to table 89 of section
   * 15.27.2 5250 QUERY Command of the Functions Reference manual.
   */

  /* This appears to be wrong.  Table 89 describes the first field of the
   * QUERY Response Data Field as:
   * Workstation Control Unit  |  X'0043'  |  Identifies the controller as
   *                                          a 5494
   *
   * I don't know where the X'0600' came from, but it appears to be
   * working.
   */
  temp[8] = 0x06;		/* Controller hardware class */
  temp[9] = 0x00;		/* 0x600 - Other WSF or another 5250 emulator */

  /* Here the docs use X'040310' for version 4 release 3.1.  But it doesn't
   * appear to make any difference.
   */
  temp[10] = 0x01;		/* Controller code level (Version 1 Release 1.0 */
  temp[11] = 0x01;
  temp[12] = 0x00;

  temp[13] = 0x00;		/* Reserved (set to zero) */
  temp[14] = 0x00;
  temp[15] = 0x00;
  temp[16] = 0x00;
  temp[17] = 0x00;
  temp[18] = 0x00;
  temp[19] = 0x00;
  temp[20] = 0x00;
  temp[21] = 0x00;
  temp[22] = 0x00;
  temp[23] = 0x00;
  temp[24] = 0x00;
  temp[25] = 0x00;
  temp[26] = 0x00;
  temp[27] = 0x00;
  temp[28] = 0x00;

  temp[29] = 0x01;		/* Display or printer emulation */

  /* Retreive the device type from the stream, parse out the device
   * type and model, convert it to EBCDIC, and put it in the packet. */

  scan = tn5250_config_get (This->config, "env.TERM");
  TN5250_ASSERT (scan != NULL);
  TN5250_ASSERT (strchr (scan, '-') != NULL);

  scan = strchr (scan, '-') + 1;

  dev_type = atoi (scan);
  if (strchr (scan, '-'))
    {
      dev_model = atoi (strchr (scan, '-') + 1);
    }
  else
    {
      dev_model = 1;
    }

  sprintf ((char *) temp + 30, "%04d", dev_type);
  sprintf ((char *) temp + 35, "%02d", dev_model);

  for (i = 30; i <= 36; i++)
    {
      temp[i] =
	tn5250_char_map_to_remote (tn5250_display_char_map (This->display),
				   temp[i]);
    }

  temp[37] = 0x02;		/* Keyboard ID:
				   X'02' = Standard Keyboard
				   X'82' = G Keyboard */

  temp[38] = 0x00;		/* Extended keyboard ID */
  temp[39] = 0x00;		/* Reserved */

  /* I really doubt we have serial number.  This should probably be set to
   * all zeroes since the docs say so for workstations without a serial
   * number.
   */
  temp[40] = 0x00;		/* Display serial number */
  temp[41] = 0x61;
  temp[42] = 0x50;
  temp[43] = 0x00;

  /* Here the docs suggest that the maximum is 256.  I guess it would be
   * possible to have a screen with 1701 input fields (a 127 by 27 screen
   * can have at most (126/2) * 27) = 1701 fields on it).  At any rate,
   * 65535 doesn't appear to be supported.
   */
  temp[44] = 0xff;		/* Maximum number of input fields (65535) */
  temp[45] = 0xff;

  temp[46] = 0x00;		/* Control unit customization */

  temp[47] = 0x00;		/* Reserved (set to zero) */
  temp[48] = 0x00;

  temp[49] = 0x23;		/* Controller/Display Capability */
  temp[50] = 0x31;
  temp[51] = 0x00;		/* Reserved */
  temp[52] = 0x00;

  /*  byte 53:
   *     bits 0-2:  B'000' =  no graphics
   *                B'001' =  5292-style graphics
   *                B'010' =  GDDM-OS/2 Link Graphics
   *     bit  3:  extended 3270 data stream capability
   *     bit  4:  pointer device (mouse) available
   *     bit  5:  GUI-like characters available
   *     bit  6:  Enhanced 5250 FCW & WDSFs
   *     bit  7:  WRITE ERROR CODE TO WINDOW command support
   *
   *  byte 54:
   *     bit  0:  enhanced user interface support level 2
   *     bit  1:  GUI device w/all-points addressable windows, etc
   *     bit  2:  WordPerfect support is available
   *     bit  3:  Dynamic status & scale line
   *     bit  4:  enhanced user interface support level 3
   *     bit  5:  cursor draw is supported in Office editor
   *     bits 6-7: video delivery capability B'00' = none, B'01'=5250
   *
   *  Enabling enhanced 5250 means we need to support:
   *  WDSFs: CREATE WINDOW
   *         UNRESTRICTED CURSOR MOVEMENT
   *         DEFINE SELECTION FIELD
   *         DEFINE SCROLL BAR FIELD
   *         REMOVE ALL GUI CONSTRUCTS
   *         REMOVE GUI WINDOW
   *         REMOVE GUI SELECTION FIELD
   *         REMOVE GUI SCROLL BAR FIELD
   *         READ SCREEN TO PRINT
   *         READ SCREEN TO PRINT WITH EXTENDED ATTRIBUTES
   *         WRITE ERROR CODE TO WINDOW
   *         SAVE PARTIAL SCREEN
   *         RESTORE PARTIAL SCREEN
   *
   *   FCWs: Continued Fields
   *         Cursor Progression
   *         Highlighted
   *         Mouse Selection
   *
   *  Enabling enhanced 5250 Level 2 means we also need to support:
   *  WDSFs:   WRITE DATA
   *           PROGRAMMABLE MOUSE BUTTONS
   *   FCWs:   Word Wrap
   *           Ideographic Continued Entry Fields
   */
  if (enhanced)
    {
      temp[53] = 0x02;
      temp[54] = 0x80;
    }
  else
    {
      temp[53] = 0x00;
      temp[54] = 0x00;
    }

  temp[55] = 0x00;
  temp[56] = 0x00;
  temp[57] = 0x00;
  temp[58] = 0x00;
  temp[59] = 0x00;
  temp[60] = 0x00;
  temp[61] = 0x00;
  temp[62] = 0x00;
  temp[63] = 0x00;
  temp[64] = 0x00;
  temp[65] = 0x00;
  temp[66] = 0x00;

  header.h5250.flowtype = TN5250_RECORD_FLOW_DISPLAY;
  header.h5250.flags = TN5250_RECORD_H_NONE;
  header.h5250.opcode = TN5250_RECORD_OPCODE_NO_OP;

  tn5250_stream_send_packet (This->stream, temp[4] + 3, header,
			     (unsigned char *) temp);
  return;
}


/****i* lib5250/tn5250_session_define_selection_field
 * NAME
 *    tn5250_session_define_selection_field
 * SYNOPSIS
 *    tn5250_session_define_selection_field (This, len)
 * INPUTS
 *    Tn5250Session *      This       -
 *    int                  length     -
 * DESCRIPTION
 *    DOCUMENT ME!!!
 *****/
static void
tn5250_session_define_selection_field (Tn5250Session * This, int length)
{
  Tn5250DBuffer *dbuffer;
  Tn5250Menubar *menubar;
  unsigned char flagbyte1;
  unsigned char flagbyte2;
  unsigned char flagbyte3;
  unsigned char fieldtype;
  unsigned char padding;
  unsigned char separator;
  unsigned char selectionchar;
  unsigned char cancelaid;
  unsigned char reserved;
  int minorlength;
  int menuitemcount;
  short usescrollbar = 0;
  short createnewmenubar = 0;

  TN5250_LOG (("Entering tn5250_session_define_selection_field()\n"));


  /* Menus can't overlay each other.  If this menu is in the same position as
   * another, redefine the menu instead of creating a new one.
   */
  dbuffer = tn5250_display_dbuffer (This->display);

  if ((menubar = tn5250_menubar_hit_test (dbuffer->menubar_list,
					  tn5250_display_cursor_x (This->
								   display),
					  tn5250_display_cursor_y (This->
								   display)))
      == NULL)
    {
      menubar = tn5250_menubar_new ();
      createnewmenubar = 1;
    }

  flagbyte1 = tn5250_record_get_byte (This->record);

  /* The first two bits define mouse characteristics */
  if ((flagbyte1 & 0xC0) == 0)
    {
      TN5250_LOG (("Use this selection field in all cases\n"));
    }

  /* both bits cannot be on */
  if ((flagbyte1 & 0xC0) == 3)
    {
      TN5250_LOG (("Reserved usage of mouse characteristics!\n"));
    }
  else
    {
      if (flagbyte1 & 0x40)
	{
	  TN5250_LOG (("Use this selection field only if the display does not have a mouse\n"));
	}
      if (flagbyte1 & 0x80)
	{
	  TN5250_LOG (("Use this selection field only if the display has a mouse\n"));
	}
    }

  /* bits 4 and 5 define auto enter */
  if ((flagbyte1 & 0x0C) == 0)
    {
      TN5250_LOG (("Selection field is not auto-enter\n"));
    }
  else if ((flagbyte1 & 0x0C) == 1)
    {
      TN5250_LOG (("Selection field is auto-enter on selection except if double-digit numeric selection is used\n"));
    }
  else if ((flagbyte1 & 0x0C) == 2)
    {
      TN5250_LOG (("Selection field is auto-enter on selection or deselection except if double-digit numeric selection is used\n"));
    }
  else
    {
      TN5250_LOG (("Selection field is auto-enter on selection except if single-digit or double-digit numeric selection is used\n"));
    }

  /* bit six controls auto-select */
  if (flagbyte1 & 0x02)
    {
      TN5250_LOG (("Auto-select active\n"));
    }


  flagbyte2 = tn5250_record_get_byte (This->record);

  if (flagbyte2 & 0x80)
    {
      TN5250_LOG (("Use scroll bar\n"));
      usescrollbar = 1;
    }

  if (flagbyte2 & 0x40)
    {
      TN5250_LOG (("Add blank after numeric seperator\n"));
    }

  if (flagbyte2 & 0x20)
    {
      TN5250_LOG (("Use * for unavailable options\n"));
    }

  if (flagbyte2 & 0x10)
    {
      TN5250_LOG (("Limit cursor to input capable positions\n"));
    }

  if (flagbyte2 & 0x08)
    {
      TN5250_LOG (("Field advance = character advance\n"));
    }

  if (flagbyte2 & 0x04)
    {
      menubar->restricted_cursor = 1;
      TN5250_LOG (("Cursor may not exit selection field\n"));
    }
  else
    {
      menubar->restricted_cursor = 0;
    }


  flagbyte3 = tn5250_record_get_byte (This->record);

  if (flagbyte3 & 0x80)
    {
      TN5250_LOG (("Make selected choices available when keyboard is unlocked\n"));
    }


  TN5250_LOG (("Selection field type: "));
  fieldtype = tn5250_record_get_byte (This->record);

  if (fieldtype == 0x01)
    {
      TN5250_LOG (("Menubar\n"));
    }
  else if (fieldtype == 0x11)
    {
      TN5250_LOG (("Single choice selection field\n"));
    }
  else if (fieldtype == 0x12)
    {
      TN5250_LOG (("Multiple choice selection field\n"));
    }
  else if (fieldtype == 0x21)
    {
      TN5250_LOG (("Single choice selection list\n"));
    }
  else if (fieldtype == 0x22)
    {
      TN5250_LOG (("Multiple choice selection list\n"));
    }
  else if (fieldtype == 0x31)
    {
      TN5250_LOG (("Single choice selection field and pulldown list\n"));
    }
  else if (fieldtype == 0x32)
    {
      TN5250_LOG (("Multiple choice selection field and pulldown list\n"));
    }
  else if (fieldtype == 0x41)
    {
      TN5250_LOG (("Push buttons\n"));
    }
  else if (fieldtype == 0x51)
    {
      TN5250_LOG (("Push buttons in a pulldown menu\n"));
    }
  else
    {
      TN5250_LOG (("Invalid field selection type!!\n"));
    }

  menubar->flagbyte1 = flagbyte1;
  menubar->flagbyte2 = flagbyte2;
  menubar->flagbyte3 = flagbyte3;
  menubar->type = fieldtype;

  reserved = tn5250_record_get_byte (This->record);
  reserved = tn5250_record_get_byte (This->record);
  reserved = tn5250_record_get_byte (This->record);
  reserved = tn5250_record_get_byte (This->record);
  reserved = tn5250_record_get_byte (This->record);
  menubar->itemsize = tn5250_record_get_byte (This->record);
  TN5250_LOG (("textsize = 0x%02X (%d)\n", menubar->itemsize,
	       menubar->itemsize));
  menubar->height = tn5250_record_get_byte (This->record);
  TN5250_LOG (("rows = 0x%02X (%d)\n", menubar->height, menubar->height));
  menubar->items = tn5250_record_get_byte (This->record);
  TN5250_LOG (("choices = 0x%02X (%d)\n", menubar->items, menubar->items));
  padding = tn5250_record_get_byte (This->record);
  TN5250_LOG (("padding = 0x%02X (%d)\n", padding, (int) padding));
  separator = tn5250_record_get_byte (This->record);
  TN5250_LOG (("separator = 0x%02X\n", separator));
  selectionchar = tn5250_record_get_byte (This->record);
  TN5250_LOG (("selectionchar = 0x%02X\n", selectionchar));
  cancelaid = tn5250_record_get_byte (This->record);
  TN5250_LOG (("cancelaid = 0x%02X\n", cancelaid));
  length = length - 16;

  if (length == 0)
    {
      return;
    }

  /* The docs are confusing and I'm not sure what to do.  So I'll give up */
  if (usescrollbar)
    {
      TN5250_LOG (("Scroll bars not supported in selection fields\n"));
    }

  if (createnewmenubar)
    {
      menubar->column = tn5250_display_cursor_x (This->display);
      menubar->row = tn5250_display_cursor_y (This->display);

      tn5250_dbuffer_add_menubar (tn5250_display_dbuffer (This->display),
				  menubar);
      tn5250_terminal_create_menubar (This->display->terminal,
				      This->display, menubar);
    }

  menuitemcount = 0;
  while (length > 0)
    {
      minorlength = (int) tn5250_record_get_byte (This->record) - 2;
      length--;
      reserved = tn5250_record_get_byte (This->record);
      length--;

      if (reserved == 0x10)
	{
	  tn5250_session_define_selection_item (This, menubar, minorlength,
						menuitemcount,
						createnewmenubar);
	  menuitemcount++;
	  length = length - minorlength;
	}
      /* Get the Choice Presentation Display Attributes Minor Structure if
       * it is there.
       */
      else if (reserved == 0x01)
	{
	  while (minorlength > 0)
	    {
	      reserved = tn5250_record_get_byte (This->record);
	      TN5250_LOG (("Choice Presentation = 0x%02X\n", reserved));
	      length--;
	      minorlength--;
	    }
	}
      /* Get the Menu Bar Separator Minor Structure if it is there.
       */
      else if (reserved == 0x09)
	{
	  while (minorlength > 0)
	    {
	      reserved = tn5250_record_get_byte (This->record);
	      TN5250_LOG (("Menu Bar Separator = 0x%02X\n", reserved));
	      length--;
	      minorlength--;
	    }
	}
      /* Get the Choice Indicator Minor Structure if it is there.
       */
      else if (reserved == 0x02)
	{
	  while (minorlength > 0)
	    {
	      reserved = tn5250_record_get_byte (This->record);
	      TN5250_LOG (("Choice Indicator = 0x%02X\n", reserved));
	      length--;
	      minorlength--;
	    }
	}
      /* Get the Scroll Bar Indicators Minor Structure if it is there.
       */
      else if (reserved == 0x03)
	{
	  while (minorlength > 0)
	    {
	      reserved = tn5250_record_get_byte (This->record);
	      TN5250_LOG (("Scroll Bar Indicators = 0x%02X\n", reserved));
	      length--;
	      minorlength--;
	    }
	}
      else
	{
	  TN5250_LOG (("unknown data = 0x%02X\n", reserved));
	}
    }
  return;
}


/****i* lib5250/tn5250_session_define_selection_item
 * NAME
 *    tn5250_session_define_selection_item
 * SYNOPSIS
 *    tn5250_session_define_selection_item (This, len)
 * INPUTS
 *    Tn5250Session *      This       -
 *    int                  length     -
 * DESCRIPTION
 *    DOCUMENT ME!!!
 *****/
static void
tn5250_session_define_selection_item (Tn5250Session * This,
				      Tn5250Menubar * menubar, int length,
				      int count, short createnew)
{
  Tn5250Menuitem *menuitem;
  unsigned char flagbyte1;
  unsigned char flagbyte2;
  unsigned char flagbyte3;
  unsigned char reserved;
  short offset_incl = 0;
  short aid_incl = 0;
  short selectchars_incl = 0;
  int i;

  TN5250_LOG (("Entering tn5250_session_define_selection_item()\n"));


  if (createnew)
    {
      menuitem = tn5250_menuitem_new ();
    }
  else
    {
      menuitem = tn5250_menuitem_list_find_by_id (menubar->menuitem_list,
						  count);
    }

  flagbyte1 = tn5250_record_get_byte (This->record);
  length--;

  /* The first two bits choice state */
  if ((flagbyte1 & 0xC0) == 0)
    {
      menuitem->available = 1;
      TN5250_LOG (("Available and not a default selection\n"));
    }

  /* both bits cannot be on */
  if ((flagbyte1 & 0xC0) == 3)
    {
      TN5250_LOG (("Reserved usage of choice state!\n"));
    }
  else
    {
      if (flagbyte1 & 0x40)
	{
	  menuitem->selected = 1;
	  menuitem->available = 1;
	  TN5250_LOG (("Available and is a default selection (selected state)\n"));
	}
      if (flagbyte1 & 0x80)
	{
	  menuitem->selected = 0;
	  menuitem->available = 0;
	  TN5250_LOG (("Not available\n"));
	}
    }

  /* bit 2 specifies menu item that start a new row */
  if (flagbyte1 & 0x20)
    {
      TN5250_LOG (("Menu item starts a new row\n"));
    }

  /* bit 4 indicates mnemonic offset is included */
  if (flagbyte1 & 0x08)
    {
      TN5250_LOG (("mnemonic offset is included\n"));
      offset_incl = 1;
    }

  /* bit 5 specifies an AID if selected is included in this minor structure */
  if (flagbyte1 & 0x04)
    {
      TN5250_LOG (("AID is included\n"));
      aid_incl = 1;
    }

  /* bits 6 and 7 define numeric selection characters */
  if ((flagbyte1 & 0x03) == 0)
    {
      TN5250_LOG (("Numeric selection characters are not included in this minor structure\n"));
    }
  else if ((flagbyte1 & 0x0C) == 1)
    {
      TN5250_LOG (("A single-digit numeric selection character is included in this minor structure\n"));
      selectchars_incl = 1;
    }
  else if ((flagbyte1 & 0x0C) == 2)
    {
      TN5250_LOG (("Double-digit numeric selection characters are included in this minor structure\n"));
      selectchars_incl = 1;
    }
  else
    {
      TN5250_LOG (("Reserved use of numeric selection charaters!!\n"));
    }


  flagbyte2 = tn5250_record_get_byte (This->record);
  length--;

  if (flagbyte2 & 0x80)
    {
      TN5250_LOG (("choice cannot accept a cursor\n"));
    }

  if (flagbyte2 & 0x40)
    {
      TN5250_LOG (("application user desires a roll-down AID if the Cursor Up key is pressed on this choice\n"));
    }

  if (flagbyte2 & 0x20)
    {
      TN5250_LOG (("application user desires a roll-up AID if the Cursor Down key is pressed on this choice\n"));
    }

  if (flagbyte2 & 0x10)
    {
      TN5250_LOG (("application user desires a roll-left AID if the Cursor Left key is pressed on this choice\n"));
    }

  if (flagbyte2 & 0x08)
    {
      TN5250_LOG (("application user desires a roll-right AID if the Cursor Right key is pressed on this choice\n"));
    }

  if (flagbyte2 & 0x04)
    {
      TN5250_LOG (("no push-button box is written for this choice\n"));
    }

  if (flagbyte2 & 0x01)
    {
      TN5250_LOG (("cursor direction is right to left\n"));
    }


  flagbyte3 = tn5250_record_get_byte (This->record);
  length--;

  menuitem->flagbyte1 = flagbyte1;
  menuitem->flagbyte2 = flagbyte2;
  menuitem->flagbyte3 = flagbyte3;

  /* The first three bits cannot all be off */
  if ((flagbyte3 & 0xE0) == 0)
    {
      TN5250_LOG (("Minor structure ignored\n"));

      while (length > 0)
	{
	  reserved = tn5250_record_get_byte (This->record);
	  TN5250_LOG (("ignored = 0x%02X\n", reserved));
	  length--;
	}
      return;
    }

  if (flagbyte3 & 0x80)
    {
      TN5250_LOG (("use this minor structure for GUI devices (including GUI-like NWSs)\n"));
    }

  if (flagbyte3 & 0x40)
    {
      TN5250_LOG (("use this minor structure for non-GUI NWSs that are capable of creating mnemonic underscores\n"));
    }

  if (flagbyte3 & 0x20)
    {
      TN5250_LOG (("use this minor structure for NWS display devices that are not capable of creating underscores\n"));
    }


  if (offset_incl)
    {
      reserved = tn5250_record_get_byte (This->record);
      length--;
      TN5250_LOG (("mnemonic offset: 0x%02X\n", reserved));
    }


  if (aid_incl)
    {
      reserved = tn5250_record_get_byte (This->record);
      length--;
      TN5250_LOG (("AID: 0x%02X\n", reserved));
    }


  if (selectchars_incl)
    {
      reserved = tn5250_record_get_byte (This->record);
      length--;
      TN5250_LOG (("Numeric characters: 0x%02X\n", reserved));
    }

  if (!createnew)
    {
      free (menuitem->text);
    }

  menuitem->text = tn5250_new (unsigned char, menubar->itemsize + 1);

  for (i = 0; (i < menubar->itemsize) && (length > 0); i++)
    {
      menuitem->text[i] = tn5250_record_get_byte (This->record);
      TN5250_LOG (("Choice text = %c\n", tn5250_char_map_to_local
		   (tn5250_display_char_map (This->display),
		    menuitem->text[i])));
      length--;
    }

  /* The size of this menu item is the length of the text plus two
   * attribute characters.
   */
  menuitem->size = i + 2;

  /* fill the rest of the text with nulls since that will allow us to use
   * strlen() on it.
   */
  for (; (i < menubar->itemsize + 1); i++)
    {
      menuitem->text[i] = '\0';
    }

  if (createnew)
    {
      tn5250_menu_add_menuitem (menubar, menuitem);

      /* The row and column must be calculated after adding the menu item to
       * the list.
       */
      menuitem->row = tn5250_menuitem_new_row (menuitem);
      menuitem->column = tn5250_menuitem_new_col (menuitem);
      /*
         menuitem->row = tn5250_display_cursor_y (This->display);
         menuitem->column = tn5250_display_cursor_x (This->display);
       */
    }

  if (createnew)
    {
      tn5250_terminal_create_menuitem (This->display->terminal,
				       This->display, menuitem);
    }

  return;
}


/****i* lib5250/tn5250_session_remove_gui_selection_field
 * NAME
 *    tn5250_session_remove_gui_selection_field
 * SYNOPSIS
 *    tn5250_session_remove_gui_selection_field (This, len)
 * INPUTS
 *    Tn5250Session *      This       -
 *    int                  length     -
 * DESCRIPTION
 *    DOCUMENT ME!!!
 *****/
static void
tn5250_session_remove_gui_selection_field (Tn5250Session * This, int length)
{
  unsigned char flagbyte1;
  unsigned char reserved;

  TN5250_LOG (("Entering tn5250_session_remove_gui_selection_field()\n"));

  flagbyte1 = tn5250_record_get_byte (This->record);
  reserved = tn5250_record_get_byte (This->record);

  if (tn5250_dbuffer_menubar_count (This->display->display_buffers) > 0)
    {
      tn5250_terminal_destroy_menubar (This->display->terminal,
				       This->display,
				       This->display->display_buffers->
				       menubar_list);
      This->display->display_buffers->menubar_list =
	tn5250_menubar_list_destroy (This->display->display_buffers->
				     menubar_list);
      This->display->display_buffers->menubar_count = 0;
    }

  return;
}


/****i* lib5250/tn5250_session_create_window_structured_field
 * NAME
 *    tn5250_session_create_window_structured_field
 * SYNOPSIS
 *    tn5250_session_create_window_structured_field (This, len)
 * INPUTS
 *    Tn5250Session *      This       -
 *    int                  length     -
 * DESCRIPTION
 *    DOCUMENT ME!!!
 *****/
static void
tn5250_session_create_window_structured_field (Tn5250Session * This,
					       int length)
{
  Tn5250Window *window;
  unsigned char flagbyte1;
  unsigned char depth;
  unsigned char width;
  unsigned char reserved;
  int border_length;
  unsigned char border_type, border_flags, border_mono;
  unsigned char border_color, border_upperlchar, border_topchar;
  unsigned char border_upperrchar, border_leftchar, border_rightchar;
  unsigned char border_lowerlchar, border_bottomchar, border_lowerrchar;
  short pulldown = 0;

  TN5250_LOG (("Entering tn5250_session_create_window_structured_field()\n"));


  flagbyte1 = tn5250_record_get_byte (This->record);

  if (flagbyte1 & 0x80)
    {
      TN5250_LOG (("Cursor restricted to window\n"));
    }

  if (flagbyte1 & 0x40)
    {
      /*
      TN5250_LOG (("Pull down window - data ignored (will use selection field data)\n"));
      */
      TN5250_LOG (("Pull down window\n"));
      pulldown = 1;
    }

  reserved = tn5250_record_get_byte (This->record);
  reserved = tn5250_record_get_byte (This->record);
  depth = tn5250_record_get_byte (This->record);
  TN5250_LOG (("depth = 0x%02X (%d)\n", depth, (int) depth));
  width = tn5250_record_get_byte (This->record);
  TN5250_LOG (("width = 0x%02X (%d)\n", width, (int) width));

  /*
  if (!pulldown)
    {
  */
      /*
         if ((window =
         tn5250_window_match_test (This->display->display_buffers->
         window_list,
         tn5250_display_cursor_x (This->display) +
         1,
         tn5250_display_cursor_y (This->display) +
         1, width, depth)) != NULL)
         {
         }
         else
         {
       */
      window = tn5250_window_new ();
      /*
         }
       */
      /*
    }
      */

  if ((length - 5) > 0)
    {
      length = length - 5;
      border_length = tn5250_record_get_byte (This->record);
      TN5250_LOG (("border_length = 0x%02X (%d)\n", border_length,
		   border_length));
      border_length--;
      length--;
      if (border_length > 0)
	{
	  border_type = tn5250_record_get_byte (This->record);
	  TN5250_LOG (("Border type = 0x%02X\n", border_length));
	  border_length--;
	  length--;
	}

      /* Format for Border Presentation Minor Structure */
      if (border_type == 0x01)
	{
	  if (border_length > 0)
	    {
	      border_flags = tn5250_record_get_byte (This->record);
	      if (border_flags & 0x80)
		{
		  TN5250_LOG (("Use border presentation characters (GUI-like NWS)\n"));
		}
	      border_length--;
	      length--;
	    }
	  if (border_length > 0)
	    {
	      border_mono = tn5250_record_get_byte (This->record);
	      TN5250_LOG (("Monochrome border attribute = 0x%02X\n",
			   border_mono));
	      border_length--;
	      length--;
	    }
	  if (border_length > 0)
	    {
	      border_color = tn5250_record_get_byte (This->record);
	      TN5250_LOG (("Color border attribute = 0x%02X\n",
			   border_color));
	      border_length--;
	      length--;
	    }
	  if (border_length > 0)
	    {
	      border_upperlchar = tn5250_record_get_byte (This->record);
	      TN5250_LOG (("Upper left border character = 0x%02X\n",
			   border_upperlchar));
	      border_length--;
	      length--;
	    }
	  if (border_length > 0)
	    {
	      border_topchar = tn5250_record_get_byte (This->record);
	      TN5250_LOG (("Top border character = 0x%02X\n",
			   border_topchar));
	      border_length--;
	      length--;
	    }
	  if (border_length > 0)
	    {
	      border_upperrchar = tn5250_record_get_byte (This->record);
	      TN5250_LOG (("Upper right border character = 0x%02X\n",
			   border_upperrchar));
	      border_length--;
	      length--;
	    }
	  if (border_length > 0)
	    {
	      border_leftchar = tn5250_record_get_byte (This->record);
	      TN5250_LOG (("Left border character = 0x%02X\n",
			   border_leftchar));
	      border_length--;
	      length--;
	    }
	  if (border_length > 0)
	    {
	      border_rightchar = tn5250_record_get_byte (This->record);
	      TN5250_LOG (("Right border character = 0x%02X\n",
			   border_rightchar));
	      border_length--;
	      length--;
	    }
	  if (border_length > 0)
	    {
	      border_lowerlchar = tn5250_record_get_byte (This->record);
	      TN5250_LOG (("Lower left border character = 0x%02X\n",
			   border_lowerlchar));
	      border_length--;
	      length--;
	    }
	  if (border_length > 0)
	    {
	      border_bottomchar = tn5250_record_get_byte (This->record);
	      TN5250_LOG (("Bottom border character = 0x%02X\n",
			   border_bottomchar));
	      border_length--;
	      length--;
	    }
	  if (border_length > 0)
	    {
	      border_lowerrchar = tn5250_record_get_byte (This->record);
	      TN5250_LOG (("Lower right border character = 0x%02X\n",
			   border_lowerrchar));
	      border_length--;
	      length--;
	    }
	}

      /* Format for Window Title/Footer Minor Structure */
      else if (border_type == 0x10)
	{
	  if (border_length > 0)
	    {
	      border_flags = tn5250_record_get_byte (This->record);
	      border_length--;
	      length--;
	    }
	  if (border_length > 0)
	    {
	      border_mono = tn5250_record_get_byte (This->record);
	      border_length--;
	      length--;
	    }
	  if (border_length > 0)
	    {
	      border_color = tn5250_record_get_byte (This->record);
	      border_length--;
	      length--;
	    }
	  if (border_length > 0)
	    {
	      reserved = tn5250_record_get_byte (This->record);
	      border_length--;
	      length--;
	    }
	  while (border_length > 0)
	    {
	      reserved = tn5250_record_get_byte (This->record);
	      border_length--;
	      length--;
	    }
	}
    }

  while (length > 0)
    {
      reserved = tn5250_record_get_byte (This->record);
      length--;
    }

  /*
  if (!pulldown)
    {
  */
      window->width = (int) width;
      window->height = (int) depth;
      window->column = tn5250_display_cursor_x (This->display) + 1;
      window->row = tn5250_display_cursor_y (This->display) + 1;
      TN5250_LOG (("window position: %d, %d\n", window->column, window->row));
      tn5250_dbuffer_add_window (tn5250_display_dbuffer (This->display),
				 window);
      tn5250_terminal_create_window (This->display->terminal, This->display,
				     window);

      /* Forcibly erase the region of the screen that the window covers.  I
       * thought that the iSeries would always send an Erase To Address command
       * after each Create Window command, but that isn't the case.  The weird
       * part is that sometimes we do get the erase command, and sometimes we
       * don't.  I have no idea why.
       */
      tn5250_display_erase_region (This->display, window->row + 1,
				   window->column + 2,
				   window->row + window->height + 1,
				   window->column + window->column + 2,
				   window->column + 2,
				   window->column + window->width + 2);
      /*
    }
      */
  return;
}


/****i* lib5250/tn5250_session_define_scrollbar
 * NAME
 *    tn5250_session_define_scrollbar
 * SYNOPSIS
 *    tn5250_session_define_scrollbar (This, len)
 * INPUTS
 *    Tn5250Session *      This       -
 *    int                  length     -
 * DESCRIPTION
 *    DOCUMENT ME!!!
 *****/
static void
tn5250_session_define_scrollbar (Tn5250Session * This, int length)
{
  Tn5250Scrollbar *scrollbar;
  unsigned char flagbyte1;
  unsigned char reserved;
  unsigned char totalrowscols;
  unsigned char sliderpos;
  unsigned char size;

  TN5250_LOG (("Entering tn5250_session_define_scrollbar()\n"));
  scrollbar = tn5250_scrollbar_new ();

  flagbyte1 = tn5250_record_get_byte (This->record);
  length--;

  if (flagbyte1 & 0x80)
    {
      TN5250_LOG (("Creating horizontal scrollbar\n"));
      scrollbar->direction = 1;
    }
  else
    {
      TN5250_LOG (("Creating vertical scrollbar\n"));
      scrollbar->direction = 0;
    }

  reserved = tn5250_record_get_byte (This->record);
  length--;
  totalrowscols = tn5250_record_get_byte (This->record);
  length--;
  scrollbar->rowscols = (int) totalrowscols;
  TN5250_LOG (("Total rows/columns that can be scrolled: %d\n",
	       (int) totalrowscols));
  sliderpos = tn5250_record_get_byte (This->record);
  length--;
  scrollbar->sliderpos = (int) sliderpos;
  TN5250_LOG (("Slider position: %d\n", (int) sliderpos));

  if (length > 0)
    {
      size = tn5250_record_get_byte (This->record);
      length--;
      scrollbar->size = (int) size;
      TN5250_LOG (("Scrollbar size: %d\n", (int) size));
    }

  while (length > 0)
    {
      reserved = tn5250_record_get_byte (This->record);
      length--;
    }
  /*
     scrollbar5250.direction = This->display->terminal->scrollbar.direction;
     scrollbar5250.rowscols = This->display->terminal->scrollbar.rowscols;
     scrollbar5250.sliderpos = This->display->terminal->scrollbar.sliderpos;
     scrollbar5250.size = This->display->terminal->scrollbar.size;
   */
  tn5250_dbuffer_add_scrollbar (tn5250_display_dbuffer (This->display),
				scrollbar);
  tn5250_terminal_create_scrollbar (This->display->terminal, This->display,
				    scrollbar);
  return;
}


/****i* lib5250/tn5250_session_remove_gui_window_structured_field
 * NAME
 *    tn5250_session_remove_gui_window_structured_field
 * SYNOPSIS
 *    tn5250_session_remove_gui_window_structured_field (This, len)
 * INPUTS
 *    Tn5250Session *      This       -
 *    int                  length     -
 * DESCRIPTION
 *    DOCUMENT ME!!!
 *****/
static void
tn5250_session_remove_gui_window_structured_field (Tn5250Session * This,
						   int length)
{
  Tn5250Window *window;
  unsigned char flagbyte1;
  unsigned char flagbyte2;
  unsigned char reserved;

  TN5250_LOG (("Entering tn5250_session_remove_gui_window_structured_field()\n"));

  flagbyte1 = tn5250_record_get_byte (This->record);
  flagbyte2 = tn5250_record_get_byte (This->record);
  reserved = tn5250_record_get_byte (This->record);

  if (This->display->display_buffers->window_count > 0)
    {
      window =
	tn5250_window_hit_test (This->display->display_buffers->window_list,
				tn5250_display_cursor_x (This->display),
				tn5250_display_cursor_y (This->display));
      tn5250_terminal_destroy_window (This->display->terminal, This->display,
				      window);
      This->display->display_buffers->window_count--;

      if (This->display->display_buffers->window_count == 0)
	{
	  This->display->display_buffers->window_list =
	    tn5250_window_list_destroy (This->display->display_buffers->
					window_list);
	}
    }

  if (This->display->display_buffers->scrollbar_count > 0)
    {
      tn5250_terminal_destroy_scrollbar (This->display->terminal,
					 This->display);
      This->display->display_buffers->scrollbar_list =
	tn5250_scrollbar_list_destroy (This->display->display_buffers->
				       scrollbar_list);
      This->display->display_buffers->scrollbar_count = 0;
    }

  return;
}


/****i* lib5250/tn5250_session_remove_all_gui_constructs_structured_field
 * NAME
 *    tn5250_session_remove_all_gui_constructs_structured_field
 * SYNOPSIS
 *    tn5250_session_remove_all_gui_constructs_structured_field (This, len)
 * INPUTS
 *    Tn5250Session *      This       -
 *    int                  length     -
 * DESCRIPTION
 *    DOCUMENT ME!!!
 *****/
static void
tn5250_session_remove_all_gui_constructs_structured_field (Tn5250Session *
							   This, int length)
{
  unsigned char flagbyte1;
  unsigned char flagbyte2;
  unsigned char reserved;

  TN5250_LOG (("Entering tn5250_session_remove_all_gui_constructs_structured_field()\n"));

  flagbyte1 = tn5250_record_get_byte (This->record);
  flagbyte2 = tn5250_record_get_byte (This->record);
  reserved = tn5250_record_get_byte (This->record);

  if (This->display->display_buffers->window_count > 0)
    {
      Tn5250Window *iter, *next;

      if ((iter = This->display->display_buffers->window_list) != NULL)
	{
	  do
	    {
	      next = iter->next;
	      TN5250_LOG (("destroying window id: %d\n", iter->id));
	      tn5250_terminal_destroy_window (This->display->terminal,
					      This->display, iter);
	      iter = next;
	    }
	  while (iter != This->display->display_buffers->window_list);
	}

      This->display->display_buffers->window_list =
	tn5250_window_list_destroy (This->display->display_buffers->
				    window_list);
      This->display->display_buffers->window_count = 0;
    }

  if (This->display->display_buffers->scrollbar_count > 0)
    {
      tn5250_terminal_destroy_scrollbar (This->display->terminal,
					 This->display);
      This->display->display_buffers->scrollbar_list =
	tn5250_scrollbar_list_destroy (This->display->display_buffers->
				       scrollbar_list);
      This->display->display_buffers->scrollbar_count = 0;
    }

  return;
}


/****i* lib5250/tn5250_session_write_data_structured_field
 * NAME
 *    tn5250_session_write_data_structured_field
 * SYNOPSIS
 *    tn5250_session_write_data_structured_field (This, len)
 * INPUTS
 *    Tn5250Session *      This       -
 *    int                  length     -
 * DESCRIPTION
 *    DOCUMENT ME!!!
 *****/
static void
tn5250_session_write_data_structured_field (Tn5250Session * This, int length)
{
  Tn5250Field *field = tn5250_display_current_field (This->display);
  unsigned char *data, *ptr;
  int datalength;
  unsigned char flagbyte1;
  unsigned char c;

  TN5250_LOG (("Entering tn5250_session_write_data_structured_field()\n"));

  flagbyte1 = tn5250_record_get_byte (This->record);
  length--;

  if (flagbyte1 & 0x80)
    {
      TN5250_LOG (("Write data to entry field\n"));
    }

  data = (unsigned char *) malloc (length * sizeof (unsigned char));
  ptr = data;
  datalength = length;

  TN5250_LOG (("Data: "));
  while (length > 0)
    {
      c = tn5250_record_get_byte (This->record);
      memcpy (ptr, &c, sizeof (unsigned char));
      ptr = ptr + sizeof (unsigned char);
      TN5250_LOG (("%c", tn5250_char_map_to_local (This->display->map, c)));
      length--;
    }
  TN5250_LOG (("\n"));

  if ((flagbyte1 & 0x80) && (tn5250_field_is_wordwrap (field)))
    {
      tn5250_display_wordwrap (This->display, data, datalength,
			       tn5250_field_length (field), field);
    }

  free (data);
  return;
}
