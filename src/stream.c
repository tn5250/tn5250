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
#include "tn5250-private.h"

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

/****f* lib5250/tn5250_stream_open
 * NAME
 *    tn5250_stream_open
 * SYNOPSIS
 *    ret = tn5250_stream_open (to);
 * INPUTS
 *    const char *         to         - `URL' of host to connect to.
 * DESCRIPTION
 *    DOCUMENT ME!!!
 *****/
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

/****f* lib5250/tn5250_stream_destroy
 * NAME
 *    tn5250_stream_destroy
 * SYNOPSIS
 *    tn5250_stream_destroy (This);
 * INPUTS
 *    Tn5250Stream *       This       - 
 * DESCRIPTION
 *    DOCUMENT ME!!!
 *****/
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

/****f* lib5250/tn5250_stream_get_record
 * NAME
 *    tn5250_stream_get_record
 * SYNOPSIS
 *    ret = tn5250_stream_get_record (This);
 * INPUTS
 *    Tn5250Stream *       This       - 
 * DESCRIPTION
 *    DOCUMENT ME!!!
 *****/
Tn5250Record *tn5250_stream_get_record(Tn5250Stream * This)
{
   Tn5250Record *record;
   int offset;

   record = This->records;
   TN5250_ASSERT(This->record_count >= 1);
   TN5250_ASSERT(record != NULL);
   This->records = tn5250_record_list_remove(This->records, record);
   This->record_count--;

   TN5250_ASSERT(tn5250_record_length(record)>= 10);

   offset = 6 + tn5250_record_data(record)[6];

   TN5250_LOG(("tn5250_stream_get_record: offset = %d\n", offset));
   tn5250_record_set_cur_pos(record, offset);
   return record;
}


/****f* lib5250/tn5250_stream_setenv
 * NAME
 *    tn5250_stream_setenv
 * SYNOPSIS
 *    tn5250_stream_setenv (This, name, value);
 * INPUTS
 *    Tn5250Stream *       This       - 
 *    const char *         name       -
 *    const char *         value      -
 * DESCRIPTION
 *    Set an 'environment' string.  This is made to look like setenv().
 *****/
void tn5250_stream_setenv(Tn5250Stream * This, const char *name, const char *value)
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

/****f* lib5250/tn5250_stream_getenv
 * NAME
 *    tn5250_stream_getenv
 * SYNOPSIS
 *    ret = tn5250_stream_getenv (This, name);
 * INPUTS
 *    Tn5250Stream *       This       - 
 *    const char *         name       - 
 * DESCRIPTION
 *    Retrieve the value of a 5250 environment string.
 *****/
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

/****f* lib5250/tn5250_stream_unsetenv
 * NAME
 *    tn5250_stream_unsetenv
 * SYNOPSIS
 *    tn5250_stream_unsetenv (This, name);
 * INPUTS
 *    Tn5250Stream *       This       - 
 *    const char *         name       - 
 * DESCRIPTION
 *    Unset a 5250 environment string.
 *****/
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

