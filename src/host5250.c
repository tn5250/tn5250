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
 * As a special exception, the copyright holder gives permission
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
    newHost->maxcol = 80;
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
      ctrlWord = TN5250_SESSION_CTL_SET_BLINK |TN5250_SESSION_CTL_UNLOCK;
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
repeat2Addr(Tn5250Host *This, int row, int col, unsigned char uc)
{
   writeToDisplay(This);
   tn5250_buffer_append_byte(&This->buffer, RA);
   tn5250_buffer_append_byte(&This->buffer, (unsigned char) row);
   tn5250_buffer_append_byte(&This->buffer, (unsigned char) col);
   tn5250_buffer_append_byte(&This->buffer, uc);
}

void 
setAttribute(Tn5250Host * This, unsigned short attr)
{
   This->curattr = attr;
}

int 
readMDTfields(Tn5250Host *This, int sendRMF)
{
  fd_set rset;

  unsigned char flags;
  int aidCode=0;

  if (sendRMF) {
    unsigned short wtdCtrl=0;
    if (This->inputInhibited && !This->inSysInterrupt)
      wtdCtrl = TN5250_SESSION_CTL_SET_BLINK 
	| TN5250_SESSION_CTL_UNLOCK;
    sendReadMDT(This->stream, &This->buffer, wtdCtrl,
		TN5250_RECORD_OPCODE_PUT_GET);
  }
  This->wtd_set = FALSE;

  while (!aidCode) {
    FD_ZERO(&rset);
    FD_SET(This->stream->sockfd, &rset);

    if( select(This->stream->sockfd+1, &rset, NULL, NULL, NULL) < 0 ) {
      syslog(LOG_INFO, "select: %s\n", strerror(errno));
      exit(1);
    }

    if (!(tn5250_stream_handle_receive(This->stream))) {
      /* We got disconnected or something. */
      This->disconnected = TRUE;
      return -1;
    }
    if (This->record) {
      tn5250_record_destroy(This->record);
      This->record = NULL;
    }
    if (This->stream->record_count>0)
      This->record = tn5250_stream_get_record(This->stream);
    else
      continue;
    if (flags=tn5250_record_flags(This->record))
      aidCode = processFlags(This, flags, &This->record->data.data[10]);
    else if (This->record->data.len>10) {
      int hor, ver;
      ver = tn5250_record_get_byte(This->record) - 1;
      hor = tn5250_record_get_byte(This->record) - 1;
      This->cursorPos = ver*This->maxcol + hor;
      aidCode =  tn5250_record_get_byte(This->record);
    }
  }
  /* Return error or AID code */
  return aidCode;
}

int 
processFlags(Tn5250Host *This, unsigned char flags, unsigned char *buf)
{
   char *msg;
   short ecode;
   Tn5250Stream *myStream=This->stream;

   switch (flags) {
      case TN5250_RECORD_H_HLP:
		ecode = (short)buf[0]<<8 | (short)buf[1];
		if (!cancelInvite(myStream))
		   return -1;
		msg = getErrMsg(ecode);
		sendWriteErrorCode(This, msg,
			TN5250_RECORD_OPCODE_OUTPUT_ONLY);
		break;

      case TN5250_RECORD_H_ERR:
		/* Data stream output error */
		msg = processErr(buf);
		sendWriteErrorCode(This, msg,
			TN5250_RECORD_OPCODE_OUTPUT_ONLY);
		return -1;
		break;

      case TN5250_RECORD_H_TRQ:
		/* Test Request Key */
		break;

      case TN5250_RECORD_H_ATN:
      case TN5250_RECORD_H_SRQ:
		if (This->inSysInterrupt)
		   return 0;
		if (!cancelInvite(myStream))
		   return -1;
		This->inSysInterrupt = TRUE;
		This->wtd_set = FALSE;
		This->clearState = FALSE;
		tn5250_buffer_free(&This->buffer);
		clearScreen(This);
		processSRQ(myStream);
		This->wtd_set = FALSE;
		This->clearState = FALSE;
		This->inSysInterrupt = FALSE;
		tn5250_buffer_free(&This->buffer);
		break;

      default:
		break;

   } /* switch */

   return 0;
} /* processFlags */

int 
cancelInvite(Tn5250Stream *This)
{
   int statOK;

   tn5250_stream_send_packet(This, 0, TN5250_RECORD_FLOW_DISPLAY,
	TN5250_RECORD_H_NONE, TN5250_RECORD_OPCODE_CANCEL_INVITE, NULL);
   if (This->record_count) {
      This->records = tn5250_record_list_destroy(This->records);
      This->record_count = 0;
   }
   /* Get Cancel Invite acknowlegement from client. */
   do {
      statOK = (int)(tn5250_stream_handle_receive(This));
   } while (statOK && !This->record_count);
   if (This->record_count>0) { /* Zap the record(s) */
      This->records = tn5250_record_list_destroy(This->records);
      This->record_count = 0;
   }
   return statOK;
}

char 
*getErrMsg(short ecode)
{
   char *errmsg;
   switch (ecode) {
      case ERR_DONT_KNOW:
		errmsg = MSG_DONT_KNOW;
		break;
      case ERR_BYPASS_FIELD:
		errmsg = MSG_BYPASS_FIELD;
		break;
      case ERR_NO_FIELD:
		errmsg = MSG_NO_FIELD;
		break;
      case ERR_INVALID_SYSREQ:
		errmsg = MSG_INVALID_SYSREQ;
		break;
      case ERR_MANDATORY_ENTRY:
		errmsg = MSG_MANDATORY_ENTRY;
		break;
      case ERR_ALPHA_ONLY:
		errmsg = MSG_ALPHA_ONLY;
		break;
      case ERR_NUMERIC_ONLY:
		errmsg = MSG_NUMERIC_ONLY;
		break;
      case ERR_DIGITS_ONLY:
		errmsg = MSG_DIGITS_ONLY;
		break;
      case ERR_LAST_SIGNED:
		errmsg = MSG_LAST_SIGNED;
		break;
      case ERR_NO_ROOM:
		errmsg = MSG_NO_ROOM;
		break;
      case ERR_MANADATORY_FILL:
		errmsg = MSG_MANADATORY_FILL;
		break;
      case ERR_CHECK_DIGIT:
		errmsg = MSG_CHECK_DIGIT;
		break;
      case ERR_NOT_SIGNED:
		errmsg = MSG_NOT_SIGNED;
		break;
      case ERR_EXIT_NOT_VALID:
		errmsg = MSG_EXIT_NOT_VALID;
		break;
      case ERR_DUP_NOT_ENABLED:
		errmsg = MSG_DUP_NOT_ENABLED;
		break;
      case ERR_NO_FIELD_EXIT:
		errmsg = MSG_NO_FIELD_EXIT;
		break;
      case ERR_NO_INPUT:
		errmsg = MSG_NO_INPUT;
		break;
      case ERR_BAD_CHAR:
		errmsg = MSG_BAD_CHAR;
		break;
#ifdef JAPAN
      case ERR_DBCS_WRONG_TYPE:
		errmsg = MSG_DBCS_WRONG_TYPE;
		break;
      case ERR_SBCS_WRONG_TYPE:
		errmsg = MSG_SBCS_WRONG_TYPE;
		break;
#endif
      default:
		errmsg = MSG_NO_HELP;
		break;
   } /* switch */
   return errmsg;
}

void 
sendWriteErrorCode(Tn5250Host *This, char *msg, unsigned char opcode)
{
   Tn5250Buffer tbuf;

   tn5250_buffer_init(&tbuf);
   tn5250_buffer_append_byte(&tbuf, ESC);
   tn5250_buffer_append_byte(&tbuf, CMD_WRITE_ERROR_CODE);
   hiliteString(&tbuf, msg, This->map);
   if (opcode==TN5250_RECORD_OPCODE_OUTPUT_ONLY) {
      tn5250_stream_send_packet(This->stream, tn5250_buffer_length(&tbuf),
		TN5250_RECORD_FLOW_DISPLAY, TN5250_RECORD_H_NONE, opcode, 
		tn5250_buffer_data(&tbuf));
      tbuf.len = 0;
      opcode = TN5250_RECORD_OPCODE_INVITE;
   }
   sendReadMDT(This->stream, &tbuf, 0, opcode);
}

typedef struct {
   int code;
   char *msg;
} DSNRTABLE;

static DSNRTABLE dsnrMsgTable[]= {
	{ DSNR_RESEQ_ERR,	EMSG_RESEQ_ERR },
	{ DSNR_INVCURSPOS,	EMSG_INVCURSPOS },
	{ DSNR_RAB4WSA,		EMSG_RAB4WSA },
	{ DSNR_INVSFA,		EMSG_INVSFA },
	{ DSNR_FLDEOD,		EMSG_FLDEOD },
	{ DSNR_FMTOVF,		EMSG_FMTOVF },
	{ DSNR_WRTEOD,		EMSG_WRTEOD },
	{ DSNR_SOHLEN,		EMSG_SOHLEN },
	{ DSNR_ROLLPARM,	EMSG_ROLLPARM },
	{ DSNR_NO_ESC,		EMSG_NO_ESC },
	{ DSNR_INV_WECW,	EMSG_INV_WECW },
	{ DSNR_UNKNOWN,		NULL }
};

char 
*processErr(unsigned char *buf)
{
   static char invCmd[]="Invalid command encountered in data stream.",
	unkfmt[]="Unknown data stream error: 0x%04X: %02X %02X",
	mbuf[80]="";
   short catmod;
   unsigned char ubyte1;
   int dsnrCode, i=0;

   catmod = (short)buf[0]<<8 | buf[1];
   ubyte1 = buf[2];
   dsnrCode = (int) buf[3];
   if (catmod==0x1003 && ubyte1==1 && dsnrCode==1)
      return invCmd;
   if (catmod!=0x1005 || ubyte1!=1) {
      sprintf(mbuf, unkfmt, catmod, ubyte1, dsnrCode);
      return mbuf;
   }
   while (dsnrMsgTable[i].code!=DSNR_UNKNOWN &&
		dsnrMsgTable[i].code!=dsnrCode)
      i++;

   if (dsnrMsgTable[i].code==DSNR_UNKNOWN) {
      sprintf(mbuf, unkfmt, catmod, ubyte1, dsnrCode);
      return mbuf;
   }
   return dsnrMsgTable[i].msg;
}

void 
clearScreen(Tn5250Host * This)
{
   if (This->clearState)
      return;
   if (This->wtd_set)
      flushTN5250(This);
   tn5250_buffer_append_byte(&This->buffer, ESC);
   tn5250_buffer_append_byte(&This->buffer, CMD_CLEAR_UNIT);
   This->inputInhibited = 1;
   This->clearState = 1;
}

int 
processSRQ(Tn5250Stream *This)
{
   Tn5250Record *record;
   Tn5250Buffer tbuf;

   record = saveScreen(This);
   if (!record)
      return -1;
   raise(SIGINT);  /* Generate an interrupt. */
   restoreScreen(This, &record->data);
   tn5250_record_destroy(record);
   tn5250_buffer_init(&tbuf);
   sendReadMDT(This, &tbuf, 0, TN5250_RECORD_OPCODE_INVITE);
   return 0;
}

void 
hiliteString(Tn5250Buffer *buff, char *str, Tn5250CharMap *map)
{
   tn5250_buffer_append_byte(buff, ATTR_5250_WHITE);
   appendBlock2Ebcdic(buff, (unsigned char *) str, strlen(str), map);
   tn5250_buffer_append_byte(buff, ATTR_5250_NORMAL);
}

void 
flushTN5250(Tn5250Host *This)
{
   if (tn5250_buffer_length(&This->buffer)>0) {
      tn5250_stream_send_packet(This->stream,
		tn5250_buffer_length(&This->buffer),
		TN5250_RECORD_FLOW_DISPLAY, TN5250_RECORD_H_NONE,
		TN5250_RECORD_OPCODE_PUT_GET, 
		tn5250_buffer_data(&This->buffer));
      tn5250_buffer_free(&This->buffer);
   }
   if (This->wtd_set)
      This->wtd_set = 0;
}

Tn5250Record 
*saveScreen(Tn5250Stream *This)
{
   Tn5250Buffer tbuf;
   Tn5250Record *trec;
   int statOK;

   tn5250_buffer_init(&tbuf);
   tn5250_buffer_append_byte(&tbuf, ESC);
   tn5250_buffer_append_byte(&tbuf, CMD_SAVE_SCREEN);
   tn5250_stream_send_packet(This, 2, TN5250_RECORD_FLOW_DISPLAY,
	TN5250_RECORD_H_NONE, TN5250_RECORD_OPCODE_SAVE_SCR,
	tbuf.data);
   tn5250_buffer_free(&tbuf);
   while (This->record_count>0) {
      trec = tn5250_stream_get_record(This);
      tn5250_record_destroy(trec);
   }
   do {
      statOK = (int)(tn5250_stream_handle_receive(This));
   } while (statOK && !This->record_count);
   if (statOK)
      return(tn5250_stream_get_record(This));
   else
      return NULL;
}

void 
restoreScreen(Tn5250Stream *This, Tn5250Buffer *buff)
{
   int len=tn5250_buffer_length(buff);
   unsigned char *bufp=tn5250_buffer_data(buff);

   TN5250_ASSERT(buff->data!=NULL);
   TN5250_ASSERT(len>10);
   /* Skip the standard 10 byte header since the send_packet function
      automatically generates/prefixes one for us.   GJS 3/20/2000 */
   bufp += 10;
   len -= 10;
   tn5250_stream_send_packet(This, len,
	TN5250_RECORD_FLOW_DISPLAY, TN5250_RECORD_H_NONE,
	TN5250_RECORD_OPCODE_RESTORE_SCR, bufp);
}

void 
tn5250_host_destroy(Tn5250Host *This)
{
   Tn5250Stream *myStream;

   if (This==NULL)
      return;

   tn5250_buffer_free(&This->buffer);
   if (This->map)
      tn5250_char_map_destroy(This->map);
   if (This->record)
      tn5250_record_destroy(This->record);
   if (This->screenRec)
      tn5250_record_destroy(This->screenRec);
   if (!(myStream = This->stream))
      return;
   if ( (myStream->sockfd >= 0) && myStream->disconnect)
     {
       printf("Disconnecting...\n");
         tn5250_stream_disconnect(myStream);
     }
   tn5250_stream_destroy(myStream);
}

int
SendTestScreen(Tn5250Host * This)
{

  int currow;

static char ascii_banner[][560]={
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
"   ############  ###  ####   ####   #### ### ##### ######   QQQQQ#######QQQQQ  ",
"                                                                               ",
"                                                                               ",
"                                TN5250E Server                                 ",
"                       (Press an AID key to disconnect)                        "
};

 for(currow = 0; currow < 16; currow++) 
   { 
     setBufferAddr(This, currow+3, 1);
     appendBlock2Ebcdic(&This->buffer, ascii_banner[currow], 
			strlen(ascii_banner[currow]),
			This->map);
   }
 This->inputInhibited = This->inSysInterrupt = FALSE;
 
 return( readMDTfields(This, 1) );
 
}



