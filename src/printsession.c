#include "tn5250-config.h"
#include <stdio.h>
#include <malloc.h>
#include "buffer.h"
#include "record.h"
#include "stream.h"
#include <sys/time.h>
#include "printsession.h"
#include "utility.h"

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
   if (This->output_cmd != NULL)
	   strcpy(This->output_cmd, output_cmd);
}

/****f* lib5250/tn5250_print_session_get_response_code
 * NAME
 *    tn5250_print_session_get_response_code
 * SYNOPSIS
 *    tn5250_print_session_get_response_code (This, code);
 * INPUTS
 *    Tn5250PrintSession * This       - 
 *    char *               code       - 
 * DESCRIPTION
 *    DOCUMENT ME!!!
 *****/
void tn5250_print_session_get_response_code(Tn5250PrintSession * This, char *code)
{
   memcpy (code, tn5250_record_data(This->rec) + 5, 4);
   code[4] = '\0';
}

/****f* lib5250/tn5250_print_session_main_loop
 * NAME
 *    tn5250_print_session_main_loop
 * SYNOPSIS
 *    tn5250_print_session_main_loop (This);
 * INPUTS
 *    Tn5250PrintSession * This       - 
 * DESCRIPTION
 *    DOCUMENT ME!!!
 *****/
void tn5250_print_session_main_loop(Tn5250PrintSession * This)
{
   int pcount;
   int newjob;
   char responsecode[5];

   while (1) {
      if (tn5250_print_session_waitevent(This)) {
	 tn5250_stream_handle_receive(This->stream);
	 pcount = tn5250_stream_record_count(This->stream);
	 if (pcount > 0) {
	    if (This->rec != NULL)
	       tn5250_record_destroy(This->rec);
	    This->rec = tn5250_stream_get_record(This->stream);
	    tn5250_print_session_get_response_code(This, responsecode);
	    if (strcmp(responsecode, "I902")) {
	       printf("Could not establish printer session: %s\n",
		      responsecode);
	       exit(1);
	    } else {
	       printf("Printer session established.\n");
	       break;
	    }
	 }
      }
   }

   newjob = 1;
   while (1) {
      if (tn5250_print_session_waitevent(This)) {
	 tn5250_stream_handle_receive(This->stream);
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
	    tn5250_stream_send_packet(This->stream, 0,
		  TN5250_RECORD_FLOW_CLIENTO,
		  TN5250_RECORD_H_NONE,
		  TN5250_RECORD_OPCODE_PRINT_COMPLETE,
		  NULL);
	    if (tn5250_record_length(This->rec) == 0x11) {
	       printf("Job Complete\n");
	       pclose(This->printfile);
	       newjob = 1;
	    } else {
	       while (!tn5250_record_is_chain_end(This->rec))
		  fprintf(This->printfile, "%c", tn5250_record_get_byte(This->rec));
	    }
	 }
      }
   }
}

/****i* lib5250/tn5250_print_session_waitevent
 * NAME
 *    tn5250_print_session_waitevent
 * SYNOPSIS
 *    ret = tn5250_print_session_waitevent (This);
 * INPUTS
 *    Tn5250PrintSession * This       - 
 * DESCRIPTION
 *    DOCUMENT ME!!!
 *****/
static int tn5250_print_session_waitevent(Tn5250PrintSession * This)
{
   fd_set fdr;
   int result = 0;

   FD_ZERO(&fdr);
   FD_SET(0, &fdr);
   FD_SET(This->conn_fd, &fdr);
   select(This->conn_fd + 1, &fdr, NULL, NULL, NULL);

   if (FD_ISSET(This->conn_fd, &fdr))
      result = 1;

   return result;
}
