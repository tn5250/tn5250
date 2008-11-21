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
#include "tn5250-private.h"

/* External declarations of initializers for each type of stream. */
extern int tn5250_telnet_stream_init (Tn5250Stream *This);
extern int tn3270_telnet_stream_init (Tn5250Stream *This);
#ifdef HAVE_LIBSSL
extern int tn5250_ssl_stream_init (Tn5250Stream *This);
#endif
#ifndef NDEBUG
extern int tn5250_debug_stream_init (Tn5250Stream *This);
#endif

/* This structure and the stream_types[] array defines what types of
 * streams we can create. */
struct _Tn5250StreamType {
   const char *prefix;
   int (* init) (Tn5250Stream *This);
};

typedef struct _Tn5250StreamType Tn5250StreamType;

static const Tn5250StreamType stream_types[] = {
#ifndef NDEBUG
   { "debug:", tn5250_debug_stream_init },
#endif
   { "telnet:", tn5250_telnet_stream_init },
   { "tn5250:", tn5250_telnet_stream_init },
#ifdef HAVE_LIBSSL
   { "ssl:", tn5250_ssl_stream_init },
   { "telnet-ssl:", tn5250_ssl_stream_init },
   { "telnets:", tn5250_ssl_stream_init },
#endif
   { NULL, NULL }
};

static void streamInit(Tn5250Stream *This, long timeout)
{
  This->options = 0;
  This->status = 0;
  This->config = NULL;
  This->connect = NULL;
  This->disconnect = NULL;
  This->handle_receive = NULL;
  This->send_packet = NULL;
  This->destroy = NULL;
  This->record_count = 0;
  This->records = This->current_record = NULL;
  This->sockfd = (SOCKET_TYPE) - 1;
  This->msec_wait = timeout;
  This->streamtype = TN5250_STREAM;
  This->rcvbufpos = 0;
  This->rcvbuflen = -1;
  tn5250_buffer_init(&(This->sb_buf));
}

/****f* lib5250/tn5250_stream_open
 * NAME
 *    tn5250_stream_open
 * SYNOPSIS
 *    ret = tn5250_stream_open (to);
 * INPUTS
 *    const char *         to         - `URL' of host to connect to.
 *    Tn5250Config     *   config     - config to associate w/stream
 * DESCRIPTION
 *    Opens a 5250 stream to the specified host.  URL is of the format 
 *    [protocol]:host:[port], where protocol is currently one of the following:
 *
 *       telnet - connect using tn5250 protocol
 *       tn5250 - connect using tn5250 protocol
 *       debug  - read recorded session from debug file
 *
 *    This is maintained by a protocol -> function mapping.  Each protocol has
 *    an associated function which is responsible for initializing the stream.
 *    Stream initialization sets up protocol specific functions to handle
 *    communication with the host system.
 *
 *    The next protocol to add is SNA.  This will allow the emulator to use 
 *    APPC (LU 6.2) to establish a session with the AS/400.
 *****/
Tn5250Stream *tn5250_stream_open (const char *to, Tn5250Config *config)
{
   Tn5250Stream *This = tn5250_new(Tn5250Stream, 1);
   const Tn5250StreamType *iter;
   const char *postfix;
   int ret;

   if (This != NULL) {

      streamInit(This, 0);

      if (config != NULL) 
           tn5250_stream_config(This, config);

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

/****f* lib5250/tn5250_stream_host
 * NAME
 *    tn5250_stream_host
 * SYNOPSIS
 *    ret = tn5250_stream_host (masterSock);
 * INPUTS
 *    SOCKET_TYPE	masterSock	-	Master socket
 * DESCRIPTION
 *    DOCUMENT ME!!!
 *****/
Tn5250Stream *tn5250_stream_host (SOCKET_TYPE masterfd, long timeout,
				  int streamtype)
{
   Tn5250Stream *This = tn5250_new(Tn5250Stream, 1);
   int ret;

   if (This != NULL) {
      streamInit(This, timeout);
      if(streamtype == TN5250_STREAM) 
	{
	  /* Assume telnet stream type. */
	  ret = tn5250_telnet_stream_init (This);
	}
      else
	{
	  ret = tn3270_telnet_stream_init (This);
	}
      if (ret != 0) {
         tn5250_stream_destroy (This);
         return NULL;
      }
      /* Accept */
      printf("masterfd = %d\n", masterfd);
      ret = (* (This->accept)) (This, masterfd);
      if (ret == 0)
	 return This;

      tn5250_stream_destroy (This);
   }
   return NULL;
}


/****f* lib5250/tn5250_stream_config
 * NAME
 *    tn5250_stream_config
 * SYNOPSIS
 *    tn5250_stream_config (This, config);
 * INPUTS
 *    Tn5250Stream *       This       - Stream object.
 *    Tn5250Config *       config     - Configuration object.
 * DESCRIPTION
 *    Associates a stream with a configuration object.  The stream uses the
 *    configuration object at run time to determine how to operate.
 *****/
int tn5250_stream_config (Tn5250Stream *This, Tn5250Config *config)
{
   /* Always reference before unreferencing, in case it's the same
    * object. */
   tn5250_config_ref (config);
   if (This->config != NULL)
      tn5250_config_unref (This->config);
   This->config = config;
   return 0;
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
   /* Call particular stream type's destroy handler. */
   if (This->destroy)
      (* (This->destroy)) (This);

   /* Free the environment. */
   if (This->config != NULL)
      tn5250_config_unref (This->config);
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

   if(This->streamtype == TN5250_STREAM)
     {
       TN5250_ASSERT(tn5250_record_length(record)>= 10);
       offset = 6 + tn5250_record_data(record)[6];
     }
   else
     {
       offset = 0;
     }

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
   char *name_buf;
   if (This->config == NULL) {
      This->config = tn5250_config_new ();
      TN5250_ASSERT (This->config != NULL);
   }
   name_buf = (char*)malloc (strlen (name) + 10);
   strcpy (name_buf, "env.");
   strcat (name_buf, name);
   tn5250_config_set (This->config, name_buf, value);
   free (name_buf);
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
const char *tn5250_stream_getenv(Tn5250Stream * This, const char *name)
{
   char *name_buf;
   const char *val;

   if (This->config == NULL)
      return NULL;

   name_buf = (char*)malloc (strlen (name) + 10);
   strcpy (name_buf, "env.");
   strcat (name_buf, name);
   val = tn5250_config_get (This->config, name_buf);
   free (name_buf);
   return val;
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
   char *name_buf;
   if (This->config == NULL)
      return; /* Nothing to unset. */

   name_buf = (char*)malloc (strlen (name) + 10);
   strcpy (name_buf, "env.");
   strcat (name_buf, name);
   tn5250_config_unset (This->config, name_buf);
   free (name_buf);
}

int tn5250_stream_socket_handle (Tn5250Stream *This)
{
   return (int) This->sockfd;
}

/* vi:set sts=3 sw=3: */
