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
#ifndef STREAM5250_H
#define STREAM5250_H

#if _WIN32
#include <winsock2.h> /* Need for SOCKET type.  GJS 3/3/2000 */
#endif

#ifdef HAVE_LIBSSL
#include <openssl/ssl.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

struct _Tn5250Config;

struct Tn5250Header {
    int flowtype;
    unsigned char flags;
    unsigned char opcode;
};

typedef struct Tn5250Header StreamHeader;

/****s* lib5250/Tn5250Stream
 * NAME
 *    Tn5250Stream
 * SYNOPSIS
 *    Tn5250Stream *str = tn5250_stream_open ("telnet:my.as400.com");
 *    Tn5250Record *rec;
 *    tn5250_stream_send_packet (str, 0, TN5250_RECORD_FLOW_DISPLAY,
 *	 TN5250_RECORD_H_NONE, TN5250_RECORD_OPCODE_PUT_GET, NULL);
 *    rec = tn5250_stream_get_record (str);
 *    tn5250_stream_disconnect (str);
 *    tn5250_stream_destroy (str);
 * DESCRIPTION
 *    Tn5250Stream is 'abstract', implementations currently reside in
 *    the telnetstr.c and debug.c source files.  A stream object
 *    manages the communications transport, such as TCP/IP.
 * SOURCE
 */
struct _Tn5250Stream;
typedef struct _Tn5250Stream Tn5250Stream;
/******/

extern Tn5250Stream /*@only@*/ /*@null@*/*
tn5250_stream_open(const char* to, struct _Tn5250Config* config);
extern int tn5250_stream_config(Tn5250Stream* This,
                                struct _Tn5250Config* config);
extern void tn5250_stream_destroy(Tn5250Stream /*@only@*/* This);
extern Tn5250Record /*@only@*/* tn5250_stream_get_record(Tn5250Stream* This);
#define tn5250_stream_connect(This, to)    (*(This->connect))((This), (to))
#define tn5250_stream_disconnect(This)     (*(This->disconnect))((This))
#define tn5250_stream_handle_receive(This) (*(This->handle_receive))((This))
#define tn5250_stream_send_packet(This, len, header, data)                     \
    (*(This->send_packet))((This), (len), (header), (data))

/* This should be a more flexible replacement for different NEW_ENVIRON
 * strings. */
extern void tn5250_stream_setenv(Tn5250Stream* This, const char* name,
                                 const char /*@null@*/* value);
extern void tn5250_stream_unsetenv(Tn5250Stream* This, const char* name);
extern /*@observer@*/ /*@null@*/ const char*
tn5250_stream_getenv(Tn5250Stream* This, const char* name);

#define tn5250_stream_record_count(This) ((This)->record_count)
extern int tn5250_stream_socket_handle(Tn5250Stream* This);

#ifdef __cplusplus
}

#endif
#endif
