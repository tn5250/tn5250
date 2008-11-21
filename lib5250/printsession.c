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

#ifndef WIN32


static const struct response_code {
   const char * code;
   int retval;
   const char * text;
} response_codes[] = {
   { "I901", 1, "Virtual device has less function than source device." },
   { "I902", 1, "Session successfully started." },
   { "I906", 1, "Automatic sign-on requested, but not allowed.  A sign-on screen will follow." },
   { "2702", 0, "Device description not found." },
   { "2703", 0, "Controller description not found." },
   { "2777", 0, "Damaged device description." },
   { "8901", 0, "Device not varied on." },
   { "8902", 0, "Device not available." },
   { "8903", 0, "Device not valid for session." },
   { "8906", 0, "Session initiation failed." },
   { "8907", 0, "Session failure." },
   { "8910", 0, "Controller not valid for session." },
   { "8916", 0, "No matching device found." },
   { "8917", 0, "Not authorized to object." },
   { "8918", 0, "Job cancelled." },
   { "8920", 0, "Object partially damaged." },  /* As opposed to fully damaged? */
   { "8921", 0, "Communications error." },
   { "8922", 0, "Negative response received." }, /* From what?!? */
   { "8923", 0, "Start-up record built incorrectly." },
   { "8925", 0, "Creation of device failed." },
   { "8928", 0, "Change of device failed." },
   { "8929", 0, "Vary on or vary off failed." },
   { "8930", 0, "Message queue does not exist." },
   { "8934", 0, "Start up for S/36 WSF received." },
   { "8935", 0, "Session rejected." },
   { "8936", 0, "Security failure on session attempt." },
   { "8937", 0, "Automatic sign-on rejected." },
   { "8940", 0, "Automatic configuration failed or not allowed." },
   { "I904", 0, "Source system at incompatible release." },
   { NULL, 0, NULL }
};

static int tn5250_print_session_waitevent(Tn5250PrintSession * This);

/****f* lib5250/tn5250_print_session_new
 * NAME
 *    tn5250_print_session_new
 * SYNOPSIS
 *    ret = tn5250_print_session_new ();
 * INPUTS
 *    None
 * DESCRIPTION
 *    DOCUMENT ME!!!
 *****/
Tn5250PrintSession *tn5250_print_session_new()
{
   Tn5250PrintSession *This;
   
   This = tn5250_new(Tn5250PrintSession, 1);
   if (This == NULL)
	   return NULL;

   This->rec = tn5250_record_new();
   if (This->rec == NULL) {
	   free (This);
	   return NULL;
   }

   This->stream = NULL;
   This->printfile = NULL;
   This->output_cmd = NULL;
   This->conn_fd = -1;
   This->map = NULL;
   This->script_slot = NULL;

   return This;
}

/****f* lib5250/tn5250_print_session_destroy
 * NAME
 *    tn5250_print_session_destroy
 * SYNOPSIS
 *    tn5250_print_session_destroy (This);
 * INPUTS
 *    Tn5250PrintSession * This       - 
 * DESCRIPTION
 *    DOCUMENT ME!!!
 *****/
void tn5250_print_session_destroy(Tn5250PrintSession * This)
{
   if (This->stream != NULL)
      tn5250_stream_destroy(This->stream);
   if (This->rec != NULL)
      tn5250_record_destroy(This->rec);
   if (This->output_cmd != NULL)
      free(This->output_cmd);
   if (This->map != NULL)
      tn5250_char_map_destroy (This->map);
   free (This);
}

/****f* lib5250/tn5250_print_session_set_fd
 * NAME
 *    tn5250_print_session_set_fd
 * SYNOPSIS
 *    tn5250_print_session_set_fd (This, fd);
 * INPUTS
 *    Tn5250PrintSession * This       - 
 *    SOCKET_TYPE          fd         - 
 * DESCRIPTION
 *    DOCUMENT ME!!!
 *****/
void tn5250_print_session_set_fd(Tn5250PrintSession * This, SOCKET_TYPE fd)
{
   This->conn_fd = fd;
}

/****f* lib5250/tn5250_print_session_set_stream
 * NAME
 *    tn5250_print_session_set_stream
 * SYNOPSIS
 *    tn5250_print_session_set_stream (This, newstream);
 * INPUTS
 *    Tn5250PrintSession * This       - 
 *    Tn5250Stream *       newstream  - 
 * DESCRIPTION
 *    DOCUMENT ME!!!
 *****/
void tn5250_print_session_set_stream(Tn5250PrintSession * This, Tn5250Stream * newstream)
{
   if (This->stream != NULL)
      tn5250_stream_destroy (This->stream);
   This->stream = newstream;
}

/****f* lib5250/tn5250_print_session_set_char_map
 * NAME
 *    tn5250_print_session_set_char_map
 * SYNOPSIS
 *    tn5250_print_session_set_char_map (This, map);
 * INPUTS
 *    Tn5250PrintSession * This       - 
 *    const char *         map        -
 * DESCRIPTION
 *    Sets the current translation map for this print session.  This is
 *    used to translate response codes to something we can use.
 *****/
void tn5250_print_session_set_char_map (Tn5250PrintSession *This, const char *map)
{
   if (This->map != NULL)
      tn5250_char_map_destroy (This->map);
   This->map = tn5250_char_map_new (map);
}

/****f* lib5250/tn5250_print_session_set_output_command
 * NAME
 *    tn5250_print_session_set_output_command
 * SYNOPSIS
 *    tn5250_print_session_set_output_command (This, output_cmd);
 * INPUTS
 *    Tn5250PrintSession * This       - 
 *    const char *         output_cmd - 
 * DESCRIPTION
 *    DOCUMENT ME!!!
 *****/
void tn5250_print_session_set_output_command(Tn5250PrintSession * This, const char *output_cmd)
{
   if (This->output_cmd != NULL)
      free(This->output_cmd);
   This->output_cmd = (char *) malloc(strlen(output_cmd) + 1);
   strcpy(This->output_cmd, output_cmd);
}

/****f* lib5250/tn5250_print_session_get_response_code
 * NAME
 *    tn5250_print_session_get_response_code
 * SYNOPSIS
 *    rc = tn5250_print_session_get_response_code (This, code);
 * INPUTS
 *    Tn5250PrintSession * This       - 
 *    char *               code       - 
 * DESCRIPTION
 *    Retrieves the response code from the startup response record.  The 
 *    function returns 1 for a successful startup, and 0 otherwise.  On return,
 *    code contains the 5 character response code.
 *****/
int tn5250_print_session_get_response_code(Tn5250PrintSession * This, char *code)
{

   /* Offset of first byte of data after record variable-length header. */
   int o = 6 + tn5250_record_data(This->rec)[6];
   int i;

   for (i = 0; i < 4; i++) {
      if (This->map == NULL)
	 code[i] = tn5250_record_data(This->rec)[o+i+5];
      else {
	 code[i] = tn5250_char_map_to_local (This->map,
	       tn5250_record_data(This->rec)[o+i+5]
	       );
      }
   }

   code[4] = '\0';
   for (i = 0; i < sizeof (response_codes)/sizeof (struct response_code); i++) {
      if (!strcmp (response_codes[i].code, code)) {
         syslog(LOG_INFO, "%s : %s", response_codes[i].code, response_codes[i].text);
	 return response_codes[i].retval;
      }
   }
   return 0;
}

/****f* lib5250/tn5250_print_session_main_loop
 * NAME
 *    tn5250_print_session_main_loop
 * SYNOPSIS
 *    tn5250_print_session_main_loop (This);
 * INPUTS
 *    Tn5250PrintSession * This       - 
 * DESCRIPTION
 *    This function continually loops, waiting for print jobs from the AS/400.
 *    When it gets one, it sends it to the output command which was specified 
 *    on the command line.  If the host closes the socket, we exit.
 *****/
void tn5250_print_session_main_loop(Tn5250PrintSession * This)
{
   int pcount;
   int newjob;
   char responsecode[5];
   StreamHeader header;

   while (1) {
      if (tn5250_print_session_waitevent(This)) {
	 if( tn5250_stream_handle_receive(This->stream) ) {
	    pcount = tn5250_stream_record_count(This->stream);
	    if (pcount > 0) {
	       if (This->rec != NULL)
	          tn5250_record_destroy(This->rec);
	       This->rec = tn5250_stream_get_record(This->stream);
	       if (!tn5250_print_session_get_response_code(This, responsecode))
	          exit (1);
	       break;
	    }
	 }
	 else {
	    syslog(LOG_INFO, "Socket closed by host.");
	    exit(-1);
	 }
      }

   }
   
   
   newjob = 1;
   while (1) {
      if (tn5250_print_session_waitevent(This)) {
	 if( tn5250_stream_handle_receive(This->stream) ) {
	    pcount = tn5250_stream_record_count(This->stream);
	    if (pcount > 0) {
	       if (newjob) {
	          char *output_cmd;
	          if ((output_cmd = This->output_cmd) == NULL)
		     output_cmd = "scs2ascii |lpr";
	          This->printfile = popen(output_cmd, "w");
	          TN5250_ASSERT(This->printfile != NULL);
	          newjob = 0;
	       }
	       if (This->rec != NULL)
	          tn5250_record_destroy(This->rec);
	       This->rec = tn5250_stream_get_record(This->stream);

	       if(tn5250_record_opcode(This->rec)
		  == TN5250_RECORD_OPCODE_CLEAR) 
		 {
		   syslog(LOG_INFO, "Clearing print buffers");
		   continue;
		 }

	       header.h5250.flowtype = TN5250_RECORD_FLOW_CLIENTO;
	       header.h5250.flags    = TN5250_RECORD_H_NONE;
	       header.h5250.opcode   = TN5250_RECORD_OPCODE_PRINT_COMPLETE;

	       tn5250_stream_send_packet(This->stream, 0,
					 header,
					 NULL);

	       if (tn5250_record_length(This->rec) == 0x11) {
		 syslog(LOG_INFO, "Job Complete\n");
		 pclose(This->printfile);
		 newjob = 1;
	       } else {
		 while (!tn5250_record_is_chain_end(This->rec))
		   fprintf(This->printfile, "%c", 
			   tn5250_record_get_byte(This->rec));
	       }
	    }
	 }
	 else {
	    syslog(LOG_INFO, "Socket closed by host");
	    exit(-1);
	 }
      }
   }
}

/****f* lib5250/tn5250_print_session_waitevent
 * NAME
 *    tn5250_print_session_waitevent
 * SYNOPSIS
 *    ret = tn5250_print_session_waitevent (This);
 * INPUTS
 *    Tn5250PrintSession * This       - 
 * DESCRIPTION
 *    Calls select() to wait for data to arrive on the socket fdr.
 *    This is the socket being used by the print session to communicate
 *    with the AS/400.  There is no timeout, so the function will wait forever
 *    if no data arrives.
 *****/
static int tn5250_print_session_waitevent(Tn5250PrintSession * This)
{
   fd_set fdr;
   int result = 0;

   FD_ZERO(&fdr);
   FD_SET(This->conn_fd, &fdr);
   select(This->conn_fd + 1, &fdr, NULL, NULL, NULL);

   if (FD_ISSET(This->conn_fd, &fdr))
      result = 1;

   return result;
}

#endif /* ifndef WIN32 */
/* vi:set sts=3 sw=3: */








