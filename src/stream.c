/*
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
#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <malloc.h>
#include <string.h>
#include "buffer.h"
#include "record.h"
#include "stream.h"
#include "utility.h"

/* External declarations of initializers for each type of stream. */
extern int tn5250_telnet_stream_init (Tn5250Stream *This);
#ifndef NDEBUG
extern int tn5250_debug_stream_init (Tn5250Stream *This);
#endif

/* This structure and the stream_types[] array defines what types of
 * streams we can create. */
struct _Tn5250StreamType {
   char *prefix;
   int (* init) (Tn5250Stream *This);
};

typedef struct _Tn5250StreamType Tn5250StreamType;

static Tn5250StreamType stream_types[] = {
#ifndef NDEBUG
   { "debug:", tn5250_debug_stream_init },
#endif
   { "telnet:", tn5250_telnet_stream_init },
   { "tn5250:", tn5250_telnet_stream_init },
   { NULL, NULL }
};

Tn5250Stream *tn5250_stream_open (const char *to)
{
   Tn5250Stream *This = tn5250_new(Tn5250Stream, 1);
   Tn5250StreamType *iter;
   const char *postfix;
   int ret;

   if (This != NULL) {
      This->connect = NULL;
      This->disconnect = NULL;
      This->handle_receive = NULL;
      This->send_packet = NULL;
      This->destroy = NULL;
      This->record_count = 0;
      This->records = This->current_record = NULL;
      This->sockfd = (SOCKET_TYPE) - 1;
      This->environ = NULL;
      tn5250_buffer_init(&(This->sb_buf));

      /* Figure out the stream type. */
      iter = stream_types;
      while (iter->prefix != NULL) {
	 if (strlen (to) >= strlen(iter->prefix)
	       && !memcmp (iter->prefix, to, strlen (iter->prefix))) {
	    /* Found the stream type, initialize it. */
	    ret = (* (iter->init)) (This);
	    if (ret != 0) {
	       tn5250_stream_destroy (This);
	       return NULL;
	    }
	    break;
	 }
	 iter++;
      }

      /* If we haven't found a type, assume telnet. */
      if (iter->prefix == NULL) { 
	 ret = tn5250_telnet_stream_init (This);
	 if (ret != 0) {
	    tn5250_stream_destroy (This);
	    return NULL;
	 }
	 postfix = to;
      } else 
	 postfix = to + strlen (iter->prefix);

      /* Connect */
      ret = (* (This->connect)) (This, postfix);
      if (ret == 0)
	 return This;

      tn5250_stream_destroy (This);
   }
   return NULL;
}

void tn5250_stream_destroy(Tn5250Stream * This)
{
   Tn5250StreamVar *iter, *next;

   /* Call particular stream type's destroy handler. */
   if (This->destroy)
      (* (This->destroy)) (This);

   /* Free the environment. */
   if ((iter = This->environ) != NULL) {
      do {
	 next = iter->next;
	 if (iter->name != NULL)
	    free(iter->name);
	 if (iter->value != NULL)
	    free(iter->value);
	 free(iter);
	 iter = next;
      } while (iter != This->environ);
   }
   tn5250_buffer_free(&(This->sb_buf));
   tn5250_record_list_destroy(This->records);
   free(This);
}

Tn5250Record *tn5250_stream_get_record(Tn5250Stream * This)
{
   Tn5250Record *record;
   int offset;

   record = This->records;
   TN5250_ASSERT(This->record_count >= 1);
   TN5250_ASSERT(record != NULL);
   This->records = tn5250_record_list_remove(This->records, record);
   This->record_count--;

   TN5250_ASSERT(record->length >= 10);

   tn5250_record_set_flow_type(record, record->data[4], record->data[5]);
   tn5250_record_set_flags(record, record->data[7]);
   tn5250_record_set_opcode(record, record->data[9]);

   offset = 6 + record->data[6];

   TN5250_LOG(("tn5250_stream_get_record: offset = %d\n", offset));
   tn5250_record_set_header_length(record, offset);

   /* FIXME: This is rather silly.  We should just keep those
    * 10 bytes in the record, adjust how the record is accessed.
    */
   memmove(record->data, record->data + offset, record->length - offset);
   return record;
}

void tn5250_stream_query_reply(Tn5250Stream * This)
{
   unsigned char temp[61];

   temp[0] = 0x00;		/* Cursor Row/Column (set to zero) */

   temp[1] = 0x00;
   temp[2] = 0x88;		/* Inbound Write Structured Field Aid */

   temp[3] = 0x00;		/* Length of Query Reply */

   temp[4] = 0x3A;
   temp[5] = 0xD9;		/* Command class */

   temp[6] = 0x70;		/* Command type - Query */

   temp[7] = 0x80;		/* Flag byte */

   temp[8] = 0x06;		/* Controller hardware class */

   temp[9] = 0x00;
   temp[10] = 0x01;		/* Controller code level (Version 1 Release 1.0 */

   temp[11] = 0x01;
   temp[12] = 0x00;
   temp[13] = 0x00;		/* Reserved (set to zero) */

   temp[14] = 0x00;
   temp[15] = 0x00;
   temp[16] = 0x00;
   temp[17] = 0x00;
   temp[18] = 0x00;
   temp[19] = 0x00;
   temp[20] = 0x00;
   temp[21] = 0x00;
   temp[22] = 0x00;
   temp[23] = 0x00;
   temp[24] = 0x00;
   temp[25] = 0x00;
   temp[26] = 0x00;
   temp[27] = 0x00;
   temp[28] = 0x00;
   temp[29] = 0x01;		/* 5250 Display or 5250 emulation */

   temp[30] = tn5250_ascii2ebcdic('5');
   temp[31] = tn5250_ascii2ebcdic('2');
   temp[32] = tn5250_ascii2ebcdic('5');
   temp[33] = tn5250_ascii2ebcdic('1');
   temp[34] = tn5250_ascii2ebcdic('0');
   temp[35] = tn5250_ascii2ebcdic('1');
   temp[36] = tn5250_ascii2ebcdic('1');
   temp[37] = 0x02;		/* Keyboard ID (standard keyboard) */
   temp[38] = 0x00;		/* Extended keyboard ID */
   temp[39] = 0x00;		/* Reserved */
   temp[40] = 0x00;		/* Display serial number */

   temp[41] = 0x61;
   temp[42] = 0x50;
   temp[43] = 0x00;
   temp[44] = 0x01;		/* Maximum number of input fields (256) */

   temp[45] = 0x00;
   temp[46] = 0x00;		/* Reserved (set to zero) */

   temp[47] = 0x00;
   temp[48] = 0x00;
   temp[49] = 0x01;		/* Controller/Display Capability */

   temp[50] = 0x10;
   temp[51] = 0x00;		/* Reserved */

   temp[52] = 0x00;
   temp[53] = 0x00;
   temp[54] = 0x00;		/* Reserved (set to zero) */

   temp[55] = 0x00;
   temp[56] = 0x00;
   temp[57] = 0x00;
   temp[58] = 0x00;
   temp[59] = 0x00;
   temp[60] = 0x00;

   tn5250_stream_send_packet(This, 61, TN5250_RECORD_FLOW_DISPLAY, TN5250_RECORD_H_NONE,
		 TN5250_RECORD_OPCODE_NO_OP, (unsigned char *) &temp[0]);

}

/*
 *    Hmm, this should probably be moved to printsession.c.
 */
void tn5250_stream_print_complete(Tn5250Stream * This)
{
   tn5250_stream_send_packet(This, 0, TN5250_RECORD_FLOW_CLIENTO, TN5250_RECORD_H_NONE,
			     TN5250_RECORD_OPCODE_PRINT_COMPLETE, NULL);
}

/*
 *    Set an 'environment' string.  This is made to look like setenv().
 */
void tn5250_stream_setenv(Tn5250Stream * This, const char *name,
			  const char *value)
{
   Tn5250StreamVar *var;

   TN5250_ASSERT(name != NULL);
   if (value == NULL) {
      tn5250_stream_unsetenv(This, name);
      return;
   }
   /* Clear the variable if it exists already. */
   if (tn5250_stream_getenv(This, name) != NULL)
      tn5250_stream_unsetenv(This, name);

   /* Create a new list node for the variable, fill it in. */
   var = tn5250_new(Tn5250StreamVar, 1);
   if (var != NULL) {
      var->name = tn5250_new(char, strlen(name) + 1);
      if (var->name == NULL) {
	 free(var);
	 return;
      }
      strcpy(var->name, name);
      var->value = tn5250_new(char, strlen(value) + 1);
      if (var->value == NULL) {
	 free(var->name);
	 free(var);
	 return;
      }
      strcpy(var->value, value);
   }
   /* Add the variable node to the linked list. */
   if (This->environ == NULL)
      This->environ = var->next = var->prev = var;
   else {
      var->next = This->environ;
      var->prev = This->environ->prev;
      var->next->prev = var;
      var->prev->next = var;
   }
}

/*
 *    Retrieve the value of a 5250 environment string.
 */
char *tn5250_stream_getenv(Tn5250Stream * This, const char *name)
{
   Tn5250StreamVar *iter;

   if ((iter = This->environ) != NULL) {
      do {
	 if (!strcmp(iter->name, name))
	    return iter->value;
	 iter = iter->next;
      } while (iter != This->environ);
   }
   return NULL;
}

/*
 *    Unset a 5250 environment string.
 */
void tn5250_stream_unsetenv(Tn5250Stream * This, const char *name)
{
   Tn5250StreamVar *iter;

   if ((iter = This->environ) != NULL) {
      do {
	 if (!strcmp(iter->name, name)) {
	    if (iter == iter->next)
	       This->environ = NULL;
	    else {
	       if (iter == This->environ)
		  This->environ = This->environ->next;
	       iter->next->prev = iter->prev;
	       iter->prev->next = iter->next;
	    }
	    free(iter->name);
	    if (iter->value != NULL)
	       free(iter->value);
	    free(iter);
	    return;
	 }
	 iter = iter->next;
      } while (iter != This->environ);
   }
}

/* vi:set cindent sts=3 sw=3: */
