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
#ifndef RECORD_H
#define RECORD_H

#ifdef __cplusplus
extern "C" {
#endif

/****s* lib5250/Tn5250Record
 * NAME
 *    Tn5250Record
 * SYNOPSIS
 *    Tn5250Record *rec = tn5250_stream_get_record (stream);
 *    unsigned char b;
 *    while (!tn5250_record_is_chain_end (rec)) {
 *	 b = tn5250_record_get_byte (rec);
 *	 printf ("X'%02X' ", b);
 *    }
 *    tn5250_record_destroy (rec);
 * DESCRIPTION
 *    Handles a 5250-protocol communications record.
 * SOURCE
 */
struct _Tn5250Record {
   struct _Tn5250Record /*@dependent@*/ *prev;
   struct _Tn5250Record /*@dependent@*/ *next;

   Tn5250Buffer data;
   int cur_pos;
};

typedef struct _Tn5250Record Tn5250Record;
/******/

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

extern unsigned char tn5250_record_get_byte(Tn5250Record * This);
extern void tn5250_record_unget_byte(Tn5250Record * This);
extern int tn5250_record_is_chain_end(Tn5250Record * This);
extern void tn5250_record_skip_to_end(Tn5250Record * This);
#define tn5250_record_length(This) \
   tn5250_buffer_length (&((This)->data))
#define tn5250_record_append_byte(This,c) \
   tn5250_buffer_append_byte (&((This)->data),(c))
#define tn5250_record_data(This) \
   tn5250_buffer_data(&((This)->data))

/* Should this be hidden? */
#define tn5250_record_set_cur_pos(This,newpos) \
   (void)((This)->cur_pos = (newpos))
#define tn5250_record_opcode(This) \
   (tn5250_record_data(This)[9])
#define tn5250_record_flow_type(This) \
   ((tn5250_record_data(This)[4] << 8) | (tn5250_record_data(This)[5]))
#define tn5250_record_flags(This) \
   (tn5250_record_data(This)[7])
#define tn5250_record_sys_request(This) \
   ((tn5250_record_flags((This)) & TN5250_RECORD_H_SRQ) != 0)
#define tn5250_record_attention(This) \
   ((tn5250_record_flags((This)) & TN5250_RECORD_H_ATN) != 0)

/* Manipulating lists of records (used in stream.c) */
extern Tn5250Record /*@null@*/ *tn5250_record_list_add(Tn5250Record /*@null@*/ * list, Tn5250Record /*@dependent@*/ * record);
extern Tn5250Record *tn5250_record_list_remove(Tn5250Record * list, Tn5250Record * record);
extern Tn5250Record *tn5250_record_list_destroy(Tn5250Record /*@only@*/ /*@null@*/ * list);
extern void tn5250_record_dump(Tn5250Record * This);

#ifdef __cplusplus
}

#endif
#endif				/* RECORD_H */

/* vi:set cindent sts=3 sw=3: */
