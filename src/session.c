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

#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>

#include "utility.h"
#include "buffer.h"
#include "dbuffer.h"
#include "record.h"
#include "stream.h"
#include "field.h"
#include "terminal.h"
#include "session.h"
#include "codes5250.h"
#include "display.h"

static void tn5250_session_handle_keys(Tn5250Session * This);
static void tn5250_session_handle_receive(Tn5250Session * This);
static void tn5250_session_invite(Tn5250Session * This);
static void tn5250_session_cancel_invite(Tn5250Session * This);
static void tn5250_session_send_fields(Tn5250Session * This, int aidcode);
static void tn5250_session_send_field(Tn5250Session *This, Tn5250Buffer *buf,
                                      Tn5250Field *field);
static void tn5250_session_process_stream(Tn5250Session * This);
static void tn5250_session_write_error_code(Tn5250Session * This);
static void tn5250_session_write_to_display(Tn5250Session * This);
static void tn5250_session_clear_unit(Tn5250Session * This);
static void tn5250_session_clear_unit_alternate(Tn5250Session * This);
static void tn5250_session_clear_format_table(Tn5250Session * This);
static void tn5250_session_read_immediate(Tn5250Session * This);
static void tn5250_session_home(Tn5250Session * This);
static void tn5250_session_print(Tn5250Session * This);
static void tn5250_session_output_only(Tn5250Session * This);
static void tn5250_session_save_screen(Tn5250Session * This);
static void tn5250_session_message_on(Tn5250Session * This);
static void tn5250_session_message_off(Tn5250Session * This);
static void tn5250_session_roll(Tn5250Session * This);
static void tn5250_session_start_of_field(Tn5250Session * This);
static void tn5250_session_start_of_header(Tn5250Session * This);
static void tn5250_session_set_buffer_address(Tn5250Session * This);
static void tn5250_session_repeat_to_address(Tn5250Session * This);
static void tn5250_session_insert_cursor(Tn5250Session * This);
static void tn5250_session_write_structured_field(Tn5250Session * This);
static void tn5250_session_read_screen_immediate(Tn5250Session * This);
static void tn5250_session_read_input_fields(Tn5250Session * This);
static void tn5250_session_read_mdt_fields(Tn5250Session * This);
static int tn5250_session_valid_wtd_data_char (unsigned char c);
static int tn5250_session_handle_aidkey (Tn5250Session *This, int key);
static void tn5250_session_handle_cc1 (Tn5250Session *This, unsigned char cc1);
static void tn5250_session_handle_cc2 (Tn5250Session *This, unsigned char cc2);

Tn5250Session *tn5250_session_new()
{
   Tn5250Session *This;
   int n;

   This = tn5250_new(Tn5250Session, 1);
   if (This == NULL)
      return NULL;

   This->record = tn5250_record_new();
   if (This->record == NULL) {
      free (This);
      return NULL;
   }

   This->stream = NULL;
   This->invited = 1;
   This->read_opcode = 0;
   This->key_queue_head = This->key_queue_tail = 0;

   This->handle_aidkey = tn5250_session_handle_aidkey;
   This->display = NULL;
   return This;
}

void tn5250_session_destroy(Tn5250Session * This)
{
   int n;

   if (This->stream != NULL)
      tn5250_stream_destroy(This->stream);
   if (This->record != NULL)
      tn5250_record_destroy(This->record);
   free (This);
}

void tn5250_session_set_stream(Tn5250Session * This, Tn5250Stream * newstream)
{
   if ((This->stream = newstream) != NULL)
      tn5250_display_update(This->display);
}

void tn5250_session_main_loop(Tn5250Session * This)
{
   int r;
   int is_x_system;
   int handled_key = 0;

   while (1) {
      is_x_system = (tn5250_display_indicators(This->display) & 
	 TN5250_DISPLAY_IND_X_SYSTEM) != 0;

      /* Handle keys from our key queue if we aren't X SYSTEM. */
      if (This->key_queue_head != This->key_queue_tail && !is_x_system) {
	 TN5250_LOG (("Handling buffered key.\n"));
	 tn5250_display_do_key(This->display,
	       This->key_queue[This->key_queue_head]);
	 if (++ This->key_queue_head == TN5250_SESSION_KEY_QUEUE_SIZE)
	    This->key_queue_head = 0;
	 handled_key = 1;
	 continue;
      }

      if (handled_key)
	 tn5250_display_update(This->display);
      r = tn5250_display_waitevent(This->display);
      if ((r & TN5250_TERMINAL_EVENT_KEY) != 0)
	 tn5250_session_handle_keys(This);
      if ((r & TN5250_TERMINAL_EVENT_QUIT) != 0)
	 return;
      if ((r & TN5250_TERMINAL_EVENT_DATA) != 0) {
	 if (!tn5250_stream_handle_receive(This->stream))
	    return;
	 tn5250_session_handle_receive(This);
      }
   }
}

/*
 *    Handle keys from the terminal until we run out or are in the X SYSTEM
 *    state.
 */
static void tn5250_session_handle_keys(Tn5250Session *This)
{
   int cur_key;

   do {
      cur_key = tn5250_display_getkey (This->display);

      if (cur_key != -1) {
	 if ((tn5250_display_indicators(This->display) & TN5250_DISPLAY_IND_X_SYSTEM) != 0) {
	    /* We can handle system request here. */
	    if (cur_key == K_SYSREQ || cur_key == K_RESET) {
	       /* Flush the keyboard queue. */
	       This->key_queue_head = This->key_queue_tail = 0;
	       tn5250_display_do_key (This->display, cur_key);
	       break;
	    }

	    if ((This->key_queue_tail + 1 == This->key_queue_head) ||
		  (This->key_queue_head == 0 &&
		   This->key_queue_tail == TN5250_SESSION_KEY_QUEUE_SIZE - 1)) {
	       tn5250_display_beep (This->display);
	    }
	    This->key_queue[This->key_queue_tail] = cur_key;
	    if (++ This->key_queue_tail == TN5250_SESSION_KEY_QUEUE_SIZE)
	       This->key_queue_tail = 0;
	 } else {
	    /* We shouldn't ever be handling a key here if there are keys in the queue. */
	    TN5250_ASSERT(This->key_queue_head == This->key_queue_tail);
	    tn5250_display_do_key (This->display, cur_key);
	 }
      }
   } while (cur_key != -1);

   tn5250_display_update(This->display);
}

/* 
 *    Tell the socket to receive as much data as possible, then, if there are
 *      full packets to handle, handle them.
 */
static void tn5250_session_handle_receive(Tn5250Session * This)
{
   int atn;
   int cur_opcode;

   TN5250_LOG(("HandleReceive: entered.\n"));
   while (tn5250_stream_record_count(This->stream) > 0) {
      if (This->record != NULL)
	 tn5250_record_destroy(This->record);
      This->record = tn5250_stream_get_record(This->stream);
      cur_opcode = tn5250_record_opcode(This->record);
      atn = tn5250_record_attention(This->record);

      TN5250_LOG(("HandleReceive: cur_opcode = 0x%02X %d\n", cur_opcode,
	   atn));

      switch (cur_opcode) {
      case TN5250_RECORD_OPCODE_PUT_GET:
      case TN5250_RECORD_OPCODE_INVITE:
	 tn5250_session_invite(This);
	 break;

      case TN5250_RECORD_OPCODE_OUTPUT_ONLY:
	 tn5250_session_output_only(This);
	 break;

      case TN5250_RECORD_OPCODE_CANCEL_INVITE:
	 tn5250_session_cancel_invite(This);
	 break;

      case TN5250_RECORD_OPCODE_MESSAGE_ON:
	 tn5250_session_message_on(This);
	 break;

      case TN5250_RECORD_OPCODE_MESSAGE_OFF:
	 tn5250_session_message_off(This);
	 break;

      case TN5250_RECORD_OPCODE_NO_OP:
      case TN5250_RECORD_OPCODE_SAVE_SCR:
      case TN5250_RECORD_OPCODE_RESTORE_SCR:
      case TN5250_RECORD_OPCODE_READ_IMMED:
      case TN5250_RECORD_OPCODE_READ_SCR:
	 break;

      default:
	 TN5250_LOG(("Error: unknown opcode %2.2X\n", cur_opcode));
	 TN5250_ASSERT(0);
      }

      if (!tn5250_record_is_chain_end(This->record))
	 tn5250_session_process_stream(This);
   }
}

static void tn5250_session_invite(Tn5250Session * This)
{
   TN5250_LOG(("Invite: entered.\n"));
   This->invited = 1;
   tn5250_display_indicator_clear(This->display, TN5250_DISPLAY_IND_X_CLOCK);
   tn5250_display_update(This->display);
}

static void tn5250_session_cancel_invite(Tn5250Session * This)
{
   TN5250_LOG(("CancelInvite: entered.\n"));
   tn5250_display_indicator_set(This->display, TN5250_DISPLAY_IND_X_CLOCK);
   tn5250_display_update(This->display);
   tn5250_stream_send_packet(This->stream, 0, TN5250_RECORD_FLOW_DISPLAY, TN5250_RECORD_H_NONE,
			     TN5250_RECORD_OPCODE_CANCEL_INVITE, NULL);
   This->invited = 0;
}

static void tn5250_session_send_fields(Tn5250Session * This, int aidcode)
{
   int temp;
   Tn5250Buffer field_buf;
   Tn5250Field *field;
   Tn5250DBuffer *dbuffer;
   int X, Y, size;

   X = tn5250_display_cursor_x(This->display);
   Y = tn5250_display_cursor_y(This->display);

   dbuffer = tn5250_display_dbuffer (This->display);
   TN5250_ASSERT (dbuffer != NULL);
   TN5250_LOG(("SendFields: Number of fields: %d\n",
	    tn5250_dbuffer_field_count(dbuffer)));

   tn5250_buffer_init(&field_buf);
   tn5250_buffer_append_byte(&field_buf, Y + 1);
   tn5250_buffer_append_byte(&field_buf, X + 1);

   /* We can have an aidcode of 0 if we are doing a Read Immediate */
   tn5250_buffer_append_byte(&field_buf, aidcode);

   TN5250_LOG(("SendFields: row = %d; col = %d; aid = 0x%02x\n", Y, X, aidcode));

   /* FIXME: Implement field resequencing. */
   switch (This->read_opcode) {
   case CMD_READ_INPUT_FIELDS:
      TN5250_ASSERT(aidcode != 0);
      if (tn5250_dbuffer_mdt(dbuffer) 
	    && tn5250_dbuffer_send_data_for_aid_key(dbuffer,aidcode)) {
	 field = dbuffer->field_list;
	 if (field != NULL) {
	    do {
	       tn5250_session_send_field (This, &field_buf, field);
	       field = field->next;
	    } while (field != dbuffer->field_list);
	 }
      }
      break;

   case CMD_READ_IMMEDIATE:
      if (tn5250_dbuffer_mdt(dbuffer)) {
	 field = dbuffer->field_list;
	 if (field != NULL) {
	    do {
	       tn5250_session_send_field (This, &field_buf, field);
	       field = field->next;
	    } while (field != dbuffer->field_list);
	 }
      }
      break;

   case CMD_READ_MDT_FIELDS:
      TN5250_ASSERT(aidcode != 0);
      if (tn5250_dbuffer_send_data_for_aid_key(dbuffer,aidcode)) {
	 field = dbuffer->field_list;
	 if (field != NULL) {
	    do {
	       if (tn5250_field_mdt(field))
		  tn5250_session_send_field (This, &field_buf, field);
	       field = field->next;
	    } while (field != dbuffer->field_list);
	 }
      }
      break;

   default: /* Huh? */
      TN5250_LOG(("BUG!!! Trying to transmit fields when This->read_opcode = 0x%02X.\n",
	       This->read_opcode));
      TN5250_ASSERT(0);
   }

   This->read_opcode = 0; /* No longer in a read command. */

   tn5250_display_indicator_clear(This->display, TN5250_DISPLAY_IND_INSERT);
   tn5250_display_update(This->display);

   tn5250_stream_send_packet(This->stream,
	 tn5250_buffer_length(&field_buf),
	 TN5250_RECORD_FLOW_DISPLAY,
	 TN5250_RECORD_H_NONE,
	 TN5250_RECORD_OPCODE_PUT_GET,
	 tn5250_buffer_data(&field_buf));
   tn5250_buffer_free(&field_buf);
}

/*
 *    Append a single field to the transmit buffer in the manner required
 *    by the current read opcode.
 */
static void tn5250_session_send_field (Tn5250Session * This, Tn5250Buffer *buf, Tn5250Field *field)
{
   int size, n;
   unsigned char *data;
   unsigned char c;

   size = tn5250_field_length(field);
   data = tn5250_display_field_data (This->display, field);

   TN5250_LOG(("Sending:\n"));
   tn5250_field_dump(field);

   switch (This->read_opcode) {
   case CMD_READ_INPUT_FIELDS:
   case CMD_READ_IMMEDIATE:
      if (tn5250_field_is_signed_num (field)) {
	 for (n = 0; n < size - 1; n++)
	    tn5250_buffer_append_byte(buf, data[n] == 0 ? 0x40 : data[n]);
	 c = data[size-2];
	 tn5250_buffer_append_byte(buf,
	       tn5250_ebcdic2ascii(data[size-1]) == '-' ?
	       (0xd0 | (0x0f & c)) : c);
      } else {
	 for (n = 0; n < size; n++)
	    tn5250_buffer_append_byte(buf, data[n] == 0 ? 0x40 : data[n]);
      }
      break;

   case CMD_READ_MDT_FIELDS:
      tn5250_buffer_append_byte(buf, SBA);
      tn5250_buffer_append_byte(buf, tn5250_field_start_row(field) + 1);
      tn5250_buffer_append_byte(buf, tn5250_field_start_col(field) + 1);

      /* For signed numeric fields, if the second-last character is a digit
       * and the last character is a '-', zone shift the second-last char.
       * In any case, don't send the sign position. */
      c = data[size-1];
      if (tn5250_field_is_signed_num (field)) {
	 size--;
	 c = size > 0 ? data[size-1] : 0;
	 if (size > 1 && data[size] == tn5250_ascii2ebcdic('-') &&
	       isdigit (tn5250_ebcdic2ascii (c)))
	    c = (0xd0 | (0x0f & c));
      }
      
      /* Strip trailing NULs */
      while (size > 0 && data[size-1] == 0) {
	 size--;
	 c = size > 0 ? data[size-1] : 0;
      }

      /* Send all but the last character, then send the last character.
       * This is because we don't want to modify the display buffer's data */
      for (n = 0; n < size - 1; n++)
	 tn5250_buffer_append_byte(buf, data[n] == 0 ? 0x40 : data[n]);
      if (size > 0)
	 tn5250_buffer_append_byte(buf, c == 0 ? 0x40 : c);
      break;
   }
}

static void tn5250_session_process_stream(Tn5250Session * This)
{
   int cur_command;

   TN5250_LOG(("ProcessStream: entered.\n"));
   tn5250_display_clear_pending_insert (This->display);
   while (!tn5250_record_is_chain_end(This->record)) {

      cur_command = tn5250_record_get_byte(This->record);
      if (cur_command != ESC) {
	 TN5250_LOG (("cur_command != ESC; cur_pos = %d\n",
		  This->record->cur_pos));
	 TN5250_ASSERT(0);
      }
      cur_command = tn5250_record_get_byte(This->record);
      TN5250_LOG(("ProcessStream: cur_command = 0x%02X\n", cur_command));

      switch (cur_command) {
      case CMD_WRITE_TO_DISPLAY:
	 tn5250_session_write_to_display(This);
	 break;
      case CMD_CLEAR_UNIT:
	 tn5250_session_clear_unit(This);
	 break;
      case CMD_CLEAR_UNIT_ALTERNATE:
	 tn5250_session_clear_unit_alternate(This);
	 break;
      case CMD_CLEAR_FORMAT_TABLE:
	 tn5250_session_clear_format_table(This);
	 break;
      case CMD_READ_MDT_FIELDS:
	 tn5250_session_read_mdt_fields(This);
	 break;
      case CMD_READ_IMMEDIATE:
	 tn5250_session_read_immediate(This);
	 break;
      case CMD_READ_INPUT_FIELDS:
	 tn5250_session_read_input_fields(This);
	 break;
      case CMD_READ_SCREEN_IMMEDIATE:
	 tn5250_session_read_screen_immediate(This);
	 break;
      case CMD_WRITE_STRUCTURED_FIELD:
	 tn5250_session_write_structured_field(This);
	 break;
      case CMD_SAVE_SCREEN:
	 tn5250_session_save_screen(This);
	 break;
      case CMD_RESTORE_SCREEN:
	 /* Ignored, the data following this should be a valid
	  * Write To Display command. */
	 break;
      case CMD_WRITE_ERROR_CODE:
	 tn5250_session_write_error_code(This);
	 break;
      case CMD_ROLL:
	 tn5250_session_roll(This);
	 break;
      default:
	 TN5250_LOG(("Error: Unknown command 0x%02X.\n", cur_command));
	 TN5250_ASSERT(0);
      }
   }
}

static void tn5250_session_write_error_code(Tn5250Session * This)
{
   unsigned char cur_order;
   int done;
   int curX, curY;

   TN5250_LOG(("WriteErrorCode: entered.\n"));

   curX = tn5250_display_cursor_x(This->display);
   curY = tn5250_display_cursor_y(This->display);

   /* FIXME: Use same code as WTD for writing data characters to display
    * (?) */
   /* FIXME: Use the correct message line? */

   tn5250_display_set_cursor(This->display, 23, 0);
   done = 0;
   while (!done) {
      if (tn5250_record_is_chain_end(This->record))
	 done = 1;
      else {
	 cur_order = tn5250_record_get_byte(This->record);
#ifndef NDEBUG
	 if (cur_order > 0 && cur_order < 0x40)
	    TN5250_LOG(("\n"));
#endif				/* NDEBUG */
	 if (cur_order == IC)
	    tn5250_session_insert_cursor(This);
	 else if (cur_order == ESC) {
	    done = 1;
	    tn5250_record_unget_byte(This->record);
	 } else if (tn5250_printable(cur_order))
	    tn5250_display_addch(This->display, cur_order);
	 else {
	    TN5250_LOG(("Error: Unknown order -- %2.2X --\n", cur_order));
	    TN5250_ASSERT(0);
	 }
      }
   }
   TN5250_LOG(("\n"));
   tn5250_display_set_cursor(This->display, curY, curX);
   tn5250_display_inhibit(This->display);
   tn5250_display_update(This->display);
}

/*
 *    Process the first byte of the CC from the WTD command.
 */
static void tn5250_session_handle_cc1 (Tn5250Session *This, unsigned char cc1)
{
   int lock_kb = 1;
   int reset_non_bypass_mdt = 0;
   int reset_all_mdt = 0;
   int null_non_bypass_mdt = 0;
   int null_non_bypass = 0;
   Tn5250Field *iter;

   switch (cc1 & 0xE0) {
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

   if (lock_kb) {
      TN5250_LOG(("tn5250_session_handle_cc1: Locking keyboard.\n"));
      tn5250_display_indicator_set(This->display, TN5250_DISPLAY_IND_X_SYSTEM);
   }
   TN5250_ASSERT(This->display != NULL && tn5250_display_dbuffer (This->display) != NULL);
   if ((iter = tn5250_display_dbuffer(This->display)->field_list) != NULL) {
      do {
	 if (!tn5250_field_is_bypass (iter)) {
	    if ((null_non_bypass_mdt && tn5250_field_mdt(iter)) || null_non_bypass) {
	       unsigned char *data;
	       data = tn5250_display_field_data (This->display, iter);
	       memset (data, 0, tn5250_field_length (iter));
	    }
	 }
	 if (reset_all_mdt || (reset_non_bypass_mdt && !tn5250_field_is_bypass (iter)))
	    tn5250_field_clear_mdt (iter);
	 iter = iter->next;
      } while (iter != This->display->display_buffers->field_list);
   }
}

static void tn5250_session_write_to_display(Tn5250Session * This)
{
   unsigned char cur_order;
   unsigned char CC1;
   unsigned char CC2;
   int done;
   Tn5250Field *last_field = NULL;

   TN5250_LOG(("WriteToDisplay: entered.\n"));

   CC1 = tn5250_record_get_byte(This->record);
   CC2 = tn5250_record_get_byte(This->record);
   TN5250_LOG(("WriteToDisplay: 0x%02X:0x%02X\n", CC1, CC2));

   tn5250_session_handle_cc1 (This, CC1);

   done = 0;
   while (!done) {
      if (tn5250_record_is_chain_end(This->record))
	 done = 1;
      else {
	 cur_order = tn5250_record_get_byte(This->record);
#ifndef NDEBUG
	 if (cur_order > 0 && cur_order < 0x40)
	    TN5250_LOG(("\n"));
#endif
	 switch (cur_order) {
	 case (IC):
	    tn5250_session_insert_cursor(This);
	    break;
	 case (RA):
	    tn5250_session_repeat_to_address(This);
	    break;
	 case (SBA):
	    tn5250_session_set_buffer_address(This);
	    break;
	 case (SF):
	    tn5250_session_start_of_field(This);
	    break;
	 case (SOH):
	    tn5250_session_start_of_header(This);
	    break;
	 case (ESC):
	    done = 1;
	    tn5250_record_unget_byte(This->record);
	    break;
	 default:
	    if (tn5250_printable(cur_order)) {
	       tn5250_display_addch(This->display, cur_order);
#ifndef NDEBUG
	       if (tn5250_attribute(cur_order)) {
		  TN5250_LOG(("(0x%02X) ", cur_order));
	       } else {
		  TN5250_LOG(("%c (0x%02X) ", tn5250_ebcdic2ascii(cur_order),
		       cur_order));
	       }
#endif
	    } else {
	       TN5250_LOG(("Error: Unknown order -- %2.2X --\n", cur_order));
	       TN5250_ASSERT(0);
	    }
	 }			/* end switch */
      }				/* end else */
   }				/* end while */
   TN5250_LOG(("\n"));

   /* FIXME: This code is pretty much a duplicate of tn5250_display_kf_home()*/
   if (tn5250_display_pending_insert (This->display))
      tn5250_display_set_cursor_home(This->display);
   else {
      /* Set the cursor to the first non-bypass field, if there is one.  If
       * not, set cursor position to 0,0 */
      Tn5250Field *field = This->display->display_buffers->field_list;
      done = 0;
      if (field != NULL) {
	 do {
	    if (!tn5250_field_is_bypass (field)) {
	       tn5250_display_set_cursor_field (This->display, field);
	       done = 1;
	       break;
	    }
	    field = field->next;
	 } while (field != This->display->display_buffers->field_list);
      }
      if (!done)
	 tn5250_display_set_cursor(This->display, 0, 0);
   }

   tn5250_session_handle_cc2 (This, CC2);
   tn5250_display_update(This->display);
}

static void tn5250_session_handle_cc2 (Tn5250Session *This, unsigned char CC2)
{
   if (CC2 & TN5250_SESSION_CTL_MESSAGE_ON)
      tn5250_display_indicator_set(This->display, TN5250_DISPLAY_IND_MESSAGE_WAITING);
   if ((CC2 & TN5250_SESSION_CTL_MESSAGE_OFF) && !(CC2 & TN5250_SESSION_CTL_MESSAGE_ON))
      tn5250_display_indicator_clear(This->display, TN5250_DISPLAY_IND_MESSAGE_WAITING);

   if ((CC2 & TN5250_SESSION_CTL_CLR_BLINK) != 0 && (CC2 & TN5250_SESSION_CTL_SET_BLINK) == 0) {
      /* FIXME: Hand off to terminal */
   }

   if ((CC2 & TN5250_SESSION_CTL_SET_BLINK) != 0) {
      /* FIXME: Hand off to terminal */
   }

   if ((CC2 & TN5250_SESSION_CTL_ALARM) != 0)
      tn5250_display_beep (This->display);
   if ((CC2 & TN5250_SESSION_CTL_UNLOCK) != 0)
      tn5250_display_indicator_clear(This->display, TN5250_DISPLAY_IND_X_SYSTEM);
}

static void tn5250_session_clear_unit(Tn5250Session * This)
{
   TN5250_LOG(("ClearUnit: entered.\n"));

   tn5250_display_clear_unit(This->display);
   This->read_opcode = 0;
}

static void tn5250_session_clear_unit_alternate(Tn5250Session * This)
{
   unsigned char c;
   TN5250_LOG(("tn5250_session_clear_unit_alternate entered.\n"));
   c = tn5250_record_get_byte(This->record);
   TN5250_LOG(("tn5250_session_clear_unit_alternate, parameter is 0x%02X.\n",
	(int) c));
   tn5250_display_clear_unit_alternate(This->display);
   This->read_opcode = 0;
}

static void tn5250_session_clear_format_table(Tn5250Session * This)
{
   TN5250_LOG(("ClearFormatTable: entered.\n"));
   tn5250_display_clear_format_table(This->display);
   This->read_opcode = 0;
}

static void tn5250_session_read_immediate(Tn5250Session * This)
{
   int old_opcode;

   TN5250_LOG(("ReadImmediate: entered.\n"));
   old_opcode = This->read_opcode;
   This->read_opcode = CMD_READ_IMMEDIATE;
   tn5250_session_send_fields(This, 0);
   This->read_opcode = old_opcode;
}

static int tn5250_session_handle_aidkey (Tn5250Session *This, int key)
{
   Tn5250Buffer buf;

   switch (key) {
   case TN5250_SESSION_AID_PRINT:
   case TN5250_SESSION_AID_RECORD_BS:
      tn5250_buffer_init (&buf);
      tn5250_buffer_append_byte (&buf,
	    tn5250_display_cursor_y(This->display) + 1);
      tn5250_buffer_append_byte (&buf,
	    tn5250_display_cursor_x(This->display) + 1);
      tn5250_buffer_append_byte (&buf, key);
      tn5250_stream_send_packet(This->stream, tn5250_buffer_length(&buf), 
                             TN5250_RECORD_FLOW_DISPLAY,
                             TN5250_RECORD_H_NONE,
                             TN5250_RECORD_OPCODE_NO_OP, 
                             tn5250_buffer_data(&buf));
      tn5250_buffer_free (&buf);
      break;

   case TN5250_SESSION_AID_SYSREQ:
      This->read_opcode = 0; /* We are out of the read */
      tn5250_stream_send_packet(This->stream, 0, TN5250_RECORD_FLOW_DISPLAY,
	    TN5250_RECORD_H_SRQ, TN5250_RECORD_OPCODE_NO_OP, NULL);
      break;

   case TN5250_SESSION_AID_ATTN:
      This->read_opcode = 0; /* We are out of the read. */
      tn5250_stream_send_packet(This->stream, 0, TN5250_RECORD_FLOW_DISPLAY,
	    TN5250_RECORD_H_ATN, TN5250_RECORD_OPCODE_NO_OP, NULL);
      break;

   default:
      tn5250_session_send_fields (This, key);
      tn5250_display_inhibit (This->display);
      break;
   }

   return 1; /* Continue processing. */
}

/*
 *    I'm not sure what the actual behavior of this opcode is supposed 
 *      to be.  I'm just sort-of fudging it based on empirical testing.
 */
static void tn5250_session_output_only(Tn5250Session * This)
{
   unsigned char temp[2];

   TN5250_LOG(("OutputOnly: entered.\n"));

   /* 
      We get this if the user picks something they shouldn't from the
      System Request Menu - such as transfer to previous system.
    */
   if (tn5250_record_sys_request(This->record)) {
      temp[0] = tn5250_record_get_byte(This->record);
      temp[1] = tn5250_record_get_byte(This->record);
      TN5250_LOG(("OutputOnly: ?? = 0x%02X; ?? = 0x%02X\n", temp[0], temp[1]));
   } else {
      /*  
         Otherwise it seems to mean the Attention key menu is being 
         displayed.
       */
   }
}

static void tn5250_session_save_screen(Tn5250Session * This)
{
   Tn5250Buffer buffer;

   TN5250_LOG(("SaveScreen: entered.\n"));

   tn5250_buffer_init (&buffer);
   tn5250_display_make_wtd_data (This->display, &buffer, NULL);

   /* Okay, now if we were in a Read MDT Fields or a Read Input Fields,
    * we need to append a command which would put us back in the appropriate
    * read. */
   if (This->read_opcode != 0) {
      tn5250_buffer_append_byte (&buffer, ESC);
      tn5250_buffer_append_byte (&buffer, This->read_opcode);
      tn5250_buffer_append_byte (&buffer, 0x00);  /* FIXME: ? CC1 */
      tn5250_buffer_append_byte (&buffer, 0x00);  /* FIXME: ? CC2 */
   }

   tn5250_stream_send_packet (This->stream, tn5250_buffer_length (&buffer),
	 TN5250_RECORD_FLOW_DISPLAY, TN5250_RECORD_H_NONE,
	 TN5250_RECORD_OPCODE_SAVE_SCR, tn5250_buffer_data (&buffer));
   tn5250_buffer_free (&buffer);
}

static void tn5250_session_message_on(Tn5250Session * This)
{
   TN5250_LOG(("MessageOn: entered.\n"));
   tn5250_display_indicator_set(This->display, TN5250_DISPLAY_IND_MESSAGE_WAITING);
   tn5250_display_update(This->display);
}

static void tn5250_session_message_off(Tn5250Session * This)
{
   TN5250_LOG(("MessageOff: entered.\n"));
   tn5250_display_indicator_clear(This->display,
				  TN5250_DISPLAY_IND_MESSAGE_WAITING);
   tn5250_display_update(This->display);
}

static void tn5250_session_roll(Tn5250Session * This)
{
   unsigned char direction, top, bot;
   int lines;

   direction = tn5250_record_get_byte(This->record);
   top = tn5250_record_get_byte(This->record);
   bot = tn5250_record_get_byte(This->record);

   TN5250_LOG(("Roll: direction = 0x%02X; top = %d; bottom = %d\n",
	(int) direction, (int) top, (int) bot));

   lines = (direction & 0x1f);
   if ((direction & 0x80) == 0)
      lines = -lines;

   TN5250_LOG(("Roll: lines = %d\n", lines));

   if (lines == 0)
      return;

   tn5250_display_roll(This->display, top, bot, lines);
   tn5250_display_update(This->display);
}

static void tn5250_session_start_of_field(Tn5250Session * This)
{
   int Y, X;
   /* int done, curpos; */
   Tn5250Field *field;
   unsigned char FFW1, FFW2, FCW1, FCW2;
   unsigned char Attr;
   unsigned char Length1, Length2, cur_char;
   int input_field;
   int endrow, endcol;

   TN5250_LOG(("StartOfField: entered.\n"));

   tn5250_display_indicator_set(This->display, TN5250_DISPLAY_IND_X_SYSTEM);

   cur_char = tn5250_record_get_byte(This->record);

   if ((cur_char & 0xe0) != 0x20) {
      input_field = 1;
      FFW1 = cur_char;
      FFW2 = tn5250_record_get_byte(This->record);

      TN5250_LOG(("StartOfField: field format word = 0x%02X%02X\n", FFW1, FFW2));
      cur_char = tn5250_record_get_byte(This->record);

      FCW1 = 0;
      FCW2 = 0;
      while ((cur_char & 0xe0) != 0x20) {
	 FCW1 = cur_char;
	 FCW2 = tn5250_record_get_byte(This->record);

	 TN5250_LOG(("StartOfField: field control word = 0x%02X%02X\n", FCW1, FCW2));
	 cur_char = tn5250_record_get_byte(This->record);
      }
   } else {
      /* Output only field. */
      input_field = 0;
      FFW1 = 0;
      FFW2 = 0;
      FCW1 = 0;
      FCW2 = 0;
   }

   TN5250_ASSERT((cur_char & 0xe0) == 0x20);

   TN5250_LOG(("StartOfField: attribute = 0x%02X\n", cur_char));
   Attr = cur_char; 
   tn5250_display_addch(This->display, cur_char);

   Length1 = tn5250_record_get_byte(This->record);
   Length2 = tn5250_record_get_byte(This->record);

   if (input_field) {
      /* FIXME: `This address is either determined by the contents of the
       * preceding SBA order or calculated from the field length parameter
       * in the last SF order.'  5494 Functions Reference.  -JMF */
      X = tn5250_display_cursor_x(This->display);
      Y = tn5250_display_cursor_y(This->display);

      if ((field = tn5250_display_field_at (This->display, Y, X)) != NULL) {
	 TN5250_LOG(("StartOfField: Modifying field.\n"));
	 if (tn5250_field_start_col (field) == X &&
	       tn5250_field_start_row (field) == Y) {
	    field->FFW = (FFW1 << 8) | FFW2;
	    field->attribute = Attr;
	 }
      } else {
	 TN5250_LOG(("StartOfField: Adding field.\n"));
	 field = tn5250_field_new (tn5250_display_width(This->display));
	 TN5250_ASSERT(field != NULL);

	 field->FFW = (FFW1 << 8) | FFW2;
	 field->FCW = (FCW1 << 8) | FCW2;
	 field->attribute = Attr;
	 field->length =(Length1 << 8) | Length2;
	 field->start_row = Y;
	 field->start_col = X;

	 tn5250_dbuffer_add_field (
	       tn5250_display_dbuffer (This->display), field);
      }
   } else {
      TN5250_LOG(("StartOfField: Output only field.\n"));
      field = NULL;
   }

   if (input_field) {
      TN5250_ASSERT (field != NULL);
      endrow = tn5250_field_end_row(field);
      endcol = tn5250_field_end_col(field);
      if (endcol == tn5250_display_width(This->display) - 1) { 
	 endcol = 0;
	 if (endrow == tn5250_display_height(This->display) - 1)
	    endrow = 0;
	 else
	    endrow++;
      } else 
	 endcol++;

      TN5250_LOG(("StartOfField: endrow = %d; endcol = %d\n", endrow, endcol));

#ifndef NDEBUG
      tn5250_field_dump(field);
#endif

      tn5250_display_set_cursor(This->display, endrow, endcol);
      tn5250_display_addch(This->display, 0x20);
      tn5250_display_set_cursor(This->display, Y, X);
   } 
}

/*
 *    Clear the format table, get the table header data, and copy it to the
 *    table.  This includes the operator error line (byte 4).
 */
static void tn5250_session_start_of_header(Tn5250Session * This)
{
   int i, n;
   unsigned char *ptr = NULL;

   TN5250_LOG (("StartOfHeader: entered.\n"));
   
   tn5250_dbuffer_clear_table(tn5250_display_dbuffer(This->display));
   tn5250_display_indicator_set(This->display, TN5250_DISPLAY_IND_X_SYSTEM);

   n = tn5250_record_get_byte (This->record);
   if (n > 0) {
      ptr = (unsigned char *)malloc (n);
      TN5250_ASSERT (ptr != NULL);
   }
   for (i = 0; i < n; i++)
      ptr[i] = tn5250_record_get_byte (This->record);
   tn5250_display_set_header_data (This->display, ptr, n);
   if (ptr != NULL)
      free (ptr);
}

/*
 *    FIXME: According to "Functions Reference",
 *    http://publib.boulder.ibm.com:80/cgi-bin/bookmgr/BOOKS/C02E2001/15.6.4,
 *    a zero in the column field (X) is acceptable if:
 *      - The rows field is 1.
 *      - The SBA order is followed by an SF (Start of Field) order.
 *    This is how to start a field at row 1, column 1.  We should also be
 *    able to handle a starting attribute there in the display buffer.
 */
static void tn5250_session_set_buffer_address(Tn5250Session * This)
{
   int X, Y;

   Y = tn5250_record_get_byte(This->record);
   X = tn5250_record_get_byte(This->record);

   /* Since we can't handle it yet... */
   TN5250_ASSERT( (X == 0 && Y == 1) ||
	 (X > 0 && Y > 0) );

   tn5250_display_set_cursor(This->display, Y - 1, X - 1);
   TN5250_LOG(("SetBufferAddress: row = %d; col = %d\n", Y, X));
}

static void tn5250_session_repeat_to_address(Tn5250Session * This)
{
   unsigned char temp[3];
   int x, y, ins_loc;
   Tn5250Field  *field;

   TN5250_LOG(("RepeatToAddress: entered.\n"));

   temp[0] = tn5250_record_get_byte(This->record);
   temp[1] = tn5250_record_get_byte(This->record);
   temp[2] = tn5250_record_get_byte(This->record);

   TN5250_LOG(("RepeatToAddress: row = %d; col = %d; char = 0x%02X\n",
	temp[0], temp[1], temp[2]));

   while(1) {
      y = tn5250_display_cursor_y(This->display);
      x = tn5250_display_cursor_x(This->display);

      tn5250_display_addch(This->display, temp[2]);

      if(y == temp[0] - 1 && x == temp[1] - 1)
	 break;
   }
}

static void tn5250_session_insert_cursor(Tn5250Session * This)
{
   int cur_char1, cur_char2;

   TN5250_LOG(("InsertCursor: entered.\n"));

   cur_char1 = tn5250_record_get_byte(This->record);
   cur_char2 = tn5250_record_get_byte(This->record);

   TN5250_LOG(("InsertCursor: row = %d; col = %d\n", cur_char1, cur_char2));

   tn5250_display_set_pending_insert (This->display, cur_char1 - 1,
	 cur_char2 - 1);
}

static void tn5250_session_write_structured_field(Tn5250Session * This)
{
   unsigned char temp[5];

   TN5250_LOG(("WriteStructuredField: entered.\n"));

   temp[0] = tn5250_record_get_byte(This->record);
   temp[1] = tn5250_record_get_byte(This->record);
   temp[2] = tn5250_record_get_byte(This->record);
   temp[3] = tn5250_record_get_byte(This->record);
   temp[4] = tn5250_record_get_byte(This->record);

   TN5250_LOG(("WriteStructuredField: length = %d\n",
	(temp[0] << 8) | temp[1]));
   TN5250_LOG(("WriteStructuredField: command class = 0x%02X\n", temp[2]));
   TN5250_LOG(("WriteStructuredField: command type = 0x%02X\n", temp[3]));

   tn5250_stream_query_reply(This->stream);
}

static void tn5250_session_read_screen_immediate(Tn5250Session * This)
{
   int row, col;
   int buffer_size;
   unsigned char *buffer;

   TN5250_LOG(("ReadScreenImmediate: entered.\n"));

   buffer_size = tn5250_display_width(This->display) *
       tn5250_display_height(This->display);

   buffer = (unsigned char *) malloc(buffer_size);
   TN5250_ASSERT(buffer != NULL);

   for (row = 0; row < tn5250_display_height(This->display); row++) {
      for (col = 0; col < tn5250_display_width(This->display); col++) {
	 buffer[row * tn5250_display_width(This->display) + col]
	     = tn5250_display_char_at(This->display, row, col);
      }
   }

   tn5250_stream_send_packet(This->stream, buffer_size,
			     TN5250_RECORD_FLOW_DISPLAY,
	       TN5250_RECORD_H_NONE, TN5250_RECORD_OPCODE_NO_OP, buffer);

   free(buffer);
}

static void tn5250_session_read_input_fields(Tn5250Session * This)
{
   unsigned char CC1, CC2;

   TN5250_LOG(("ReadInputFields: entered.\n"));

   This->read_opcode = CMD_READ_INPUT_FIELDS;

   CC1 = tn5250_record_get_byte(This->record);
   tn5250_session_handle_cc1 (This, CC1);

   CC2 = tn5250_record_get_byte(This->record);
   tn5250_session_handle_cc2 (This, CC2);

   TN5250_LOG(("ReadInputFields: CC1 = 0x%02X; CC2 = 0x%02X\n", CC1, CC2));
   tn5250_display_indicator_clear(This->display,
				  TN5250_DISPLAY_IND_X_SYSTEM
				  | TN5250_DISPLAY_IND_X_CLOCK
				  | TN5250_DISPLAY_IND_INHIBIT);
   tn5250_display_update(This->display);
}

static void tn5250_session_read_mdt_fields(Tn5250Session * This)
{
   unsigned char CC1, CC2;

   TN5250_LOG(("ReadMDTFields: entered.\n"));

   This->read_opcode = CMD_READ_MDT_FIELDS;

   CC1 = tn5250_record_get_byte(This->record);
   tn5250_session_handle_cc1 (This, CC1);

   CC2 = tn5250_record_get_byte(This->record);
   tn5250_session_handle_cc2 (This, CC2);

   TN5250_LOG(("ReadMDTFields: CC1 = 0x%02X; CC2 = 0x%02X\n", CC1, CC2));
   tn5250_display_indicator_clear(This->display,
				  TN5250_DISPLAY_IND_X_SYSTEM
				  | TN5250_DISPLAY_IND_X_CLOCK
				  | TN5250_DISPLAY_IND_INHIBIT);

   tn5250_display_update(This->display);
}

/* vi:set cindent sts=3 sw=3: */
