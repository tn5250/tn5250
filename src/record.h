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
#ifndef RECORD_H
#define RECORD_H

#ifdef __cplusplus
extern "C" {
#endif

   struct _Tn5250Record {
      struct _Tn5250Record /*@dependent@*/ *prev;
      struct _Tn5250Record /*@dependent@*/ *next;

      unsigned char *data;
      int allocated, length;
      unsigned char flowtype[2];
      unsigned char flags;
      unsigned char opcode;
      int cur_pos;
      int header_length;
   };

   typedef struct _Tn5250Record Tn5250Record;

#define TN5250_RECORD_FLOW_DISPLAY 0x00
#define TN5250_RECORD_FLOW_STARTUP 0x90
#define TN5250_RECORD_FLOW_SERVERO 0x11
#define TN5250_RECORD_FLOW_CLIENTO 0x12

#define TN5250_RECORD_H_NONE		0
#define TN5250_RECORD_H_ERR		0x80
#define TN5250_RECORD_H_ATN		0x40
#define TN5250_RECORD_H_PRINTER_READY	0x20
#define TN5250_RECORD_H_FIRST_OF_CHAIN	0x10
#define TN5250_RECORD_H_LAST_OF_CHAIN	0x08
#define TN5250_RECORD_H_SRQ		0x04
#define TN5250_RECORD_H_TRQ		0x02
#define TN5250_RECORD_H_HLP		0x01

#define TN5250_RECORD_OPCODE_NO_OP		0
#define TN5250_RECORD_OPCODE_INVITE		1
#define TN5250_RECORD_OPCODE_OUTPUT_ONLY	2
#define TN5250_RECORD_OPCODE_PUT_GET		3
#define TN5250_RECORD_OPCODE_SAVE_SCR		4
#define TN5250_RECORD_OPCODE_RESTORE_SCR	5
#define TN5250_RECORD_OPCODE_READ_IMMED		6
#define TN5250_RECORD_OPCODE_READ_SCR		8
#define TN5250_RECORD_OPCODE_CANCEL_INVITE	10
#define TN5250_RECORD_OPCODE_MESSAGE_ON		11
#define TN5250_RECORD_OPCODE_MESSAGE_OFF	12
#define TN5250_RECORD_OPCODE_PRINT_COMPLETE	1
#define TN5250_RECORD_OPCODE_CLEAR		2

   extern Tn5250Record /*@only@*/ *tn5250_record_new(void);
   extern void tn5250_record_destroy(Tn5250Record /*@only@*/ * This);
   extern void tn5250_record_append_byte(Tn5250Record * This, unsigned char c);

   extern unsigned char tn5250_record_get_byte(Tn5250Record * This);
   extern void tn5250_record_unget_byte(Tn5250Record * This);
   extern void tn5250_record_set_flow_type(Tn5250Record * This, unsigned char byte1, unsigned char byte2);
#define tn5250_record_set_flags(This,newflags) (void)((This)->flags = (newflags))
#define tn5250_record_length(This) ((This)->length)

/* Should these be hidden? */
#define tn5250_record_set_opcode(This,newop) (void)((This)->opcode = (newop))
#define tn5250_record_set_header_length(This,newlen) (void)((This)->header_length = (newlen))
#define tn5250_record_opcode(This) ((This)->opcode)
#define tn5250_record_flow_type(This) (((This)->flowtype[0] << 8) | This->flowtype[1])
   extern int tn5250_record_is_chain_end(Tn5250Record * This);
#define tn5250_record_sys_request(This) (((This)->flags & TN5250_RECORD_H_SRQ) != 0)
#define tn5250_record_attention(This) (((This)->flags & TN5250_RECORD_H_ATN) != 0)

/* Manipulating lists of records (used in stream.c) */
   extern Tn5250Record /*@null@*/ *tn5250_record_list_add(Tn5250Record /*@null@*/ * list, Tn5250Record /*@dependent@*/ * record);
   extern Tn5250Record *tn5250_record_list_remove(Tn5250Record * list, Tn5250Record * record);
   extern Tn5250Record *tn5250_record_list_destroy(Tn5250Record /*@only@*/ /*@null@*/ * list);

#ifdef NDEBUG
#define tn5250_record_dump(This) (void)0
#else
   extern void tn5250_record_dump(Tn5250Record * This);
#endif

#ifdef __cplusplus
}

#endif
#endif				/* RECORD_H */
