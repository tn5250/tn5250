/* TN5250 - An implementation of the 5250 telnet protocol.
 * Copyright (C) 2000 Michael Madore
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this software; see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 59 Temple Place, Suite 330,
 * Boston, MA 02111-1307 USA
 * 
 * As a special exception, the Free Software Foundation gives permission
 * for additional uses of the text contained in its release of TN5250.
 * 
 * The exception is that, if you link the TN5250 library with other files
 * to produce an executable, this does not by itself cause the
 * resulting executable to be covered by the GNU General Public License.
 * Your use of that executable is in no way restricted on account of
 * linking the TN5250 library code into it.
 * 
 * This exception does not however invalidate any other reasons why
 * the executable file might be covered by the GNU General Public License.
 * 
 * If you write modifications of your own for TN5250, it is your choice
 * whether to permit this exception to apply to your modifications.
 * If you do not wish that, delete this exception notice. */

#include "tn5250-private.h"
#include "host5250.h"

static struct sohPacket_t soh_pkt = {SOH,SOHLEN,0,0,0,ERR_ROW,{0,0,0}};
static char *mapname = MAP_DEFAULT;    

Tn5250Host *tn5250_host_new(Tn5250Stream *This)
{
  
  Tn5250Host *newHost = tn5250_new(Tn5250Host, 1);
  
  if (newHost != NULL) {
    memset(newHost, 0, sizeof(Tn5250Host));
    newHost->stream = This;
    tn5250_buffer_init(&newHost->buffer);
        set5250CharMap(newHost, mapname);
  }
    
  return newHost;
}

void set5250CharMap(Tn5250Host *This, const char *name)
{
  
  Tn5250CharMap *map = tn5250_char_map_new(name);
  TN5250_ASSERT(map != NULL);
  if (This->map != NULL)
    tn5250_char_map_destroy(This->map);
  This->map = map;
  
}

void writeToDisplay(Tn5250Host *This)
{
   Tn5250Buffer *buff;
   unsigned short ctrlWord=0;

   if (This->wtd_set)
      return;
   buff = &This->buffer;
   if (This->inputInhibited && !This->inSysInterrupt) {
      ctrlWord = TN5250_SESSION_CTL_SET_BLINK|TN5250_SESSION_CTL_UNLOCK;
      This->inputInhibited = FALSE;
   }
   tn5250_buffer_append_byte(buff, ESC);
   tn5250_buffer_append_byte(buff, CMD_WRITE_TO_DISPLAY);
   tn5250_buffer_append_byte(buff, (unsigned char) (ctrlWord>>8));
   tn5250_buffer_append_byte(buff, (unsigned char) (ctrlWord&0xFF));
   tn5250_buffer_append_data(buff, (unsigned char *) &soh_pkt, soh_pkt.len+2);
   This->clearState = FALSE;
   This->wtd_set = TRUE;
   This->lastattr = -1;
}

void appendBlock2Ebcdic(Tn5250Buffer * buff, 
			unsigned char *str, int len, 
			Tn5250CharMap * map)
{
   int i;
   unsigned char uc;

   for (uc=str[i=0]; i<len; uc=str[++i]) {
      if (isprint(uc))
	tn5250_buffer_append_byte(buff, 
				  tn5250_char_map_to_remote(map, uc));
   }
}

void sendReadMDT(Tn5250Stream *This, Tn5250Buffer * buff, 
		 unsigned short ctrlWord, unsigned char opcode)
{
  tn5250_buffer_append_byte(buff, ESC);
  tn5250_buffer_append_byte(buff, CMD_READ_MDT_FIELDS);
  tn5250_buffer_append_byte(buff, (unsigned char) (ctrlWord>>8));
  tn5250_buffer_append_byte(buff, (unsigned char) (ctrlWord&0xFF));
  tn5250_stream_send_packet(This, 
			    tn5250_buffer_length(buff),
			    TN5250_RECORD_FLOW_DISPLAY, 
			    TN5250_RECORD_H_NONE, opcode, 
			    tn5250_buffer_data(buff));
  tn5250_buffer_free(buff);
}

void 
setBufferAddr(Tn5250Host *This, int row, int col)
{
   writeToDisplay(This);
   tn5250_buffer_append_byte(&This->buffer, SBA);
   tn5250_buffer_append_byte(&This->buffer, (unsigned char) row);
   tn5250_buffer_append_byte(&This->buffer, (unsigned char) col);
}

void
SendTestScreen(Tn5250Host * This)
{

  int currow;

static char ascii_banner[][400]={
"                                                                  #####        ",
"                                                                 #######       ",
"                    @                                            ##O#O##       ",
"   ######          @@#                                           #VVVVV#       ",
"     ##             #                                          ##  VVV  ##     ",
"     ##         @@@   ### ####   ###    ###  ##### ######     #          ##    ",
"     ##        @  @#   ###    ##  ##     ##    ###  ##       #            ##   ",
"     ##       @   @#   ##     ##  ##     ##      ###         #            ###  ",
"     ##          @@#   ##     ##  ##     ##      ###        QQ#           ##Q  ",
"     ##       # @@#    ##     ##  ##     ##     ## ##     QQQQQQ#       #QQQQQQ",
"     ##      ## @@# #  ##     ##  ###   ###    ##   ##    QQQQQQQ#     #QQQQQQQ",
"   ############  ###  ####   ####   #### ### ##### ######   QQQQQ#######QQQQQ  "
"                                                                               ",
"                                                                               "
"                                TN5250E Server                                 "
};

 for(currow = 0; currow < 14; currow++) 
   { 
     setBufferAddr(This, currow+3, 1);
     appendBlock2Ebcdic(&This->buffer, ascii_banner[currow], 
			strlen(ascii_banner[currow]),
			This->map);
   }
 This->inputInhibited = This->inSysInterrupt = FALSE;
 sendReadMDT(This->stream, &This->buffer, 0, TN5250_RECORD_OPCODE_PUT_GET);

}



