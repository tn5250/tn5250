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
#ifndef PRINTSESSION_H
#define PRINTSESSION_H

#ifdef __cplusplus
extern "C" {
#endif

   struct _Tn5250PrintSession {
      Tn5250Stream /*@null@*/ /*@owned@*/ *stream;
      Tn5250Record /*@owned@*/ *rec;
      int conn_fd;
      FILE /*@null@*/ *printfile;
      char /*@null@*/ *output_cmd;
   };

   typedef struct _Tn5250PrintSession Tn5250PrintSession;

   extern Tn5250PrintSession /*@only@*/ /*@null@*/ *tn5250_print_session_new();
   extern void tn5250_print_session_destroy(Tn5250PrintSession /*@only@*/ * This);
   extern void tn5250_print_session_set_fd(Tn5250PrintSession * This, SOCKET_TYPE fd);
   extern void tn5250_print_session_get_response_code(Tn5250PrintSession * This, char /*@out@*/ *code);
   extern void tn5250_print_session_set_stream(Tn5250PrintSession * This, Tn5250Stream /*@owned@*/ * s);
   extern void tn5250_print_session_set_output_command(Tn5250PrintSession * This, const char *output_cmd);
   extern void tn5250_print_session_main_loop(Tn5250PrintSession * This);

#define tn5250_print_session_stream(This) ((This)->stream)

#ifdef __cplusplus
}

#endif
#endif				/* PRINTSESSION_H */
