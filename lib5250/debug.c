/* TN5250
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
#include "config.h"

#ifndef NDEBUG

#define _TN5250_TERMINAL_PRIVATE_DEFINED

#if defined(WINE) || defined(WIN32)
#undef VERSION
#include <windows.h>
#include <winsock.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "utility.h"
#include "buffer.h"
#include "record.h"
#include "stream.h"
#include "dbuffer.h"
#include "menu.h"
#include "terminal.h"
#include "field.h"
#include "display.h"
#include "debug.h"

struct _Tn5250TerminalPrivate {
   Tn5250Stream *dbgstream;
   Tn5250Terminal *slaveterm;
   int keyq;
   int pauseflag;
};

typedef struct _Tn5250TerminalPrivate Tn5250TerminalPrivate;

static int debug_stream_connect(Tn5250Stream * This, const char *to);
static void debug_stream_disconnect(Tn5250Stream * This);
static int debug_stream_handle_receive(Tn5250Stream * This);
static void debug_stream_send_packet(Tn5250Stream * This, int length, 
				     StreamHeader header, unsigned char *data);static void debug_stream_destroy(Tn5250Stream *This);

static void debug_terminal_init(Tn5250Terminal *This);
static void debug_terminal_term(Tn5250Terminal *This);
static void debug_terminal_destroy(Tn5250Terminal /*@only@*/ *This);
static int debug_terminal_width(Tn5250Terminal *This);
static int debug_terminal_height(Tn5250Terminal *This);
static int debug_terminal_flags(Tn5250Terminal *This);
static void debug_terminal_update(Tn5250Terminal *This, Tn5250Display *display);
static void debug_terminal_update_indicators(Tn5250Terminal *This, Tn5250Display *display);
static int debug_terminal_waitevent(Tn5250Terminal *This);
static int debug_terminal_getkey(Tn5250Terminal *This);
static void debug_terminal_beep(Tn5250Terminal *This);

/****f* lib5250/tn5250_debug_stream_init
 * NAME
 *    tn5250_debug_stream_init
 * SYNOPSIS
 *    ret = tn5250_debug_stream_init (This);
 * INPUTS
 *    Tn5250Stream *       This       - 
 * DESCRIPTION
 *    DOCUMENT ME!!!
 *****/
int tn5250_debug_stream_init (Tn5250Stream *This)
{
   This->connect = debug_stream_connect;
   This->disconnect = debug_stream_disconnect;
   This->handle_receive = debug_stream_handle_receive;
   This->send_packet = debug_stream_send_packet;
   This->destroy = debug_stream_destroy;
   This->debugfile = NULL;
   return 0; /* Ok */
}

/****f* lib5250/tn5250_debug_terminal_new
 * NAME
 *    tn5250_debug_terminal_new
 * SYNOPSIS
 *    ret = tn5250_debug_terminal_new (slave, dbgstream);
 * INPUTS
 *    Tn5250Terminal *     slave      - 
 *    Tn5250Stream *       dbgstream  - 
 * DESCRIPTION
 *    DOCUMENT ME!!!
 *****/
Tn5250Terminal *tn5250_debug_terminal_new (Tn5250Terminal *slave, Tn5250Stream *dbgstream)
{
   Tn5250Terminal *This = tn5250_new(Tn5250Terminal, 1);
   if (This != NULL) {
      This->conn_fd = -1;
      This->init = debug_terminal_init;
      This->term = debug_terminal_term;
      This->destroy = debug_terminal_destroy;
      This->width = debug_terminal_width;
      This->height = debug_terminal_height;
      This->flags = debug_terminal_flags;
      This->update = debug_terminal_update;
      This->update_indicators = debug_terminal_update_indicators;
      This->waitevent = debug_terminal_waitevent;
      This->getkey = debug_terminal_getkey;
      This->beep = debug_terminal_beep;
      This->config = NULL;

      This->data = tn5250_new(Tn5250TerminalPrivate, 1);
      if (This->data == NULL) {
	 free (This);
	 return NULL;
      }
      This->data->dbgstream = dbgstream;
      This->data->slaveterm = slave;
      This->data->keyq = -1;
      This->data->pauseflag = 1;
   }
   return This;
}

/****f* lib5250/tn5250_debug_terminal_set_pause
 * NAME
 *    tn5250_debug_terminal_set_pause
 * SYNOPSIS
 *    tn5250_debug_terminal_set_pause (This, f);
 * INPUTS
 *    Tn5250Terminal *     This       - 
 *    int                  f          - 
 * DESCRIPTION
 *    DOCUMENT ME!!!
 *****/
void tn5250_debug_terminal_set_pause (Tn5250Terminal *This, int f)
{
   This->data->pauseflag = f;
}

/****i* lib5250/debug_stream_connect
 * NAME
 *    debug_stream_connect
 * SYNOPSIS
 *    ret = debug_stream_connect (This, to);
 * INPUTS
 *    Tn5250Stream *       This       - 
 *    const char *         to         - 
 * DESCRIPTION
 *    DOCUMENT ME!!!
 *****/
static int debug_stream_connect(Tn5250Stream * This, const char *to)
{
   This->debugfile = fopen (to, "r");
   if (This->debugfile == NULL)
      return -1;
   return 0;
}

/****i* lib5250/debug_stream_disconnect
 * NAME
 *    debug_stream_disconnect
 * SYNOPSIS
 *    debug_stream_disconnect (This);
 * INPUTS
 *    Tn5250Stream *       This       - 
 * DESCRIPTION
 *    DOCUMENT ME!!!
 *****/
static void debug_stream_disconnect(Tn5250Stream * This)
{
   if (This->debugfile != NULL)
      fclose (This->debugfile);
}

/****i* lib5250/debug_stream_handle_receive
 * NAME
 *    debug_stream_handle_receive
 * SYNOPSIS
 *    ret = debug_stream_handle_receive (This);
 * INPUTS
 *    Tn5250Stream *       This       - 
 * DESCRIPTION
 *    DOCUMENT ME!!!
 *****/
static int debug_stream_handle_receive(Tn5250Stream * This)
{
   return 1;
}

static void debug_stream_send_packet(Tn5250Stream * This, int length, 
				     StreamHeader header, unsigned char *data)

{
   /* noop */
}

/****i* lib5250/debug_stream_destroy
 * NAME
 *    debug_stream_destroy
 * SYNOPSIS
 *    debug_stream_destroy (This);
 * INPUTS
 *    Tn5250Stream *       This       - 
 * DESCRIPTION
 *    DOCUMENT ME!!!
 *****/
static void debug_stream_destroy(Tn5250Stream *This)
{
   /* noop */
}

/****i* lib5250/debug_terminal_init
 * NAME
 *    debug_terminal_init
 * SYNOPSIS
 *    debug_terminal_init (This);
 * INPUTS
 *    Tn5250Terminal *     This       - 
 * DESCRIPTION
 *    DOCUMENT ME!!!
 *****/
static void debug_terminal_init(Tn5250Terminal *This)
{
   (* (This->data->slaveterm->init)) (This->data->slaveterm);
}

/****i* lib5250/debug_terminal_term
 * NAME
 *    debug_terminal_term
 * SYNOPSIS
 *    debug_terminal_term (This);
 * INPUTS
 *    Tn5250Terminal *     This       - 
 * DESCRIPTION
 *    DOCUMENT ME!!!
 *****/
static void debug_terminal_term(Tn5250Terminal *This)
{
   (* (This->data->slaveterm->term)) (This->data->slaveterm);
}

/****i* lib5250/debug_terminal_destroy
 * NAME
 *    debug_terminal_destroy
 * SYNOPSIS
 *    debug_terminal_destroy (This);
 * INPUTS
 *    Tn5250Terminal *     This       - 
 * DESCRIPTION
 *    DOCUMENT ME!!!
 *****/
static void debug_terminal_destroy(Tn5250Terminal /*@only@*/ *This)
{
   (* (This->data->slaveterm->destroy)) (This->data->slaveterm);
   free (This->data);
   free (This);
}

/****i* lib5250/debug_terminal_width
 * NAME
 *    debug_terminal_width
 * SYNOPSIS
 *    ret = debug_terminal_width (This);
 * INPUTS
 *    Tn5250Terminal *     This       - 
 * DESCRIPTION
 *    DOCUMENT ME!!!
 *****/
static int debug_terminal_width(Tn5250Terminal *This)
{
   return (* (This->data->slaveterm->width)) (This->data->slaveterm);
}

/****i* lib5250/debug_terminal_height
 * NAME
 *    debug_terminal_height
 * SYNOPSIS
 *    ret = debug_terminal_height (This);
 * INPUTS
 *    Tn5250Terminal *     This       - 
 * DESCRIPTION
 *    DOCUMENT ME!!!
 *****/
static int debug_terminal_height(Tn5250Terminal *This)
{
   return (* (This->data->slaveterm->height)) (This->data->slaveterm);
}

/****i* lib5250/debug_terminal_flags
 * NAME
 *    debug_terminal_flags
 * SYNOPSIS
 *    ret = debug_terminal_flags (This);
 * INPUTS
 *    Tn5250Terminal *     This       - 
 * DESCRIPTION
 *    DOCUMENT ME!!!
 *****/
static int debug_terminal_flags(Tn5250Terminal *This)
{
   return (* (This->data->slaveterm->flags)) (This->data->slaveterm);
}

/****i* lib5250/debug_terminal_update
 * NAME
 *    debug_terminal_update
 * SYNOPSIS
 *    debug_terminal_update (This, display);
 * INPUTS
 *    Tn5250Terminal *     This       - 
 *    Tn5250Display *      display    - 
 * DESCRIPTION
 *    DOCUMENT ME!!!
 *****/
static void debug_terminal_update(Tn5250Terminal *This, Tn5250Display *display)
{
   (* (This->data->slaveterm->update)) (This->data->slaveterm, display);
}

/****i* lib5250/debug_terminal_update_indicators
 * NAME
 *    debug_terminal_update_indicators
 * SYNOPSIS
 *    debug_terminal_update_indicators (This, display);
 * INPUTS
 *    Tn5250Terminal *     This       - 
 *    Tn5250Display *      display    - 
 * DESCRIPTION
 *    DOCUMENT ME!!!
 *****/
static void debug_terminal_update_indicators(Tn5250Terminal *This, Tn5250Display *display)
{
   (* (This->data->slaveterm->update_indicators)) (This->data->slaveterm, display);
}

/****i* lib5250/debug_terminal_beep
 * NAME
 *    debug_terminal_beep
 * SYNOPSIS
 *    debug_terminal_beep (This);
 * INPUTS
 *    Tn5250Terminal *     This       - 
 * DESCRIPTION
 *    DOCUMENT ME!!!
 *****/
static void debug_terminal_beep(Tn5250Terminal *This)
{
   (* (This->data->slaveterm->beep)) (This->data->slaveterm);
}

/****i* lib5250/debug_terminal_waitevent
 * NAME
 *    debug_terminal_waitevent
 * SYNOPSIS
 *    ret = debug_terminal_waitevent (This);
 * INPUTS
 *    Tn5250Terminal *     This       - 
 * DESCRIPTION
 *    This is a hook for the Tn5250Terminal->waitevent method.
 *****/
static int debug_terminal_waitevent(Tn5250Terminal *This)
{
   char buf[256];
   int n;

   if (feof (This->data->dbgstream->debugfile))
      return (* (This->data->slaveterm->waitevent)) (This->data->slaveterm);

   while (fgets (buf, sizeof (buf)-2, This->data->dbgstream->debugfile)) {
      if (buf[0] != '@')
	 continue;

      if (!memcmp (buf, "@record ", 8)) {
	 if (This->data->dbgstream->current_record == NULL)
	    This->data->dbgstream->current_record = tn5250_record_new ();
	 for (n = 14; n < 49; n += 2) {
	    unsigned char b;

	    if (isspace (buf[n]))
	       n++;
	    if (isspace (buf[n]))
	       break;

	    b = (isdigit (buf[n]) ? (buf[n] - '0') : (tolower (buf[n]) - 'a' + 10)) << 4;
	    b |= (isdigit (buf[n+1]) ? (buf[n+1] - '0') : (tolower (buf[n+1]) - 'a' + 10));
	    tn5250_record_append_byte(This->data->dbgstream->current_record, b);
	 }
      } else if (!memcmp (buf, "@eor", 4)) {
	 if (This->data->dbgstream->current_record == NULL)
	    This->data->dbgstream->current_record = tn5250_record_new ();
	 if (This->data->dbgstream->records == NULL)
	    This->data->dbgstream->records
	       = This->data->dbgstream->current_record->prev
	       = This->data->dbgstream->current_record->next
	       = This->data->dbgstream->current_record;
	 else {
	    This->data->dbgstream->current_record->next = This->data->dbgstream->records;
	    This->data->dbgstream->current_record->prev = This->data->dbgstream->records->prev;
	    This->data->dbgstream->current_record->next->prev = This->data->dbgstream->current_record;
	    This->data->dbgstream->current_record->prev->next = This->data->dbgstream->current_record;
	 }
	 This->data->dbgstream->current_record = NULL;
	 This->data->dbgstream->record_count++;
	 return TN5250_TERMINAL_EVENT_DATA;
      } else if (!memcmp (buf, "@abort", 6)) {
	 /* It's useful to force a core dump sometimes... */
	 abort ();
      } else if (!memcmp (buf, "@key ", 5)) {
	 if (This->data->pauseflag)
	    (* (This->data->slaveterm->waitevent)) (This->data->slaveterm);
	 This->data->keyq = atoi (buf + 5);
	 return TN5250_TERMINAL_EVENT_KEY;
      }
   }
   
   /* EOF */
   return (* (This->data->slaveterm->waitevent)) (This->data->slaveterm);
}

/****i* lib5250/debug_terminal_getkey
 * NAME
 *    debug_terminal_getkey
 * SYNOPSIS
 *    ret = debug_terminal_getkey (This);
 * INPUTS
 *    Tn5250Terminal *     This       - 
 * DESCRIPTION
 *    DOCUMENT ME!!!
 *****/
static int debug_terminal_getkey(Tn5250Terminal *This)
{
   int ret = This->data->keyq;

   if (This->data->keyq == -1 && feof (This->data->dbgstream->debugfile))
      ret = (* (This->data->slaveterm->getkey)) (This->data->slaveterm);
   else
      (* (This->data->slaveterm->getkey)) (This->data->slaveterm);

   This->data->keyq = -1;
   return ret;
}

#endif /* NDEBUG */

