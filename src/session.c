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
#include <ctype.h>

#include "utility.h"
#include "buffer.h"
#include "display.h"
#include "record.h"
#include "stream.h"
#include "field.h"
#include "formattable.h"
#include "terminal.h"
#include "session.h"
#include "codes5250.h"

static void tn5250_session_handle_keys(Tn5250Session * This);
static void tn5250_session_handle_key(Tn5250Session * This, int cur_key);
static void tn5250_session_handle_receive(Tn5250Session * This);
static void tn5250_session_invite(Tn5250Session * This);
static void tn5250_session_cancel_invite(Tn5250Session * This);
static void tn5250_session_send_fields(Tn5250Session * This, int aidcode);
static void tn5250_session_process_stream(Tn5250Session * This);
static void tn5250_session_write_error_code(Tn5250Session * This);
static void tn5250_session_write_to_display(Tn5250Session * This);
static void tn5250_session_clear_unit(Tn5250Session * This);
static void tn5250_session_clear_unit_alternate(Tn5250Session * This);
static void tn5250_session_clear_format_table(Tn5250Session * This);
static void tn5250_session_read_immediate(Tn5250Session * This);
static void tn5250_session_home(Tn5250Session * This);
static void tn5250_session_print(Tn5250Session * This);
static void tn5250_session_system_request(Tn5250Session * This);
static void tn5250_session_attention(Tn5250Session * This);
static void tn5250_session_output_only(Tn5250Session * This);
static void tn5250_session_save_screen(Tn5250Session * This);
static void tn5250_session_restore_screen(Tn5250Session * This);
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

Tn5250Session *tn5250_session_new()
{
   Tn5250Session *This;
   int n;

   This = tn5250_new(Tn5250Session, 1);
   if (This == NULL)
      return NULL;

   This->dsp = tn5250_display_new(80, 24);
   if (This->dsp == NULL) {
      free (This);
      return NULL;
   }

   This->record = tn5250_record_new();
   if (This->record == NULL) {
      tn5250_display_destroy(This->dsp);
      free (This);
      return NULL;
   }

   This->term = NULL;
   This->stream = NULL;
   This->table = tn5250_table_new(This->dsp);
   if (This->table == NULL) {
      tn5250_display_destroy (This->dsp);
      tn5250_record_destroy (This->record);
      free (This);
      return NULL;
   }

   This->invited = 1;
   This->pending_insert = 0;
   This->read_opcode = 0;
   This->key_queue_head = This->key_queue_tail = 0;

   for (n = 0; n < sizeof(This->saved_bufs) / sizeof(Tn5250Display *); n++)
      This->saved_bufs[n] = NULL;

   return This;
}

void tn5250_session_destroy(Tn5250Session * This)
{
   int n;

   for (n = 0; n < sizeof(This->saved_bufs) / sizeof(Tn5250Display *); n++) {
      if (This->saved_bufs[n] != NULL)
	 tn5250_display_destroy(This->saved_bufs[n]);
   }

   if (This->stream != NULL)
      tn5250_stream_destroy(This->stream);
   if (This->record != NULL)
      tn5250_record_destroy(This->record);
   if (This->table != NULL)
      tn5250_table_destroy(This->table);
   if (This->term != NULL)
      tn5250_terminal_destroy(This->term);
   if (This->dsp != NULL)
      tn5250_display_destroy(This->dsp);

   free (This);
}

void tn5250_session_set_terminal(Tn5250Session * This, Tn5250Terminal * newterminal)
{
   This->term = newterminal;
}

void tn5250_session_set_stream(Tn5250Session * This, Tn5250Stream * newstream)
{
   if ((This->stream = newstream) != NULL && This->term != NULL) {
      tn5250_terminal_update(This->term, This->dsp);
      tn5250_terminal_update_indicators(This->term, This->dsp);
   }
}

void tn5250_session_main_loop(Tn5250Session * This)
{
   int r;
   int is_x_system;

   while (1) {
      is_x_system = (tn5250_display_indicators(This->dsp) & 
	 TN5250_DISPLAY_IND_X_SYSTEM) != 0;

      /* Handle keys from our key queue if we aren't X SYSTEM. */
      if (This->key_queue_head != This->key_queue_tail && !is_x_system) {
	 tn5250_session_handle_key (This, This->key_queue[This->key_queue_head]);
	 if (++ This->key_queue_head == TN5250_SESSION_KEY_QUEUE_SIZE)
	    This->key_queue_head = 0;
	 continue;
      }

      r = tn5250_terminal_waitevent(This->term);
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
      cur_key = tn5250_terminal_getkey (This->term);

      if (cur_key != -1) {
	 if ((tn5250_display_indicators(This->dsp) & TN5250_DISPLAY_IND_X_SYSTEM) != 0) {
	    /* We can handle system request here. */
	    if (cur_key == K_SYSREQ) {
	       /* Flush the keyboard queue. */
	       This->key_queue_head = This->key_queue_tail = 0;
	       tn5250_session_handle_key (This, K_SYSREQ);
	       break;
	    }

	    if ((This->key_queue_tail + 1 == This->key_queue_head) ||
		  (This->key_queue_head == 0 &&
		   This->key_queue_tail == TN5250_SESSION_KEY_QUEUE_SIZE - 1)) {
	       /* FIXME: Keyboard buffer is full, beep. */
	    }
	    This->key_queue[This->key_queue_tail] = cur_key;
	    if (++ This->key_queue_tail == TN5250_SESSION_KEY_QUEUE_SIZE)
	       This->key_queue_tail = 0;
	 } else {
	    /* We shouldn't ever be handling a key here if there are keys in the queue. */
	    TN5250_ASSERT(This->key_queue_head == This->key_queue_tail);
	    tn5250_session_handle_key (This, cur_key);
	 }
      }
   } while (cur_key != -1);

   tn5250_terminal_update(This->term, This->dsp);
   tn5250_terminal_update_indicators(This->term, This->dsp);
}

static void tn5250_session_handle_key(Tn5250Session * This, int cur_key)
{
   int curfield;
   int shiftcount;
   int send = 0;
   unsigned char aidcode;
   int fieldtype;
   int valid_char;
   int X, Y;
   Tn5250Field *field;

   TN5250_LOG(("HandleKey: entered.\n"));

   TN5250_ASSERT(This->term != NULL);

   curfield = tn5250_table_current_field(This->table);
   /* cur_key = tn5250_terminal_getkey(This->term); */
   /* while (cur_key != -1) { */

   TN5250_LOG(("@key %d\n", cur_key));
   X = tn5250_display_cursor_x(This->dsp);
   Y = tn5250_display_cursor_y(This->dsp);

   if (tn5250_display_inhibited(This->dsp)) {
      if (cur_key != K_RESET) {
	 /* FIXME: Terminal should beep at this point. */
	 cur_key = tn5250_terminal_getkey(This->term);
	 return;
      }
   }
   switch (cur_key) {
   case K_RESET:
      tn5250_display_uninhibit(This->dsp);
      break;
   case K_BACKSPACE:
   case K_LEFT:
      tn5250_display_left(This->dsp);
      curfield = tn5250_table_field_number(This->table,
				  tn5250_display_cursor_y(This->dsp),
				   tn5250_display_cursor_x(This->dsp));
      tn5250_table_set_current_field(This->table, curfield);
      break;
   case K_RIGHT:
      tn5250_display_right(This->dsp, 1);
      curfield = tn5250_table_field_number(This->table,
				   tn5250_display_cursor_y(This->dsp),
				  tn5250_display_cursor_x(This->dsp));
      tn5250_table_set_current_field(This->table, curfield);
      break;
   case K_UP:
      tn5250_display_up(This->dsp);
      curfield = tn5250_table_field_number(This->table, tn5250_display_cursor_y(This->dsp),
				  tn5250_display_cursor_x(This->dsp));
      tn5250_table_set_current_field(This->table, curfield);
      break;
   case K_DOWN:
      tn5250_display_down(This->dsp);
      curfield = tn5250_table_field_number(This->table, tn5250_display_cursor_y(This->dsp),
				  tn5250_display_cursor_x(This->dsp));
      tn5250_table_set_current_field(This->table, curfield);
      break;
   case (K_F1):
      send = 1;
      aidcode = TN5250_SESSION_AID_F1;
      break;
   case (K_F2):
      send = 1;
      aidcode = TN5250_SESSION_AID_F2;
      break;
   case (K_F3):
      send = 1;
      aidcode = TN5250_SESSION_AID_F3;
      break;
   case (K_F4):
      send = 1;
      aidcode = TN5250_SESSION_AID_F4;
      break;
   case (K_F5):
      send = 1;
      aidcode = TN5250_SESSION_AID_F5;
      break;
   case (K_F6):
      send = 1;
      aidcode = TN5250_SESSION_AID_F6;
      break;
   case (K_F7):
      send = 1;
      aidcode = TN5250_SESSION_AID_F7;
      break;
   case (K_F8):
      send = 1;
      aidcode = TN5250_SESSION_AID_F8;
      break;
   case (K_F9):
      send = 1;
      aidcode = TN5250_SESSION_AID_F9;
      break;
   case (K_F10):
      send = 1;
      aidcode = TN5250_SESSION_AID_F10;
      break;
   case (K_F11):
      send = 1;
      aidcode = TN5250_SESSION_AID_F11;
      break;
   case (K_F12):
      send = 1;
      aidcode = TN5250_SESSION_AID_F12;
      break;
   case (K_F13):
      send = 1;
      aidcode = TN5250_SESSION_AID_F13;
      break;
   case (K_F14):
      send = 1;
      aidcode = TN5250_SESSION_AID_F14;
      break;
   case (K_F15):
      send = 1;
      aidcode = TN5250_SESSION_AID_F15;
      break;
   case (K_F16):
      send = 1;
      aidcode = TN5250_SESSION_AID_F16;
      break;
   case (K_F17):
      send = 1;
      aidcode = TN5250_SESSION_AID_F17;
      break;
   case (K_F18):
      send = 1;
      aidcode = TN5250_SESSION_AID_F18;
      break;
   case (K_F19):
      send = 1;
      aidcode = TN5250_SESSION_AID_F19;
      break;
   case (K_F20):
      send = 1;
      aidcode = TN5250_SESSION_AID_F20;
      break;
   case (K_F21):
      send = 1;
      aidcode = TN5250_SESSION_AID_F21;
      break;
   case (K_F22):
      send = 1;
      aidcode = TN5250_SESSION_AID_F22;
      break;
   case (K_F23):
      send = 1;
      aidcode = TN5250_SESSION_AID_F23;
      break;
   case (K_F24):
      send = 1;
      aidcode = TN5250_SESSION_AID_F24;
      break;
   case (K_HOME):
      if (This->pending_insert) {
	 tn5250_display_goto_ic(This->dsp);
      } else {
	 int gx, gy;
	 curfield = tn5250_table_first_field(This->table);
	 field = tn5250_table_field_n(This->table, curfield);
	 if (field != NULL) {
	    gy = tn5250_field_start_row(field) - 1;
	    gx = tn5250_field_start_col(field) - 1;
	 } else
	    gx = gy = 0;
	 if (gy == tn5250_display_cursor_y (This->dsp)
	       && gx == tn5250_display_cursor_x (This->dsp))
	    tn5250_session_home (This);
	 else
	    tn5250_display_cursor_set(This->dsp, gy, gx);
      }
      break;
   case (K_END):
      curfield = tn5250_table_field_number(This->table, Y, X);
      field = tn5250_table_field_n(This->table, curfield);
      if (field != NULL && !tn5250_field_is_bypass(field)) {
	 tn5250_display_cursor_set(This->dsp,
				   tn5250_field_start_row(field) - 1,
				   tn5250_field_start_col(field) - 1);
	 tn5250_display_right(This->dsp,
		       tn5250_field_count_eof (field));
      }
      break;
   case (K_DELETE):
      curfield = tn5250_table_field_number(This->table, Y, X);
      field = tn5250_table_field_n(This->table, curfield);
      if (field == NULL || tn5250_field_is_bypass(field)) {
	 tn5250_display_inhibit(This->dsp);
      } else {
	 tn5250_table_del_char(This->table, Y, X);
	 shiftcount = tn5250_field_count_right(field, Y, X);
	 tn5250_display_del(This->dsp, shiftcount);
      }
      break;
   case (K_INSERT):
      if ((tn5250_display_indicators(This->dsp) & TN5250_DISPLAY_IND_INSERT)
	  != 0)
	 tn5250_display_indicator_clear(This->dsp, TN5250_DISPLAY_IND_INSERT);
      else
	 tn5250_display_indicator_set(This->dsp, TN5250_DISPLAY_IND_INSERT);
      break;
   case (K_TAB):
      TN5250_LOG(("HandleKey: (K_TAB) curfield = %d\n", curfield));
      curfield = tn5250_table_next_field2(This->table,
				   tn5250_display_cursor_y(This->dsp),
				  tn5250_display_cursor_x(This->dsp));
      field = tn5250_table_field_n(This->table, curfield);
      tn5250_display_cursor_set(This->dsp,
				tn5250_field_start_row(field) - 1,
				tn5250_field_start_col(field) - 1);
      break;

   case (K_BACKTAB):
      /* Backtab: Move to start of this field, or start of 
       * previous field if already there. */
      TN5250_LOG(("HandleKey: (K_BACKTAB) curfield = %d\n", curfield));
      curfield = tn5250_table_field_number (This->table,
				   tn5250_display_cursor_y(This->dsp),
				  tn5250_display_cursor_x(This->dsp));
      field = tn5250_table_field_n (This->table, curfield);

      if (field == NULL || tn5250_field_count_left(field,
	       tn5250_display_cursor_y(This->dsp),
	       tn5250_display_cursor_x(This->dsp)) == 0) {
	 curfield = tn5250_table_prev_field2(This->table,
				   tn5250_display_cursor_y(This->dsp),
				  tn5250_display_cursor_x(This->dsp));
      }
      if ((field = tn5250_table_field_n(This->table, curfield)) != NULL)
	 tn5250_display_cursor_set(This->dsp, tn5250_field_start_row(field) - 1,
	       tn5250_field_start_col(field) - 1);
      break;

   case (K_ENTER):
      send = 1;
      aidcode = TN5250_SESSION_AID_ENTER;
      break;

   case (K_ROLLDN):
      send = 1;
      aidcode = TN5250_SESSION_AID_PGUP;
      break;
   case (K_ROLLUP):
      send = 1;
      aidcode = TN5250_SESSION_AID_PGDN;
      break;

   case (K_FIELDEXIT):
      curfield = tn5250_table_field_number(This->table, Y, X);
      field = tn5250_table_field_n(This->table, curfield);
      if (field == NULL || tn5250_field_is_bypass(field))
	 tn5250_display_inhibit(This->dsp);
      else {
	 tn5250_table_field_exit(This->table, Y, X);
	 curfield = tn5250_table_next_field(This->table);
	 field = tn5250_table_field_n(This->table, curfield);
	 tn5250_display_cursor_set(This->dsp,
				   tn5250_field_start_row(field) - 1,
				   tn5250_field_start_col(field) - 1);
	 tn5250_terminal_update(This->term, This->dsp);
	 if (tn5250_field_is_auto_enter(field)) {
	    /* FIXME: Need to set the aidcode here */
	    send = 1;
	 }
      }
      break;

   case K_SYSREQ:
      This->read_opcode = 0; /* We are out of the read. */
      tn5250_session_system_request(This);
      break;

   case K_ATTENTION:
      This->read_opcode = 0; /* We are out of the read. */
      tn5250_session_attention(This);
      break;

   case K_PRINT:
      tn5250_session_print(This);
      break;
      
   default:
      TN5250_LOG(("HandleKey: cur_key = %c\n", cur_key));

      /* Field Exit driven by '-' and '+', for Numeric Only and Signed Numbers */
      curfield = tn5250_table_field_number(This->table, Y, X);
      tn5250_table_set_current_field(This->table, curfield);
      field = tn5250_table_field_n(This->table, curfield);

      if (field == NULL || tn5250_field_is_bypass(field)) {
	 tn5250_display_inhibit(This->dsp);
	 break;
      }

      if (cur_key == '+' || cur_key == '-') {
	 if (tn5250_field_is_num_only(field)
	     || tn5250_field_is_signed_num(field)) {
	    tn5250_table_field_exit(This->table, Y, X);
	    if (cur_key == '-')
	       tn5250_field_set_minus_zone(field);
	    curfield = tn5250_table_next_field(This->table);
	    tn5250_display_cursor_set(This->dsp,
				 tn5250_field_start_row(field) - 1,
				tn5250_field_start_col(field) - 1);
	    tn5250_terminal_update(This->term, This->dsp);
	    if (tn5250_field_is_auto_enter(field))
	       send = 1;
	    break;
	 }
      }			/* End of '-' and '+' Field Exit Processing */
      if (cur_key >= K_FIRST_SPECIAL || cur_key < ' ') {
	 break;
      }

      fieldtype = tn5250_field_type(field);

      valid_char = 0;
      TN5250_LOG(("HandleKey: fieldtype = %d\n", fieldtype));
      switch (fieldtype) {
      case TN5250_FIELD_ALPHA_SHIFT:
	 valid_char = 1;
	 break;

      case TN5250_FIELD_ALPHA_ONLY:
	 if (isalpha(cur_key) ||
	     cur_key == ',' ||
	     cur_key == '.' ||
	     cur_key == '-' ||
	     cur_key == ' ') {
	    valid_char = 1;
	 }
	 break;

      case TN5250_FIELD_NUM_SHIFT:
	 valid_char = 1;
	 break;

      case TN5250_FIELD_NUM_ONLY:
	 if (tn5250_isnumeric(cur_key) ||
	     cur_key == '+' ||
	     cur_key == ',' ||
	     cur_key == '.' ||
	     cur_key == '-' ||
	     cur_key == ' ') {
	    valid_char = 1;
	 } else
	    tn5250_display_inhibit(This->dsp);
	 break;

      case TN5250_FIELD_KATA_SHIFT:
	 TN5250_LOG(("KATAKANA not implemneted.\n"));
	 valid_char = 1;
	 break;

      case TN5250_FIELD_DIGIT_ONLY:
	 if (isdigit(cur_key)) {
	    valid_char = 1;
	 }
	 break;

      case TN5250_FIELD_MAG_READER:
	 TN5250_LOG(("MAG_READER not implemneted.\n"));
	 valid_char = 1;
	 break;

      case TN5250_FIELD_SIGNED_NUM:
	 if (isdigit(cur_key) ||
	     cur_key == '+' ||
	     cur_key == '-') {
	    valid_char = 1;
	 }
      }

      if (valid_char) {
	 if (tn5250_field_is_monocase(field)) {
	    cur_key = toupper(cur_key);
	 }
	 if ((tn5250_display_indicators(This->dsp) & TN5250_DISPLAY_IND_INSERT) != 0) {
	    if (tn5250_field_is_full(field)) {
	       tn5250_display_inhibit(This->dsp);
	       break;
	    }
	    tn5250_table_ins_char(This->table, Y, X, tn5250_ascii2ebcdic(cur_key));
	 } else {
	    tn5250_table_add_char(This->table, Y, X, tn5250_ascii2ebcdic(cur_key));
	    TN5250_LOG(("HandleKey: curfield = %d\n", curfield));
	 }

	 if ((tn5250_display_indicators(This->dsp) & TN5250_DISPLAY_IND_INSERT) != 0) {
	    shiftcount = tn5250_field_count_right(field, Y, X);
	    tn5250_display_ins(This->dsp, tn5250_ascii2ebcdic(cur_key), shiftcount);
	 } else
	    tn5250_display_addch(This->dsp, tn5250_ascii2ebcdic(cur_key));

	 if ((X + 1 == tn5250_field_end_col(field))
	     && (Y + 1 == tn5250_field_end_row(field))) {
	    curfield = tn5250_table_next_field(This->table);
	    field = tn5250_table_field_n(This->table, curfield);
	    tn5250_display_cursor_set(This->dsp,
			      tn5250_field_start_row(field) - 1,
				tn5250_field_start_col(field) - 1);
	 }
	 break;
      } else {
	 tn5250_display_inhibit(This->dsp);
	 break;
      }
   }
   if (send) {
      tn5250_display_indicator_set(This->dsp, TN5250_DISPLAY_IND_X_SYSTEM);
      tn5250_terminal_update_indicators(This->term, This->dsp);
      tn5250_session_send_fields(This, aidcode);
      send = 0;
   }
/*      if ((tn5250_display_indicators(This->dsp) & TN5250_DISPLAY_IND_X_SYSTEM)
	    != 0)
	 break;
      cur_key = tn5250_terminal_getkey(This->term);
   } */

 /*  tn5250_terminal_update(This->term, This->dsp);
   tn5250_terminal_update_indicators(This->term, This->dsp); */
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
   tn5250_display_indicator_clear(This->dsp, TN5250_DISPLAY_IND_X_CLOCK);
   if (This->term != NULL)
      tn5250_terminal_update_indicators(This->term, This->dsp);
}

static void tn5250_session_cancel_invite(Tn5250Session * This)
{
   TN5250_LOG(("CancelInvite: entered.\n"));
   tn5250_display_indicator_set(This->dsp, TN5250_DISPLAY_IND_X_CLOCK);
   if (This->term != NULL)
      tn5250_terminal_update_indicators(This->term, This->dsp);
   tn5250_stream_send_packet(This->stream, 0, TN5250_RECORD_FLOW_DISPLAY, TN5250_RECORD_H_NONE,
			     TN5250_RECORD_OPCODE_CANCEL_INVITE, NULL);
   This->invited = 0;
}

static void tn5250_session_send_fields(Tn5250Session * This, int aidcode)
{
   int temp;
   Tn5250Buffer field_buf;
   Tn5250Field *field;
   unsigned char c;
   int X, Y, size;

   X = tn5250_display_cursor_x(This->dsp);
   Y = tn5250_display_cursor_y(This->dsp);

   TN5250_LOG(("SendFields: Number of fields: %d\n", tn5250_table_field_count(This->table)));

   tn5250_buffer_init(&field_buf);
   tn5250_buffer_append_byte(&field_buf, Y + 1);
   tn5250_buffer_append_byte(&field_buf, X + 1);

   TN5250_ASSERT(aidcode != 0);
   tn5250_buffer_append_byte(&field_buf, aidcode);

   TN5250_LOG(("SendFields: row = %d; col = %d; aid = 0x%02x\n", Y, X, aidcode));

   /* FIXME: Do we handle field resequencing? */
   /* FIXME: Do we handle signed fields (changing the zone of the second-last
    * digit and not transmitting the last digit)? */
   switch (This->read_opcode) {
   case CMD_READ_INPUT_FIELDS:
      if (tn5250_table_mdt(This->table)) {
	 field = This->table->field_list;
	 if (field != NULL) {
	    do {
	       size = tn5250_field_length(field);
	       for (temp = 0; temp < size; temp++) {
		  c = tn5250_field_get_char(field, temp);
		  tn5250_buffer_append_byte(&field_buf, c == 0 ? 0x40 : c);
	       }
	       field = field->next;
	    } while (field != This->table->field_list);
	 }
      }
      break;

   case CMD_READ_IMMEDIATE:
      if (tn5250_table_mdt(This->table)) {
	 field = This->table->field_list;
	 if (field != NULL) {
	    do {
	       size = tn5250_field_length(field);
	       for (temp = 0; temp < size; temp++) {
		  c = tn5250_field_get_char(field, temp);
		  tn5250_buffer_append_byte(&field_buf, c == 0 ? 0x40 : c);
	       }
	       field = field->next;
	    } while (field != This->table->field_list);
	 }
      }
      break;

   case CMD_READ_MDT_FIELDS:
      field = This->table->field_list;
      if (field != NULL) {
	 do {
	    if (tn5250_field_mdt(field)) {
	       TN5250_LOG(("Sending:\n"));
	       tn5250_field_dump(field);
	       tn5250_buffer_append_byte(&field_buf, SBA);
	       tn5250_buffer_append_byte(&field_buf,
					 tn5250_field_start_row(field));
	       tn5250_buffer_append_byte(&field_buf,
					 tn5250_field_start_col(field));
	       size = tn5250_field_length(field);

	       /* Strip trailing NULs */
	       while (size > 0 && (tn5250_field_get_char (field, size - 1) == 0x00))
		  size--;

	       TN5250_LOG(("SendFields: size = %d\n", size));
	       for (temp = 0; temp < size; temp++) {
		  c = tn5250_field_get_char(field, temp);
		  tn5250_buffer_append_byte(&field_buf, c == 0 ? 0x40 : c);
	       }
	    }
	    field = field->next;
	 } while (field != This->table->field_list);
      }
      break;

   default: /* Huh? */
      TN5250_LOG(("BUG!!! Trying to transmit fields when This->read_opcode = 0x%02X.\n",
	       This->read_opcode));
      TN5250_ASSERT(0);
   }

   This->read_opcode = 0; /* No longer in a read command. */

   tn5250_display_indicator_clear(This->dsp, TN5250_DISPLAY_IND_INSERT);
   if (This->term != NULL)
      tn5250_terminal_update_indicators(This->term, This->dsp);

   tn5250_stream_send_packet(This->stream, tn5250_buffer_length(&field_buf), TN5250_RECORD_FLOW_DISPLAY,
			     TN5250_RECORD_H_NONE,
			     TN5250_RECORD_OPCODE_PUT_GET,
			     tn5250_buffer_data(&field_buf));
   tn5250_buffer_free(&field_buf);
}

static void tn5250_session_process_stream(Tn5250Session * This)
{
   int cur_command;

   TN5250_LOG(("ProcessStream: entered.\n"));
   This->pending_insert = 0;
   while (!tn5250_record_is_chain_end(This->record)) {

      cur_command = tn5250_record_get_byte(This->record);
      TN5250_ASSERT(cur_command == ESC);
      cur_command = tn5250_record_get_byte(This->record);
      TN5250_LOG(("ProcessStream: cur_command = 0x%02X\n", cur_command));

      switch (cur_command) {
      case (CMD_WRITE_TO_DISPLAY):
	 tn5250_session_write_to_display(This);
	 break;
      case (CMD_CLEAR_UNIT):
	 tn5250_session_clear_unit(This);
	 break;
      case (CMD_CLEAR_UNIT_ALTERNATE):
	 tn5250_session_clear_unit_alternate(This);
	 break;
      case (CMD_CLEAR_FORMAT_TABLE):
	 tn5250_session_clear_format_table(This);
	 break;
      case (CMD_READ_MDT_FIELDS):
	 tn5250_session_read_mdt_fields(This);
	 break;
      case (CMD_READ_IMMEDIATE):
	 tn5250_session_read_immediate(This);
	 break;
      case (CMD_READ_INPUT_FIELDS):
	 tn5250_session_read_input_fields(This);
	 break;
      case (CMD_READ_SCREEN_IMMEDIATE):
	 tn5250_session_read_screen_immediate(This);
	 break;
      case (CMD_WRITE_STRUCTURED_FIELD):
	 tn5250_session_write_structured_field(This);
	 break;
      case (CMD_SAVE_SCREEN):
	 tn5250_session_save_screen(This);
	 break;
      case (CMD_RESTORE_SCREEN):
	 tn5250_session_restore_screen(This);
	 break;
      case (CMD_WRITE_ERROR_CODE):
	 tn5250_session_write_error_code(This);
	 break;
      case (CMD_ROLL):
	 tn5250_session_roll(This);
	 break;
      default:
	 TN5250_LOG(("Error: Unknown command %2.2\n", cur_command));
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

   curX = tn5250_display_cursor_x(This->dsp);
   curY = tn5250_display_cursor_y(This->dsp);

   tn5250_display_cursor_set(This->dsp, 23, 0);
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
	 if (cur_order == IC) {
	    tn5250_session_insert_cursor(This);
	    This->pending_insert = 1;
	 } else if (cur_order == ESC) {
	    done = 1;
	    tn5250_record_unget_byte(This->record);
	 } else if (tn5250_printable(cur_order))
	    tn5250_display_addch(This->dsp, cur_order);
	 else {
	    TN5250_LOG(("Error: Unknown order -- %2.2X --\n", cur_order));
	    TN5250_ASSERT(0);
	 }
      }
   }
   TN5250_LOG(("\n"));
   tn5250_display_cursor_set(This->dsp, curY, curX);
   tn5250_display_inhibit(This->dsp);
   if (This->term != NULL)
      tn5250_terminal_update(This->term, This->dsp);
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

   switch (cc1 & 0x07) {
   case 0x00:
      lock_kb = 0;
      break;

   case 0x01:
      reset_non_bypass_mdt = 1;
      break;

   case 0x02:
      reset_all_mdt = 1;
      break;

   case 0x03:
      null_non_bypass_mdt = 1;
      break;

   case 0x04:
      reset_non_bypass_mdt = 1;
      null_non_bypass = 1;
      break;

   case 0x05:
      reset_non_bypass_mdt = 1;
      null_non_bypass_mdt = 1;
      break;

   case 0x06:
      reset_all_mdt = 1;
      null_non_bypass = 1;
      break;
   }

   if (lock_kb) {
      TN5250_LOG(("tn5250_session_handle_cc1: Locking keyboard.\n"));
      tn5250_display_indicator_set (This->dsp, TN5250_DISPLAY_IND_X_SYSTEM);
   }
   if ((iter = This->table->field_list) != NULL) {
      do {
	 if (!tn5250_field_is_bypass (iter)) {
	    if ((null_non_bypass_mdt && tn5250_field_mdt(iter)) || null_non_bypass) {
	       memset (iter->data, 0, iter->length);
	       tn5250_field_update_display (iter);
	    }
	 }
	 if (reset_all_mdt || (reset_non_bypass_mdt && !tn5250_field_is_bypass (iter)))
	    tn5250_field_clear_mdt (iter);
	 iter = iter->next;
      } while (iter != This->table->field_list);
   }
}

static void tn5250_session_write_to_display(Tn5250Session * This)
{
   unsigned char cur_order;
   unsigned char CC1;
   unsigned char CC2;
   int done;
   int count;
   int fieldnum;
   int Y, X;

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
	    This->pending_insert = 1;
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

	       X = tn5250_display_cursor_x(This->dsp);
	       Y = tn5250_display_cursor_y(This->dsp);
	       if (tn5250_table_field_number(This->table, Y, X) >= 0) {
		  Tn5250Field *field;
		  int curfield;
		  int ofs;
		  /* We don't want to set MDT here, so we access the Tn5250Field
		   * structures innards directly. */
		  TN5250_LOG(("WTD putting char 0x%02X in field %d!",
			   cur_order, tn5250_table_field_number(This->table, Y, X)));
		  curfield = tn5250_table_field_number (This->table, Y, X);
		  field = tn5250_table_field_n (This->table, curfield);
		  ofs = tn5250_field_count_left (field, Y, X);
		  field->data[ofs] = cur_order;
	       }
	       tn5250_display_addch(This->dsp, cur_order);
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

   if (This->pending_insert) {
      TN5250_LOG(("WriteToDisplay: disp_buf.set_new_ic()\n"));
      tn5250_display_goto_ic(This->dsp);
      X = tn5250_display_cursor_x(This->dsp);
      Y = tn5250_display_cursor_y(This->dsp);
      fieldnum = tn5250_table_field_number(This->table, Y, X);
      if (fieldnum >= 0)
	 tn5250_table_set_current_field(This->table, fieldnum);
   } else {
      done = 0;
      count = 0;
      while (!done && count < tn5250_table_field_count(This->table)) {
	 Tn5250Field *field = tn5250_table_field_n(This->table, count);
	 TN5250_ASSERT (field != NULL);
	 if (!tn5250_field_is_bypass(field)) {
	    TN5250_LOG(("WriteToDisplay: Position to field %d\n", count));
	    tn5250_display_cursor_set(This->dsp,
				      tn5250_field_start_row(field) - 1,
				      tn5250_field_start_col(field) - 1);
	    TN5250_LOG(("WriteToDisplay: row = %d; col = %d\n",
		 tn5250_field_start_row(field),
		 tn5250_field_start_col(field)));
	    X = tn5250_display_cursor_x(This->dsp);
	    Y = tn5250_display_cursor_y(This->dsp);
	    fieldnum = tn5250_table_field_number(This->table, Y, X);
	    if (fieldnum >= 0)
	       tn5250_table_set_current_field(This->table, fieldnum);
	    done = 1;
	 }
	 count++;
      }
   }

   if (!done) {
      TN5250_LOG(("WriteToDisplay: Moving IC to (1,1)\n"));
      tn5250_display_cursor_set(This->dsp, 0, 0);
   }
   TN5250_LOG(("WriteToDisplay: CTL = 0x%02X\n", CC2));
   if (CC2 & TN5250_SESSION_CTL_MESSAGE_ON)
      tn5250_display_indicator_set(This->dsp, TN5250_DISPLAY_IND_MESSAGE_WAITING);

   if ((CC2 & TN5250_SESSION_CTL_MESSAGE_OFF) && !(CC2 & TN5250_SESSION_CTL_MESSAGE_ON))
      tn5250_display_indicator_clear(This->dsp, TN5250_DISPLAY_IND_MESSAGE_WAITING);

   if ((CC2 & TN5250_SESSION_CTL_CLR_BLINK) != 0 && (CC2 & TN5250_SESSION_CTL_SET_BLINK) == 0) {
      /* FIXME: Hand off to terminal */
   }

   if ((CC2 & TN5250_SESSION_CTL_SET_BLINK) != 0) {
      /* FIXME: Hand off to terminal */
   }

   if ((CC2 & TN5250_SESSION_CTL_ALARM) != 0) {
      /* FIXME: Hand off to terminal */
   }

   if ((CC2 & TN5250_SESSION_CTL_UNLOCK) != 0) {
      tn5250_display_indicator_clear(This->dsp, TN5250_DISPLAY_IND_X_SYSTEM);
   }

   if (This->term != NULL)
      tn5250_terminal_update(This->term, This->dsp);
   tn5250_terminal_update_indicators(This->term, This->dsp);
}

static void tn5250_session_clear_unit(Tn5250Session * This)
{
   TN5250_LOG(("ClearUnit: entered.\n"));

   tn5250_table_clear(This->table);
   tn5250_display_set_size(This->dsp, 24, 80);
   tn5250_display_indicator_set(This->dsp, TN5250_DISPLAY_IND_X_SYSTEM);
   tn5250_display_indicator_clear(This->dsp, TN5250_DISPLAY_IND_INSERT | TN5250_DISPLAY_IND_INHIBIT);
   This->read_opcode = 0;
   tn5250_display_set_temp_ic(This->dsp, 0, 0);

}

static void tn5250_session_clear_unit_alternate(Tn5250Session * This)
{
   unsigned char c;

   TN5250_LOG(("tn5250_session_clear_unit_alternate entered.\n"));
   c = tn5250_record_get_byte(This->record);
   TN5250_LOG(("tn5250_session_clear_unit_alternate, parameter is 0x%02X.\n",
	(int) c));
   tn5250_table_clear(This->table);
   tn5250_display_set_size(This->dsp, 27, 132);
   tn5250_display_indicator_set(This->dsp, TN5250_DISPLAY_IND_X_SYSTEM);
   tn5250_display_indicator_clear(This->dsp, TN5250_DISPLAY_IND_INSERT | TN5250_DISPLAY_IND_INHIBIT);
   This->read_opcode = 0;
   tn5250_display_set_temp_ic(This->dsp, 0, 0);
   tn5250_terminal_update_indicators(This->term, This->dsp);
}

static void tn5250_session_clear_format_table(Tn5250Session * This)
{
   TN5250_LOG(("ClearFormatTable: entered.\n"));
   tn5250_table_clear(This->table);
   tn5250_display_cursor_set(This->dsp, 0, 0);
   tn5250_display_indicator_set(This->dsp, TN5250_DISPLAY_IND_X_SYSTEM);
   tn5250_display_indicator_clear(This->dsp, TN5250_DISPLAY_IND_INSERT);
   This->read_opcode = 0;
   tn5250_terminal_update(This->term, This->dsp);
   tn5250_terminal_update_indicators(This->term, This->dsp);
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

static void tn5250_session_home(Tn5250Session * This)
{
   Tn5250Buffer buf;

   TN5250_LOG(("Home: entered.\n"));

   tn5250_buffer_init(&buf);
   tn5250_buffer_append_byte(&buf, tn5250_display_cursor_y(This->dsp) + 1);
   tn5250_buffer_append_byte(&buf, tn5250_display_cursor_x(This->dsp) + 1);
   tn5250_buffer_append_byte(&buf, TN5250_SESSION_AID_RECORD_BS);

   tn5250_stream_send_packet(This->stream, tn5250_buffer_length(&buf), 
                             TN5250_RECORD_FLOW_DISPLAY,
                             TN5250_RECORD_H_NONE,
                             TN5250_RECORD_OPCODE_NO_OP, 
                             tn5250_buffer_data(&buf));

   tn5250_buffer_free(&buf);
}

static void tn5250_session_system_request(Tn5250Session * This)
{
   TN5250_LOG(("SystemRequest: entered.\n"));
   tn5250_stream_send_packet(This->stream, 0, TN5250_RECORD_FLOW_DISPLAY,
			     TN5250_RECORD_H_SRQ,
			     TN5250_RECORD_OPCODE_NO_OP, NULL);
}

static void tn5250_session_attention(Tn5250Session * This)
{
   TN5250_LOG(("Attention: entered.\n"));
   tn5250_stream_send_packet(This->stream, 0, TN5250_RECORD_FLOW_DISPLAY, TN5250_RECORD_H_ATN,
			     TN5250_RECORD_OPCODE_NO_OP, NULL);
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
   unsigned char outbuf[4];
   int n;

   TN5250_LOG(("SaveScreen: entered.\n"));

   for (n = 1; n < (int) (sizeof(This->saved_bufs) / sizeof(Tn5250Display *)); n++) {
      if (This->saved_bufs[n] == NULL)
	 break;
   }

   TN5250_ASSERT(This->saved_bufs[n] == NULL);
   This->saved_bufs[n] = tn5250_display_copy(This->dsp);

   outbuf[0] = 0x04;
   outbuf[1] = 0x12;
   outbuf[2] = (unsigned char) n;
   outbuf[3] = tn5250_table_save(This->table);

   TN5250_LOG(("SaveScreen: display buffer = %d; format buffer = %d\n",
	outbuf[2], outbuf[3]));

   tn5250_stream_send_packet(This->stream, 4, TN5250_RECORD_FLOW_DISPLAY, TN5250_RECORD_H_NONE,
			     TN5250_RECORD_OPCODE_SAVE_SCR, outbuf);
}

static void tn5250_session_restore_screen(Tn5250Session * This)
{
   int screen, format;

   TN5250_LOG(("RestoreScreen: entered.\n"));

   screen = tn5250_record_get_byte(This->record);
   format = tn5250_record_get_byte(This->record);

   TN5250_LOG(("RestoreScreen: screen = %d; format = %d\n", screen, format));

   TN5250_ASSERT(screen < (int) (sizeof(This->saved_bufs) / sizeof(Tn5250Display *)));
   TN5250_ASSERT(This->saved_bufs[screen] != NULL);
   tn5250_display_destroy(This->dsp);
   This->dsp = This->saved_bufs[screen];
   This->saved_bufs[screen] = NULL;

   if (This->table != NULL) {
      This->table->display = This->dsp;
      tn5250_table_restore(This->table, format);
   }
   if (This->term != NULL)
      tn5250_terminal_update(This->term, This->dsp);
}

static void tn5250_session_message_on(Tn5250Session * This)
{
   TN5250_LOG(("MessageOn: entered.\n"));
   tn5250_display_indicator_set(This->dsp, TN5250_DISPLAY_IND_MESSAGE_WAITING);
   if (This->term != NULL)
      tn5250_terminal_update_indicators(This->term, This->dsp);
}

static void tn5250_session_message_off(Tn5250Session * This)
{
   TN5250_LOG(("MessageOff: entered.\n"));
   tn5250_display_indicator_clear(This->dsp,
				  TN5250_DISPLAY_IND_MESSAGE_WAITING);
   if (This->term != NULL)
      tn5250_terminal_update_indicators(This->term, This->dsp);
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

   tn5250_display_roll(This->dsp, top, bot, lines);
   if (This->term != NULL)
      tn5250_terminal_update(This->term, This->dsp);
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

   tn5250_display_indicator_set(This->dsp, TN5250_DISPLAY_IND_X_SYSTEM);

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
   tn5250_display_addch(This->dsp, cur_char);

   Length1 = tn5250_record_get_byte(This->record);
   Length2 = tn5250_record_get_byte(This->record);

   if (input_field) {
      /* FIXME: `This address is either determined by the contents of the preceding SBA
       *  order or calculated from the field length parameter in the last SF order.'
       *  5494 Functions Reference.  -JMF */
      X = tn5250_display_cursor_x(This->dsp);
      Y = tn5250_display_cursor_y(This->dsp);

      if ((field = tn5250_table_field_yx (This->table, Y, X)) != NULL) {
	 TN5250_LOG(("StartOfField: Modifying field.\n"));
	 if (tn5250_field_start_col (field) == X+1 && tn5250_field_start_row (field) == Y+1) {
	    field->FFW = (FFW1 << 8) | FFW2;
	    field->attribute = Attr;
	 }
      } else {
	 TN5250_LOG(("StartOfField: Adding field.\n"));
	 field = tn5250_field_new (This->dsp);
	 TN5250_ASSERT(field != NULL);

	 field->FFW = (FFW1 << 8) | FFW2;
	 field->FCW = (FCW1 << 8) | FCW2;
	 field->attribute = Attr;
	 field->length =(Length1 << 8) | Length2;
	 field->start_row = Y + 1;
	 field->start_col = X + 1;

	 field->data = (unsigned char *)malloc (field->length);
	 TN5250_ASSERT(field->data != NULL);
	 memset (field->data, 0, field->length);
	 tn5250_table_add_field (This->table, field);
      }
   } else {
      TN5250_LOG(("StartOfField: Output only field.\n"));
      field = NULL;
   }

   if (input_field) {
      TN5250_ASSERT (field != NULL);
      endrow = tn5250_field_end_row(field);
      endcol = tn5250_field_end_col(field);
      if (endcol == tn5250_display_width(This->dsp)) { 
	 endcol = 1;
	 if (endrow == tn5250_display_height(This->dsp))
	    endrow = 1;
	 else
	    endrow++;
      } else 
	 endcol++;

      TN5250_LOG(("StartOfField: endrow = %d; endcol = %d\n", endrow, endcol));

#ifndef NDEBUG
      tn5250_field_dump(field);
#endif

      tn5250_display_cursor_set(This->dsp, endrow - 1, endcol - 1);
      tn5250_display_addch(This->dsp, 0x20);
      tn5250_display_cursor_set(This->dsp, Y, X);
   } 
}

static void tn5250_session_start_of_header(Tn5250Session * This)
{
   int length;
   int count;
   unsigned char temp;

   TN5250_LOG(("StartOfHeader: entered.\n"));

   tn5250_display_indicator_set(This->dsp, TN5250_DISPLAY_IND_X_SYSTEM);

   length = tn5250_record_get_byte(This->record);

   TN5250_LOG(("StartOfHeader: length = %d\nStartOfHeader: data = ", length));
   for (count = 0; count < length; count++) {
      temp = tn5250_record_get_byte(This->record);
      TN5250_LOG(("0x%02X ", temp));
      if (count == 3)
	 tn5250_table_set_message_line(This->table, temp);
   }
   TN5250_LOG(("\n"));

   tn5250_table_clear(This->table);
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

   tn5250_display_cursor_set(This->dsp, Y - 1, X - 1);
   TN5250_LOG(("SetBufferAddress: row = %d; col = %d\n", Y, X));
}

static void tn5250_session_repeat_to_address(Tn5250Session * This)
{
   unsigned char temp[4];
   int X, Y, curcol, count;

   TN5250_LOG(("RepeatToAddress: entered.\n"));

   temp[0] = tn5250_record_get_byte(This->record);
   temp[1] = tn5250_record_get_byte(This->record);
   temp[2] = tn5250_record_get_byte(This->record);

   TN5250_LOG(("RepeatToAddress: row = %d; col = %d; char = 0x%02X\n",
	temp[0], temp[1], temp[2]));

   X = tn5250_display_cursor_x(This->dsp) + 1;
   Y = tn5250_display_cursor_y(This->dsp) + 1;

   TN5250_LOG(("RepeatToAddress: currow = %d\n", Y));

   count = temp[1] - X + 1 + (temp[0] - Y) * tn5250_display_width(This->dsp);

   for (curcol = 0; curcol < count; curcol++)
      tn5250_display_addch(This->dsp, temp[2]);
}

static void tn5250_session_insert_cursor(Tn5250Session * This)
{
   int cur_char1, cur_char2;

   TN5250_LOG(("InsertCursor: entered.\n"));

   cur_char1 = tn5250_record_get_byte(This->record);
   cur_char2 = tn5250_record_get_byte(This->record);

   TN5250_LOG(("InsertCursor: row = %d; col = %d\n", cur_char1, cur_char2));

   tn5250_display_set_temp_ic(This->dsp, cur_char1 - 1, cur_char2 - 1);
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

   buffer_size = tn5250_display_width(This->dsp) *
       tn5250_display_height(This->dsp);

   buffer = (unsigned char *) malloc(buffer_size);
   TN5250_ASSERT(buffer != NULL);

   for (row = 0; row < tn5250_display_height(This->dsp); row++) {
      for (col = 0; col < tn5250_display_width(This->dsp); col++) {
	 buffer[row * tn5250_display_width(This->dsp) + col]
	     = tn5250_display_char_at(This->dsp, row, col);
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
   CC2 = tn5250_record_get_byte(This->record);

   TN5250_LOG(("ReadInputFields: CC1 = 0x%02X; CC2 = 0x%02X\n", CC1, CC2));
   tn5250_display_indicator_clear(This->dsp,
				  TN5250_DISPLAY_IND_X_SYSTEM
				  | TN5250_DISPLAY_IND_X_CLOCK
				  | TN5250_DISPLAY_IND_INHIBIT);
   tn5250_terminal_update_indicators(This->term, This->dsp);
}

static void tn5250_session_read_mdt_fields(Tn5250Session * This)
{
   unsigned char CC1, CC2;

   TN5250_LOG(("ReadMDTFields: entered.\n"));

   This->read_opcode = CMD_READ_MDT_FIELDS;

   CC1 = tn5250_record_get_byte(This->record);
   CC2 = tn5250_record_get_byte(This->record);

   TN5250_LOG(("ReadMDTFields: CC1 = 0x%02X; CC2 = 0x%02X\n", CC1, CC2));
   tn5250_display_indicator_clear(This->dsp,
				  TN5250_DISPLAY_IND_X_SYSTEM
				  | TN5250_DISPLAY_IND_X_CLOCK
				  | TN5250_DISPLAY_IND_INHIBIT);
   tn5250_terminal_update_indicators(This->term, This->dsp);
}

/*
 *    Send a Copy To Printer request.
 */
static void tn5250_session_print(Tn5250Session * This)
{
   Tn5250Buffer buf;

   TN5250_LOG(("Print request: entered.\n"));

   tn5250_buffer_init(&buf);
   tn5250_buffer_append_byte(&buf, tn5250_display_cursor_y(This->dsp) + 1);
   tn5250_buffer_append_byte(&buf, tn5250_display_cursor_x(This->dsp) + 1);
   tn5250_buffer_append_byte(&buf, TN5250_SESSION_AID_PRINT);

   tn5250_stream_send_packet(This->stream, tn5250_buffer_length(&buf), 
                             TN5250_RECORD_FLOW_DISPLAY,
                             TN5250_RECORD_H_NONE,
                             TN5250_RECORD_OPCODE_NO_OP, 
                             tn5250_buffer_data(&buf));

   tn5250_buffer_free(&buf);
}

/* vi:set cindent sts=3 sw=3: */
