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
#ifndef PRINTSESSION_H
#define PRINTSESSION_H

#ifdef __cplusplus
extern "C" {
#endif

/****s* lib5250/Tn5250PrintSession
 * NAME
 *    Tn5250PrintSession
 * SYNOPSIS
 *    Tn5250PrintSession *ps = tn5250_print_session_new ();
 *    tn5250_print_session_set_stream(ps,stream);
 *    tn5250_print_session_set_output_command(ps,"scs2ascii | lpr");
 *    tn5250_print_session_main_loop(ps);
 *    tn5250_print_session_destroy(ps);
 * DESCRIPTION
 *    Manages a 5250e printer session and parses the data records.
 *****/
struct _Tn5250PrintSession {
   Tn5250Stream /*@null@*/ /*@owned@*/ *stream;
   Tn5250Record /*@owned@*/ *rec;
   int conn_fd;
   FILE /*@null@*/ *printfile;
   Tn5250CharMap *map;
   char /*@null@*/ *output_cmd;
   void *script_slot;
};

typedef struct _Tn5250PrintSession Tn5250PrintSession;

extern Tn5250PrintSession /*@only@*/ /*@null@*/ *tn5250_print_session_new();
extern void tn5250_print_session_destroy(Tn5250PrintSession /*@only@*/ * This);
extern void tn5250_print_session_set_fd(Tn5250PrintSession * This, SOCKET_TYPE fd);
extern int tn5250_print_session_get_response_code(Tn5250PrintSession * This, char /*@out@*/ *code);
extern void tn5250_print_session_set_stream(Tn5250PrintSession * This, Tn5250Stream /*@owned@*/ * s);
extern void tn5250_print_session_set_output_command(Tn5250PrintSession * This, const char *output_cmd);
extern void tn5250_print_session_set_char_map(Tn5250PrintSession * This, const char *map);
extern void tn5250_print_session_main_loop(Tn5250PrintSession * This);

#define tn5250_print_session_stream(This) ((This)->stream)

#ifdef __cplusplus
}

#endif
#endif				/* PRINTSESSION_H */

/* vi:set cindent sts=3 sw=3: */
