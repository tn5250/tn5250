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
#ifndef SESSION_H
#define SESSION_H

#ifdef __cplusplus
extern "C" {
#endif

/* Negative response codes */
#define TN5250_NR_INVALID_COMMAND        0x10030101
#define TN5250_NR_INVALID_CLEAR_UNIT_ALT 0x10030105
#define TN5250_NR_INVALID_SOH_LENGTH     0x1005012B
#define TN5250_NR_INVALID_ROW_COL_ADDR   0x10050122
#define TN5250_NR_INVALID_EXT_ATTR_TYPE  0x1005012C
#define TN5250_NR_INVALID_SF_CLASS_TYPE  0x10050111

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

/* These are pseudo-aid codes used by the display. */
#define TN5250_SESSION_AID_SYSREQ       -1
#define TN5250_SESSION_AID_ATTN         -2
#define TN5250_SESSION_AID_TESTREQ      -3


#define TN5250_SESSION_CTL_IC_ULOCK     0x40 /* ??? - Not in my (older) spec */
#define TN5250_SESSION_CTL_CLR_BLINK    0x20
#define TN5250_SESSION_CTL_SET_BLINK    0x10
#define TN5250_SESSION_CTL_UNLOCK       0x08
#define TN5250_SESSION_CTL_ALARM        0x04
#define TN5250_SESSION_CTL_MESSAGE_OFF	0x02
#define TN5250_SESSION_CTL_MESSAGE_ON	0x01

#define TN5250_SESSION_KB_SIZE	        100

   struct _Tn5250Display;
   struct _Tn5250Config;

/****s* lib5250/Tn5250Session
 * NAME
 *    Tn5250Session
 * SYNOPSIS
 *    Tn5250Session *sess = tn5250_session_new ();
 *    tn5250_session_set_stream(sess, stream);
 *    tn5250_display_set_session(display,sess);
 *    tn5250_session_main_loop(sess);
 *    tn5250_session_destroy(sess);
 * DESCRIPTION
 *    Manages the communications session with the host and parses 5250-
 *    protocol communications records into display manipulation commands.
 * SOURCE
 */
struct _Tn5250Session {
   struct _Tn5250Display *		display;
   
   int (* handle_aidkey) (struct _Tn5250Session *This, int aidcode);

   Tn5250Stream /*@owned@*/ /*@null@*/ *stream;
   Tn5250Record /*@owned@*/ *record;
   struct _Tn5250Config *config;
   int read_opcode;	/* Current read opcode. */
   int invited;
};

typedef struct _Tn5250Session Tn5250Session;
/******/

extern Tn5250Session /*@only@*/ *tn5250_session_new(void);
extern void tn5250_session_destroy(Tn5250Session /*@only@*/ * This);
extern int tn5250_session_config (Tn5250Session *This, struct _Tn5250Config *config);

extern void tn5250_session_set_stream(Tn5250Session * This, Tn5250Stream /*@only@*/ * newstream);
#define tn5250_session_stream(This) ((This)->stream)

extern void tn5250_session_main_loop(Tn5250Session * This);

#ifdef __cplusplus
}

#endif
#endif				/* SESSION_H */

/* vi:set cindent sts=3 sw=3: */
