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
#ifndef SESSION_H
#define SESSION_H

#ifdef __cplusplus
extern "C" {
#endif

#define TN5250_SESSION_AID_F1		0x31
#define TN5250_SESSION_AID_F2		0x32
#define TN5250_SESSION_AID_F3		0x33
#define TN5250_SESSION_AID_F4		0x34
#define TN5250_SESSION_AID_F5		0x35
#define TN5250_SESSION_AID_F6		0x36
#define TN5250_SESSION_AID_F7		0x37
#define TN5250_SESSION_AID_F8		0x38
#define TN5250_SESSION_AID_F9		0x39
#define TN5250_SESSION_AID_F10		0x3A
#define TN5250_SESSION_AID_F11		0x3B
#define TN5250_SESSION_AID_F12		0x3C
#define TN5250_SESSION_AID_F13		0xB1
#define TN5250_SESSION_AID_F14		0xB2
#define TN5250_SESSION_AID_F15		0xB3
#define TN5250_SESSION_AID_F16		0xB4
#define TN5250_SESSION_AID_F17		0xB5
#define TN5250_SESSION_AID_F18		0xB6
#define TN5250_SESSION_AID_F19		0xB7
#define TN5250_SESSION_AID_F20		0xB8
#define TN5250_SESSION_AID_F21		0xB9
#define TN5250_SESSION_AID_F22		0xBA
#define TN5250_SESSION_AID_F23		0xBB
#define TN5250_SESSION_AID_F24		0xBC
#define TN5250_SESSION_AID_CLEAR	0xBD
#define TN5250_SESSION_AID_ENTER	0xF1
#define TN5250_SESSION_AID_HELP	        0xF3
#define TN5250_SESSION_AID_PGUP		0xF4
#define TN5250_SESSION_AID_PGDN		0xF5
#define TN5250_SESSION_AID_PRINT	0xF6
#define TN5250_SESSION_AID_RECORD_BS	0xF8

#define TN5250_SESSION_CTL_IC_ULOCK     0x02
#define TN5250_SESSION_CTL_CLR_BLINK    0x04
#define TN5250_SESSION_CTL_SET_BLINK    0x08
#define TN5250_SESSION_CTL_UNLOCK       0x10
#define TN5250_SESSION_CTL_ALARM        0x20
#define TN5250_SESSION_CTL_MESSAGE_OFF	0x40
#define TN5250_SESSION_CTL_MESSAGE_ON	0x80

#define TN5250_SESSION_KB_SIZE	        100

#define TN5250_SESSION_KEY_QUEUE_SIZE   100

   struct _Tn5250Session {
      Tn5250Terminal /*@owned@*/ /*@null@*/ *term;
      Tn5250Stream /*@owned@*/ /*@null@*/ *stream;
      Tn5250DBuffer /*@owned@*/ *dsp;
      Tn5250DBuffer *saved_bufs[256];
      Tn5250Record /*@owned@*/ *record;
      Tn5250Table /*@owned@*/ *table;
      int pending_insert;
      int read_opcode;	/* Current read opcode. */
      int invited;

      /* Queued keystroke ring buffer. */
      int key_queue_head, key_queue_tail;
      int key_queue[TN5250_SESSION_KEY_QUEUE_SIZE];
   };

   typedef struct _Tn5250Session Tn5250Session;

   extern Tn5250Session /*@only@*/ *tn5250_session_new(void);
   extern void tn5250_session_destroy(Tn5250Session /*@only@*/ * This);

   extern void tn5250_session_set_terminal(Tn5250Session * This, Tn5250Terminal /*@only@*/ * newterminal);
   extern void tn5250_session_set_stream(Tn5250Session * This, Tn5250Stream /*@only@*/ * newstream);
#define tn5250_session_terminal(This) ((This)->term)
#define tn5250_session_stream(This) ((This)->stream)

   extern void tn5250_session_main_loop(Tn5250Session * This);

#ifdef __cplusplus
}

#endif
#endif				/* SESSION_H */
