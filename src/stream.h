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
#ifndef STREAM5250_H
#define STREAM5250_H

#ifdef __cplusplus
extern "C" {
#endif

   struct _Tn5250Stream {
      
      int (* connect) (struct _Tn5250Stream *This, const char *to);
      void (* disconnect) (struct _Tn5250Stream *This);
      int (* handle_receive) (struct _Tn5250Stream *This);
      void (* send_packet) (struct _Tn5250Stream *This, int length, int flowtype, unsigned char flags,
	    unsigned char opcode, unsigned char *data);
      void (/*@null@*/ * destroy) (struct _Tn5250Stream /*@only@*/ *This);

      Tn5250Record /*@null@*/ *records;
      Tn5250Record /*@dependent@*/ /*@null@*/ *current_record;
      int record_count;
      struct _Tn5250StreamVar /*@null@*/ *environ;

      Tn5250Buffer sb_buf;

      SOCKET_TYPE sockfd;
      int status;
      char /*@null@*/ *termtype;
      char /*@null@*/ *devicename;
      int state;

#ifndef NDEBUG
      FILE *debugfile;
#endif
   };

   struct _Tn5250StreamVar {
      struct _Tn5250StreamVar *next;
      struct _Tn5250StreamVar *prev;
      char *name;
      char *value;
   };

   typedef struct _Tn5250Stream Tn5250Stream;
   typedef struct _Tn5250StreamVar Tn5250StreamVar;

   extern Tn5250Stream /*@only@*/ /*@null@*/ *tn5250_stream_open (const char *to);
   extern void tn5250_stream_destroy(Tn5250Stream /*@only@*/ * This);
   extern Tn5250Record /*@only@*/ *tn5250_stream_get_record(Tn5250Stream * This);
   extern void tn5250_stream_print_complete(Tn5250Stream * This);
   extern void tn5250_stream_query_reply(Tn5250Stream * This);

#define tn5250_stream_connect(This,to) \
   (* (This->connect)) ((This),(to))
#define tn5250_stream_disconnect(This) \
   (* (This->disconnect)) ((This))
#define tn5250_stream_handle_receive(This) \
   (* (This->handle_receive)) ((This))
#define tn5250_stream_send_packet(This,len,flow,flags,opcode,data) \
   (* (This->send_packet)) ((This),(len),(flow),(flags),(opcode),(data))

/* This should be a more flexible replacement for different NEW_ENVIRON
 * strings. */
   extern void tn5250_stream_setenv(Tn5250Stream * This, const char *name,
				    const char /*@null@*/ *value);
   extern void tn5250_stream_unsetenv(Tn5250Stream * This, const char *name);
   extern /*@observer@*/ /*@null@*/ char *tn5250_stream_getenv(Tn5250Stream * This, const char *name);

#define tn5250_stream_record_count(This) ((This)->record_count)
#define tn5250_stream_socket_handle(This) ((This)->sockfd)

#ifdef __cplusplus
}

#endif
#endif

/* vi:set cindent sts=3 sw=3: */
