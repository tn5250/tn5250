/* TN5250 - An implementation of the 5250 telnet protocol.
 * Copyright (C) 2001-2008 Scott Klement
 *
 * parts of this were copied from telnetstr.c which is (C) 1997 Michael Madore
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

#ifdef HAVE_LIBSSL

#include <openssl/ssl.h>
#include <openssl/err.h>
#include <time.h>

static int ssl_stream_get_next(Tn5250Stream *This,unsigned char *buf,int size);
static void ssl_stream_do_verb(Tn5250Stream * This, unsigned char verb, unsigned char what);
static int ssl_stream_host_verb(Tn5250Stream * This, unsigned char verb,
	unsigned char what);
static void ssl_stream_sb_var_value(Tn5250Buffer * buf, unsigned char *var, unsigned char *value);
static void ssl_stream_sb(Tn5250Stream * This, unsigned char *sb_buf, int sb_len);
static void ssl_stream_escape(Tn5250Buffer * buffer);
static void ssl_stream_write(Tn5250Stream * This, unsigned char *data, int size);
static int ssl_stream_get_byte(Tn5250Stream * This);

static int ssl_stream_connect(Tn5250Stream * This, const char *to);
static int ssl_stream_accept(Tn5250Stream * This, SOCKET_TYPE masterSock);
static void ssl_stream_destroy(Tn5250Stream *This);
static void ssl_stream_disconnect(Tn5250Stream * This);
static int ssl_stream_handle_receive(Tn5250Stream * This);
static void ssl_stream_send_packet(Tn5250Stream * This, int length,
				      StreamHeader header,
				      unsigned char *data);
static void tn3270_ssl_stream_send_packet(Tn5250Stream * This, int length,
				      StreamHeader header,
				      unsigned char * data);
int ssl_stream_passwd_cb(char *buf, int size, int rwflag, Tn5250Stream *This);
X509 *ssl_stream_load_cert(Tn5250Stream *This, const char *file);


#define SEND    1
#define IS      0
#define INFO    2
#define VALUE   1
#define VAR     0
#define VALUE   1
#define USERVAR 3

#define TERMINAL 1
#define BINARY   2
#define RECORD   4
#define DONE     7
#define HOST     8

#define TRANSMIT_BINARY 0
#define END_OF_RECORD   25
#define TERMINAL_TYPE   24
#define TIMING_MARK     6
#define NEW_ENVIRON	39

#define TN3270E         40

/* Sub-Options for TN3270E negotiation */
#define TN3270E_ASSOCIATE   0
#define TN3270E_CONNECT     1
#define TN3270E_DEVICE_TYPE 2
#define TN3270E_FUNCTIONS   3
#define TN3270E_IS          4
#define TN3270E_REASON      5
#define TN3270E_REJECT      6
#define TN3270E_REQUEST     7
#define TN3270E_SEND        8

/* Reason codes for TN3270E negotiation */
#define TN3270E_CONN_PARTNER    0
#define TN3270E_DEVICE_IN_USE   1
#define TN3270E_INV_ASSOCIATE   2
#define TN3270E_INV_NAME        3
#define TN3270E_INV_DEVICE_TYPE 4
#define TN3270E_TYPE_NAME_ERROR 5
#define TN3270E_UNKNOWN_ERROR   6
#define TN3270E_UNSUPPORTED_REQ 7

/* Function names for TN3270E FUNCTIONS sub-option */
#define TN3270E_BIND_IMAGE      0
#define TN3270E_DATA_STREAM_CTL 1
#define TN3270E_RESPONSES       2
#define TN3270E_SCS_CTL_CODES   3
#define TN3270E_SYSREQ          4

#define EOR  239
#define SE   240
#define SB   250
#define WILL 251
#define WONT 252
#define DO   253
#define DONT 254
#define IAC  255

#define TN5250_STREAM_STATE_NO_DATA 	0	/* Dummy state */
#define TN5250_STREAM_STATE_DATA	1
#define TN5250_STREAM_STATE_HAVE_IAC	2
#define TN5250_STREAM_STATE_HAVE_VERB	3	/* e.g. DO, DONT, WILL, WONT */
#define TN5250_STREAM_STATE_HAVE_SB	4	/* SB data */
#define TN5250_STREAM_STATE_HAVE_SB_IAC	5

/* Internal Telnet option settings (bit-wise flags) */
#define RECV_BINARY	1
#define SEND_BINARY	2
#define RECV_EOR	4
#define SEND_EOR	8

#ifndef HAVE_UCHAR
typedef unsigned char UCHAR;
#endif

static const UCHAR hostInitStr[] = {IAC,DO,NEW_ENVIRON,IAC,DO,TERMINAL_TYPE};
static const UCHAR hostDoEOR[] = {IAC,DO,END_OF_RECORD};
static const UCHAR hostDoBinary[] = {IAC,DO,TRANSMIT_BINARY};
static const UCHAR hostDoTN3270E[] = {IAC,DO,TN3270E};
static const UCHAR hostSBDevice[] = {IAC,SB,TN3270E,TN3270E_SEND,TN3270E_DEVICE_TYPE,
			       IAC,SE};
typedef struct doTable_t {
   const UCHAR	*cmd;
   unsigned	len;
} DOTABLE;

static const DOTABLE host5250DoTable[] = {
  hostInitStr,	sizeof(hostInitStr),
  hostDoEOR,	sizeof(hostDoEOR),
  hostDoBinary,	sizeof(hostDoBinary),
  NULL,		0
};

static const DOTABLE host3270DoTable[] = {
  hostInitStr,  sizeof(hostInitStr),
  hostDoEOR,    sizeof(hostDoEOR),
  hostDoBinary, sizeof(hostDoBinary),
  NULL,         0
};

static const UCHAR SB_Str_NewEnv[]={IAC, SB, NEW_ENVIRON, SEND, USERVAR,
	'I','B','M','R','S','E','E','D', 0,1,2,3,4,5,6,7,
	VAR, USERVAR, IAC, SE};
static const UCHAR SB_Str_TermType[]={IAC, SB, TERMINAL_TYPE, SEND, IAC, SE};

/* FIXME: This should be added to Tn5250Stream structure, or something
    else better than this :) */
int errnum;

#ifdef NDEBUG
 #define IACVERB_LOG(tag,verb,what)
 #define TNSB_LOG(sb_buf,sb_len)
 #define LOGERROR(tag, ecode)
 #define DUMP_ERR_STACK()
#else
 #define IACVERB_LOG	ssl_log_IAC_verb
 #define TNSB_LOG	ssl_log_SB_buf
 #define LOGERROR	ssl_logError
 #define DUMP_ERR_STACK ssl_log_error_stack

static char *ssl_getTelOpt(what)
{
   char *wcp;
   static char wbuf[12];

   switch (what) {
      case TERMINAL_TYPE:
		wcp = "<TERMTYPE>";
		break;
      case END_OF_RECORD:
		wcp = "<END_OF_REC>";
		break;
      case TRANSMIT_BINARY:
		wcp = "<BINARY>";
		break;
      case NEW_ENVIRON:
		wcp = "<NEWENV>";
		break;
      case EOR:
		wcp = "<EOR>";
		break;
      default:
		snprintf(wcp=wbuf, sizeof(wbuf), "<%02X>", what);
		break;
   }
   return wcp;
}

static void ssl_log_error_stack(void)
{
   FILE *errfp = tn5250_logfile ? tn5250_logfile : stderr;

   ERR_print_errors_fp(errfp);
}

static void ssl_logError(char *tag, int ecode)
{
   FILE *errfp = tn5250_logfile ? tn5250_logfile : stderr;

   fprintf(errfp,"%s: ERROR (code=%d) - %s\n", tag, ecode, strerror(ecode));
}

static void ssl_log_IAC_verb(char *tag, int verb, int what)
{
   char *vcp, vbuf[10];

   if (!tn5250_logfile)
      return;
   switch (verb) {
      case DO:	vcp = "<DO>";
		break;
      case DONT:
		vcp = "<DONT>";
		break;
      case WILL:
		vcp = "<WILL>";
		break;
      case WONT:
		vcp = "<WONT>";
		break;
      default:
		sprintf(vcp=vbuf, "<%02X>", verb);
		break;
   }
   fprintf(tn5250_logfile,"%s:<IAC>%s%s\n", tag, vcp, ssl_getTelOpt(what));
}

static int ssl_dumpVarVal(UCHAR *buf, int len)
{
   int c, i;

   for (c=buf[i=0]; i<len && c!=VAR && c!=VALUE && c!=USERVAR; c=buf[++i]) {
      if (isprint(c))
         putc(c, tn5250_logfile);
      else
         fprintf(tn5250_logfile,"<%02X>", c);
   }
   return i;
}

static int ssl_dumpNewEnv(unsigned char *buf, int len)
{
   int c, i=0, j;

   while (i<len) {
      switch (c=buf[i]) {
         case IAC:
		return i;
         case VAR:
		fputs("\n\t<VAR>",tn5250_logfile);
		if (++i<len && buf[i]==USERVAR) {
		   fputs("<USERVAR>",tn5250_logfile);
		   return i+1;
		}
		j = ssl_dumpVarVal(buf+i, len-i);
		i += j;
         case USERVAR:
		fputs("\n\t<USERVAR>",tn5250_logfile);
		if (!memcmp("IBMRSEED", &buf[++i], 8)) {
		   fputs("IBMRSEED",tn5250_logfile);
		   putc('<',tn5250_logfile);
		   for (j=0, i+=8; j<8; i++, j++) {
		      if (j)
		         putc(' ',tn5250_logfile);
		      fprintf(tn5250_logfile,"%02X",buf[i]);
		   }
		   putc('>',tn5250_logfile);
		} else {
		   j = ssl_dumpVarVal(buf+i, len-i);
		   i += j;
		}
		break;
         case VALUE:
		fputs("<VALUE>",tn5250_logfile);
		i++;
		j = ssl_dumpVarVal(buf+i, len-i);
		i += j;
		break;
         default:
		fputs(ssl_getTelOpt(c),tn5250_logfile);
      } /* switch */
   } /* while */
   return i;
}

static void ssl_log_SB_buf(unsigned char *buf, int len)
{
   int c, i, type;

   if (!tn5250_logfile)
      return;
   fprintf(tn5250_logfile,ssl_getTelOpt(type=*buf++));
   switch (c=*buf++) {
      case IS:
		fputs("<IS>",tn5250_logfile);
		break;
      case SEND:
		fputs("<SEND>",tn5250_logfile);
		break;
      default:
		fputs(ssl_getTelOpt(c),tn5250_logfile);
   }
   len -= 2;
   i = (type==NEW_ENVIRON) ? ssl_dumpNewEnv(buf,len) : 0;
   while (i<len) {
      switch(c=buf[i++]) {
         case IAC:
		fputs("<IAC>",tn5250_logfile);
		if (i<len)
		   fputs(ssl_getTelOpt(buf[i++]), tn5250_logfile);
		break;
         default:
		if (isprint(c))
		   putc(c, tn5250_logfile);
		else
		   fprintf(tn5250_logfile,"<%02X>", c);
      }
   }
}
#endif /* !NDEBUG */

/****f* lib5250/tn5250_ssl_stream_init
 * NAME
 *    tn5250_ssl_stream_init
 * SYNOPSIS
 *    ret = tn5250_ssl_stream_init (This);
 * INPUTS
 *    Tn5250Stream *       This       - 
 * DESCRIPTION
 *    DOCUMENT ME!!!
 *****/
int tn5250_ssl_stream_init (Tn5250Stream *This)
{
   int len;
   char methstr[5];
   SSL_METHOD *meth=NULL;

   TN5250_LOG(("tn5250_ssl_stream_init() entered.\n"));

/*  initialize SSL library */

   SSL_load_error_strings();
   SSL_library_init();

/*  which SSL method do we use? */

   strcpy(methstr,"auto");
   if (This->config!=NULL && tn5250_config_get (This->config, "ssl_method")) {
        strncpy(methstr, tn5250_config_get (This->config, "ssl_method"), 4);
        methstr[4] = '\0';
   }

   if (!strcmp(methstr, "ssl2")) {
        meth = SSLv2_client_method();         
        TN5250_LOG(("SSL Method = SSLv2_client_method()\n"));
   } else if (!strcmp(methstr, "ssl3")) {
        meth = SSLv3_client_method();         
        TN5250_LOG(("SSL Method = SSLv3_client_method()\n"));
   } else {
        meth = SSLv23_client_method();         
        TN5250_LOG(("SSL Method = SSLv23_client_method()\n"));
   }

/*  create a new SSL context */

   This->ssl_context = SSL_CTX_new(meth);
   if (This->ssl_context==NULL) {
        DUMP_ERR_STACK ();
        return -1;
   }

/* if a certificate authority file is defined, load it into this context */

   if (This->config!=NULL && tn5250_config_get (This->config, "ssl_ca_file")) {
        if (SSL_CTX_load_verify_locations(This->ssl_context, 
                  tn5250_config_get (This->config, "ssl_ca_file"), NULL)<1) {
            DUMP_ERR_STACK ();
            return -1;
        }
   }

   This->userdata = NULL;

/* if a PEM passphrase is defined, set things up so that it can be used */

   if (This->config!=NULL && tn5250_config_get (This->config,"ssl_pem_pass")){
        TN5250_LOG(("SSL: Setting password callback\n"));
        len = strlen(tn5250_config_get (This->config, "ssl_pem_pass"));
        This->userdata = malloc(len+1);
        strncpy(This->userdata,
                tn5250_config_get (This->config, "ssl_pem_pass"), len);
        SSL_CTX_set_default_passwd_cb(This->ssl_context,
                (pem_password_cb *)ssl_stream_passwd_cb);
        SSL_CTX_set_default_passwd_cb_userdata(This->ssl_context, (void *)This);

   }

/* If a certificate file has been defined, load it into this context as well */

   if (This->config!=NULL && tn5250_config_get (This->config, "ssl_cert_file")){

        if ( tn5250_config_get (This->config,  "ssl_check_exp") ) {
           X509 *client_cert;
           time_t tnow;
           int extra_time;
           TN5250_LOG(("SSL: Checking expiration of client cert\n"));
           client_cert = ssl_stream_load_cert(This,
                tn5250_config_get (This->config, "ssl_cert_file"));
           if (client_cert == NULL) {
                TN5250_LOG(("SSL: Unable to load client certificate!\n"));
                return -1;
           }
           extra_time = tn5250_config_get_int(This->config, "ssl_check_exp");
           tnow = time(NULL) + extra_time;
           if (ASN1_UTCTIME_cmp_time_t(X509_get_notAfter(client_cert), tnow)
                  == -1 ) {
                if (extra_time > 1) {
                   printf("SSL error: client certificate will be expired\n");
                   TN5250_LOG(("SSL: client certificate will be expired\n"));
                } else {
                   printf("SSL error: client certificate has expired\n");
                   TN5250_LOG(("SSL: client certificate has expired\n"));
                }
                return -1;
           }
           X509_free(client_cert);
        }
           
        TN5250_LOG(("SSL: Loading certificates from certificate file\n"));
        if (SSL_CTX_use_certificate_file(This->ssl_context,
                tn5250_config_get (This->config, "ssl_cert_file"),
                SSL_FILETYPE_PEM) <= 0) {
            DUMP_ERR_STACK ();
            return -1;
        }
        TN5250_LOG(("SSL: Loading private keys from certificate file\n"));
        if (SSL_CTX_use_PrivateKey_file(This->ssl_context,
                tn5250_config_get (This->config, "ssl_cert_file"),
                SSL_FILETYPE_PEM) <= 0) {
            DUMP_ERR_STACK ();
            return -1;
        }
   }

   This->ssl_handle = NULL;

   This->connect = ssl_stream_connect;
   This->accept = ssl_stream_accept;
   This->disconnect = ssl_stream_disconnect;
   This->handle_receive = ssl_stream_handle_receive;
   This->send_packet = ssl_stream_send_packet;
   This->destroy = ssl_stream_destroy;
   This->streamtype = TN5250_STREAM;
   TN5250_LOG(("tn5250_ssl_stream_init() success.\n"));
   return 0; /* Ok */
}

/****f* lib5250/tn3270_ssl_stream_init
 * NAME
 *    tn3270_ssl_stream_init
 * SYNOPSIS
 *    ret = tn3270_ssl_stream_init (This);
 * INPUTS
 *    Tn5250Stream *       This       - 
 * DESCRIPTION
 *    DOCUMENT ME!!!
 *****/
int tn3270_ssl_stream_init (Tn5250Stream *This)
{
   int len;

/* initialize SSL library */

   SSL_load_error_strings();
   SSL_library_init();

/* create a new SSL context */

   This->ssl_context = SSL_CTX_new(SSLv23_client_method());
   if (This->ssl_context==NULL) {
        DUMP_ERR_STACK ();
        return -1;
   }

/* if a certificate authority file is defined, load it into this context */

   if (This->config!=NULL && tn5250_config_get (This->config, "ssl_ca_file")) {
        if (SSL_CTX_load_verify_locations(This->ssl_context, 
                  tn5250_config_get (This->config, "ssl_ca_file"), NULL)<1) {
            DUMP_ERR_STACK ();
            return -1;
        }
   }

/* if a certificate authority file is defined, load it into this context */

   if (This->config!=NULL && tn5250_config_get (This->config, "ssl_ca_file")) {
        if (SSL_CTX_load_verify_locations(This->ssl_context, 
                  tn5250_config_get (This->config, "ssl_ca_file"), NULL)<1) {
            DUMP_ERR_STACK ();
            return -1;
        }
   }

   This->userdata = NULL;

/* if a PEM passphrase is defined, set things up so that it can be used */

   if (This->config!=NULL && tn5250_config_get (This->config,"ssl_pem_pass")){
        TN5250_LOG(("SSL: Setting password callback\n"));
        len = strlen(tn5250_config_get (This->config, "ssl_pem_pass"));
        This->userdata = malloc(len+1);
        strncpy(This->userdata,
                tn5250_config_get (This->config, "ssl_pem_pass"), len);
        SSL_CTX_set_default_passwd_cb(This->ssl_context,
                (pem_password_cb *)ssl_stream_passwd_cb);
        SSL_CTX_set_default_passwd_cb_userdata(This->ssl_context, (void *)This);

   }

/* If a certificate file has been defined, load it into this context as well */

   if (This->config!=NULL && tn5250_config_get (This->config, "ssl_cert_file")){
        TN5250_LOG(("SSL: Loading certificates from certificate file\n"));
        if (SSL_CTX_use_certificate_file(This->ssl_context,
                tn5250_config_get (This->config, "ssl_cert_file"),
                SSL_FILETYPE_PEM) <= 0) {
            DUMP_ERR_STACK ();
            return -1;
        }
        TN5250_LOG(("SSL: Loading private keys from certificate file\n"));
        if (SSL_CTX_use_PrivateKey_file(This->ssl_context,
                tn5250_config_get (This->config, "ssl_cert_file"),
                SSL_FILETYPE_PEM) <= 0) {
            DUMP_ERR_STACK ();
            return -1;
        }
   }

   This->ssl_handle = NULL;
   This->connect = ssl_stream_connect;
   This->accept = ssl_stream_accept;
   This->disconnect = ssl_stream_disconnect;
   This->handle_receive = ssl_stream_handle_receive;
   This->send_packet = tn3270_ssl_stream_send_packet;
   This->destroy = ssl_stream_destroy;
   This->streamtype = TN3270E_STREAM;
   return 0; /* Ok */
}

/****i* lib5250/ssl_stream_connect
 * NAME
 *    ssl_stream_connect
 * SYNOPSIS
 *    ret = ssl_stream_connect (This, to);
 * INPUTS
 *    Tn5250Stream *       This       - 
 *    const char *         to         - 
 * DESCRIPTION
 *    Connects to server.  The `to' parameter is in the form
 *    host[:port].
 *****/
static int ssl_stream_connect(Tn5250Stream * This, const char *to)
{
   struct sockaddr_in serv_addr;
   u_long ioctlarg = 1;
   char *address;
   int r;
   X509 *server_cert;
   long certvfy;

   TN5250_LOG(("tn5250_ssl_stream_connect() entered.\n"));

   memset((char *) &serv_addr, 0, sizeof(serv_addr));
   serv_addr.sin_family = AF_INET;

   /* Figure out the internet address. */
   address = (char *)malloc (strlen (to)+1);
   strcpy (address, to);
   if (strchr (address, ':'))
      *strchr (address, ':') = '\0';
   
   serv_addr.sin_addr.s_addr = inet_addr(address);
   if (serv_addr.sin_addr.s_addr == INADDR_NONE) {
      struct hostent *pent = gethostbyname(address);
      if (pent != NULL)
	 serv_addr.sin_addr.s_addr = *((u_long *) (pent->h_addr));
   }
   free (address);
   if (serv_addr.sin_addr.s_addr == INADDR_NONE) {
      TN5250_LOG(("sslstream: Host lookup failed!\n"));
      return -1;
   }

   /* Figure out the port name. */
   if (strchr (to, ':') != NULL) {
      const char *port = strchr (to, ':') + 1;
      serv_addr.sin_port = htons((u_short) atoi(port));
      if (serv_addr.sin_port == 0) {
	 struct servent *pent = getservbyname(port, "tcp");
	 if (pent != NULL)
	    serv_addr.sin_port = pent->s_port;
      }
      if (serv_addr.sin_port == 0) {
          TN5250_LOG(("sslstream: Port lookup failed!\n"));
          return -1;
      }
   } else {
      /* No port specified ... use telnet-ssl port. */
      struct servent *pent = getservbyname ("telnets", "tcp");
      if (pent == NULL)
	 serv_addr.sin_port = htons(992);
      else
	 serv_addr.sin_port = pent->s_port;
   }

   This->ssl_handle = SSL_new(This->ssl_context);
   if (This->ssl_handle==NULL) {
        DUMP_ERR_STACK ();
        TN5250_LOG(("sslstream: SSL_new() failed!\n"));
        return -1;
   }

   This->sockfd = socket(AF_INET, SOCK_STREAM, 0);
   if (WAS_INVAL_SOCK(This->sockfd)) {
      TN5250_LOG(("sslstream: socket() failed, errno=%d\n", errno));
      return -1;
   }

   if ((r=SSL_set_fd(This->ssl_handle, This->sockfd))==0) {
      errnum = SSL_get_error(This->ssl_handle, r);
      DUMP_ERR_STACK ();
      TN5250_LOG(("sslstream: SSL_set_fd() failed, errnum=%d\n", errnum));
      return errnum;
   }

   r = connect(This->sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr));
   if (WAS_ERROR_RET(r)) {
      TN5250_LOG(("sslstream: connect() failed, errno=%d\n", errno));
      return -1;
   }

   if ((r=SSL_connect(This->ssl_handle)<1)) {
        errnum = SSL_get_error(This->ssl_handle, r);
        DUMP_ERR_STACK ();
        TN5250_LOG(("sslstream: SSL_connect() failed, errnum=%d\n", errnum));
        return errnum;
   }

   TN5250_LOG(("Connected with SSL\n"));
   TN5250_LOG(("Using %s cipher with a %d bit secret key\n", 
          SSL_get_cipher_name(This->ssl_handle),
          SSL_get_cipher_bits(This->ssl_handle, NULL) ));

   server_cert = SSL_get_peer_certificate (This->ssl_handle);

   if (server_cert == NULL) {
        TN5250_LOG(("sslstream: Server did not present a certificate!\n"));
        return -1;
   }
   else {
        time_t tnow = time(NULL);
        int extra_time= 0;
        if (This->config!=NULL && 
            tn5250_config_get(This->config, "ssl_check_exp")!=NULL) {
                 extra_time = tn5250_config_get_int(This->config, 
                                             "ssl_check_exp");
                 tnow += extra_time;
            if (ASN1_UTCTIME_cmp_time_t(X509_get_notAfter(server_cert), tnow)
                   == -1 ) {
                 if (extra_time > 1) {
                    printf("SSL error: server certificate will be expired\n");
                    TN5250_LOG(("SSL: server certificate will be expired\n"));
                 } else {
                    printf("SSL error: server certificate has expired\n");
                    TN5250_LOG(("SSL: server certificate has expired\n"));
                 }
                 return -1;
            }
        }
        TN5250_LOG(("SSL Certificate issued by: %s\n",
           X509_NAME_oneline(X509_get_issuer_name(server_cert), 0,0) ));
        certvfy = SSL_get_verify_result(This->ssl_handle);
        if (certvfy == X509_V_OK) {
           TN5250_LOG(("SSL Certificate successfully verified!\n"));
        } else {
           TN5250_LOG(("SSL Certificate verification failed, reason: %d\n", 
		certvfy));
           if (This->config!=NULL && 
              tn5250_config_get_bool (This->config, "ssl_verify_server")) 
                return -1;
        }
   }
   
   /* Set socket to non-blocking mode. */
   TN5250_LOG(("SSL must be Non-Blocking\n"));
   TN_IOCTL(This->sockfd, FIONBIO, &ioctlarg);

   This->state = TN5250_STREAM_STATE_DATA;
   TN5250_LOG(("tn5250_ssl_stream_connect() success.\n"));
   return 0;
}

/****i* lib5250/ssl_stream_accept
 * NAME
 *    ssl_stream_accept
 * SYNOPSIS
 *    ret = ssl_stream_accept (This, masterSock);
 * INPUTS
 *    Tn5250Stream *	This       - 
 *    SOCKET		masterSock -
 * DESCRIPTION
 *    Accepts a connection from the client.
 *****/
static int ssl_stream_accept(Tn5250Stream * This, SOCKET_TYPE masterfd)
{
   int i, len, retCode;
   struct sockaddr_in serv_addr;
   fd_set fdr;
   struct timeval tv;
   int negotiating;

#ifndef WINELIB
   u_long ioctlarg=1L;
#endif

/* FIXME:  This routine needs to be converted to use SSL calls 
           I just left it disabled for now.  -- SCK          */

#if 0
   /*
   len = sizeof(serv_addr);
   This->sockfd = accept(masterSock, (struct sockaddr *) &serv_addr, &len);
   if (WAS_INVAL_SOCK(This->sockfd)) {
      return -1;
   }
   */
   printf("This->sockfd = %d\n", masterfd);
   This->sockfd = masterfd;

   /* Set socket to non-blocking mode. */
   TN_IOCTL(This->sockfd, FIONBIO, &ioctlarg);

   This->state = TN5250_STREAM_STATE_DATA;
   This->status = HOST;

   /* Commence TN5250 negotiations...
      Send DO options (New Environment, Terminal Type, etc.) */

   if(This->streamtype == TN3270E_STREAM)
     {
       retCode = SSL_write(this->ssl_handle, hostDoTN3270E, sizeof(hostDoTN3270E));
       if (retCode<1) {
         errnum = SSL_get_error(This->ssl_handle, retCode);
         fprintf(stderr, "sslstream: %s\n", ERR_error_string(errnum,NULL));
	 return errnum;
       }

       tv.tv_sec = 5;
       tv.tv_usec = 0;
       TN_SELECT(This->sockfd + 1, &fdr, NULL, NULL, &tv);
       if (FD_ISSET(This->sockfd, &fdr)) {
	   
	 if (!ssl_stream_handle_receive(This)) {
	   retCode = errnum;
	   return retCode ? retCode : -1;
	 }
       } else {
	 return -1;
       }

       if(This->streamtype == TN3270E_STREAM)
	 {
           retCode = SSL_write(This->ssl_handle, hostDoTN3270E,sizeof(hostDoTN3270E));
           if (retCode<1) {
              errnum = SSL_get_error(This->ssl_handle, retCode);
              fprintf(stderr,"sslstream: %s\n",ERR_error_string(errnum,NULL));
	      return errnum;
           }

	   FD_ZERO(&fdr);
	   FD_SET(This->sockfd, &fdr);
	   tv.tv_sec = 5;
	   tv.tv_usec = 0;
	   TN_SELECT(This->sockfd + 1, &fdr, NULL, NULL, &tv);
	   if (FD_ISSET(This->sockfd, &fdr)) {
	     
	     if (!ssl_stream_handle_receive(This)) {
	       retCode = errnum;
	       return retCode ? retCode : -1;
	     }
	   } else {
	     return -1;
	   }

	   FD_ZERO(&fdr);
	   FD_SET(This->sockfd, &fdr);
	   tv.tv_sec = 5;
	   tv.tv_usec = 0;
	   TN_SELECT(This->sockfd + 1, &fdr, NULL, NULL, &tv);
	   if (FD_ISSET(This->sockfd, &fdr)) {
	     
	     if (!ssl_stream_handle_receive(This)) {
	       retCode = errnum;
	       return retCode ? retCode : -1;
	     }
	   } else {
	     return -1;
	   }
	 } 
       else 
	 {
	   goto neg5250;
	 }
     }
   else
     {
     neg5250:
       for (i=0; host5250DoTable[i].cmd; i++) {
         retCode = SSL_write(This->ssl_handle, host5250DoTable[i].cmd,
                     host5250DoTable[i].len);
         if (retCode<1) {
             errnum = SSL_get_error(This->ssl_handle, retCode);
             fprintf(stderr,"sslstream: %s\n",ERR_error_string(errnum,NULL));
	     return errnum;
         }
	 
	 FD_ZERO(&fdr);
	 FD_SET(This->sockfd, &fdr);
	 tv.tv_sec = 5;
	 tv.tv_usec = 0;
	 TN_SELECT(This->sockfd + 1, &fdr, NULL, NULL, &tv);
	 if (FD_ISSET(This->sockfd, &fdr)) {
	   
	   if (!ssl_stream_handle_receive(This)) {
	     retCode = errnum;
	     return retCode ? retCode : -1;
	   }
	 } else {
	   return -1;
	 }
       }
     }
   return 0;
#endif
   return -1;
}

/****i* lib5250/ssl_stream_disconnect
 * NAME
 *    ssl_stream_disconnect
 * SYNOPSIS
 *    ssl_stream_disconnect (This);
 * INPUTS
 *    Tn5250Stream *       This       - 
 * DESCRIPTION
 *    Disconnect from the remote host.
 *****/
static void ssl_stream_disconnect(Tn5250Stream * This)
{
   SSL_shutdown(This->ssl_handle);
   TN_CLOSE(This->sockfd);
}

/****i* lib5250/ssl_stream_destroy
 * NAME
 *    ssl_stream_destroy
 * SYNOPSIS
 *    ssl_stream_destroy (This);
 * INPUTS
 *    Tn5250Stream *       This       - 
 * DESCRIPTION
 *    DOCUMENT ME!!!
 *****/
static void ssl_stream_destroy(Tn5250Stream *This)
{
   /* noop */
}

/****i* lib5250/ssl_stream_get_next
 * NAME
 *    ssl_stream_get_next
 * SYNOPSIS
 *    ssl_stream_get_next (This, buf, size);
 * INPUTS
 *    Tn5250Stream *       This       - 
 *    unsigned char *      buf        -
 *    int                  size       -
 * DESCRIPTION
 *    Reads data from the socket,  returns the length,
 *    or -2 if disconnected, or -1 if out of data to read.
 *****/
static int ssl_stream_get_next(Tn5250Stream *This,unsigned char *buf,int size)
{

   int rc;
   fd_set wrwait;

    /*  read data.
     *
     *  Note: it's possible, due to the negotiations that SSL can do below
     *  the surface, that SSL_read() will need to wait for buffer space
     *  to write to.   If that happens, we'll use select() to wait for
     *  space and try again.  
     */
    do {
          rc = SSL_read(This->ssl_handle, buf, size);
          if (rc < 1) {
               errnum = SSL_get_error(This->ssl_handle, rc);
               switch (errnum) {
                   case SSL_ERROR_WANT_WRITE:
                      FD_ZERO(&wrwait);
                      FD_SET(This->sockfd, &wrwait);
                      select(This->sockfd+1, NULL, &wrwait, NULL, NULL);
                      break;
                   case SSL_ERROR_WANT_READ:
                      return -1;
                      break;
                   default:
                      return -2;
                      break;
               }
          }
    } while (rc<1);

    return rc;
}

static int ssl_sendWill(Tn5250Stream *This, unsigned char what)
{
   UCHAR buff[3]={IAC,WILL};
   buff[2] = what;
   TN5250_LOG(("SSL_Write: %x %x %x\n", buff[0], buff[1], buff[2]));
   return SSL_write(This->ssl_handle, buff, 3);
}

/****i* lib5250/ssl_stream_host_verb
 * NAME
 *    ssl_stream_host_verb
 * SYNOPSIS
 *    ssl_stream_host_verb (This, verb, what);
 * INPUTS
 *    Tn5250Stream   *  This    -
 *    unsigned char	verb	-
 *    unsigned char	what	-
 * DESCRIPTION
 *    Process the telnet DO, DONT, WILL, or WONT escape sequence.
 *****/
static int ssl_stream_host_verb(Tn5250Stream * This, unsigned char verb,
		unsigned char what)
{
   int len, option=0, retval=0;

   IACVERB_LOG("GotVerb(1)",verb,what);
   switch (verb) {
      case DO:
	switch (what) {
	   case END_OF_RECORD:
		option = SEND_EOR;
		break;

	   case TRANSMIT_BINARY:
		option = SEND_BINARY;
		break;

	   default:
		break;
	} /* DO: switch (what) */
	break;

      case DONT:
      case WONT:
	if(what == TN3270E) 
	  {
	    This->streamtype = TN3270_STREAM;
	  }
	break;

      case WILL:
	switch (what) {
	   case NEW_ENVIRON:
		len = sizeof(SB_Str_NewEnv);
		TN5250_LOG(("Sending SB NewEnv..\n"));
		retval = SSL_write(This->ssl_handle, SB_Str_NewEnv, len);
		break;

	   case TERMINAL_TYPE:
		len = sizeof(SB_Str_TermType);
		TN5250_LOG(("Sending SB TermType..\n"));
		retval = SSL_write(This->ssl_handle, SB_Str_TermType, len);
		break;

	   case END_OF_RECORD:
		option = RECV_EOR;
		retval = ssl_sendWill(This, what);
		break;

	   case TRANSMIT_BINARY:
		option = RECV_BINARY;
		retval = ssl_sendWill(This, what);
		break;

	   default:
		break;
	} /* WILL: switch (what) */
	break;

      default:
	break;
   } /* switch (verb) */

   if (retval>0) retval=option;

   return(retval);
} /* ssl_stream_host_verb */


/****i* lib5250/ssl_stream_do_verb
 * NAME
 *    ssl_stream_do_verb
 * SYNOPSIS
 *    ssl_stream_do_verb (This, verb, what);
 * INPUTS
 *    Tn5250Stream *       This       - 
 *    unsigned char        verb       - 
 *    unsigned char        what       - 
 * DESCRIPTION
 *    Process the telnet DO, DONT, WILL, or WONT escape sequence.
 *****/
static void ssl_stream_do_verb(Tn5250Stream * This, unsigned char verb, unsigned char what)
{
   unsigned char reply[3];
   int ret;

   IACVERB_LOG("GotVerb(2)", verb, what);
   reply[0] = IAC;
   reply[2] = what;
   switch (verb) {
   case DO:
      switch (what) {
      case TERMINAL_TYPE:
      case END_OF_RECORD:
      case TRANSMIT_BINARY:
      case NEW_ENVIRON:
	 reply[1] = WILL;
	 break;

      default:
	 reply[1] = WONT;
	 break;
      }
      break;

   case DONT:
      break;

   case WILL:
      switch (what) {
      case TERMINAL_TYPE:
      case END_OF_RECORD:
      case TRANSMIT_BINARY:
      case NEW_ENVIRON:
	 reply[1] = DO;
	 break;

      case TIMING_MARK:
	 TN5250_LOG(("do_verb: IAC WILL TIMING_MARK received.\n"));
      default:
	 reply[1] = DONT;
	 break;
      }
      break;

   case WONT:
      break;
   }

   /* We should really keep track of states here, but the code has been
    * like this for some time, and no complaints.  
    *
    * Actually, I don't even remember what that comment means -JMF */

   IACVERB_LOG("GotVerb(3)",verb,what);
   TN5250_LOG(("SSL_Write: %x %x %x\n", reply[0], reply[1], reply[2]));
   ret = SSL_write(This->ssl_handle, (char *) reply, 3);
   if (ret<1) {
      errnum = SSL_get_error(This->ssl_handle, ret);
      printf("Error writing to socket: %s\n", ERR_error_string(errnum,NULL));
      exit(5);
   }
}

static void ssl_stream_host_sb(Tn5250Stream * This, UCHAR *sb_buf,
		int sb_len)
{
  int rc;
  int i;
  int sbType;
  int sbParm;
  Tn5250Buffer tbuf;
  UCHAR deviceResponse[] = {IAC,SB,TN3270E,TN3270E_DEVICE_TYPE,TN3270E_IS};
  UCHAR functionResponse[] = {IAC,SB,TN3270E,TN3270E_FUNCTIONS};
  char * dummyname = "TN3E002";
  
  if (sb_len <= 0)
    return;

  TN5250_LOG(("GotSB:<IAC><SB>"));
  TNSB_LOG(sb_buf,sb_len);
  TN5250_LOG(("<IAC><SE>\n"));
  sbType = sb_buf[0];
  switch (sbType) 
    {
    case TN3270E:
      sb_buf += 1;
      sb_len -= 1;
      sbParm = sb_buf[0];
      switch (sbParm)
	{
	case TN3270E_DEVICE_TYPE:
	  sb_buf += 2; /* Device string follows DEVICE_TYPE IS parameter */
	  sb_len -= 2;
	  tn5250_buffer_init(&tbuf);
	  tn5250_buffer_append_data(&tbuf, deviceResponse, 
				    sizeof(deviceResponse));
	  for(i=0; i<sb_len && sb_buf[i] != IAC; i++)
	    tn5250_buffer_append_byte(&tbuf, sb_buf[i]);
	  tn5250_buffer_append_byte(&tbuf, TN3270E_CONNECT);
	  tn5250_buffer_append_data(&tbuf, dummyname, strlen(dummyname));
	  tn5250_buffer_append_byte(&tbuf, IAC);
	  tn5250_buffer_append_byte(&tbuf, SE);
	  rc = SSL_write(This->ssl_handle, (char *) tn5250_buffer_data(&tbuf),
		       tn5250_buffer_length(&tbuf));
	  if (rc<1) {
            errnum = SSL_get_error(This->ssl_handle, rc);
	    printf("Error in SSL_write: %s\n", ERR_error_string(errnum,NULL));
	    exit(5);
	  }
	  break;
	case TN3270E_FUNCTIONS:
	  sb_buf += 2; /* Function list follows FUNCTIONS REQUEST parameter */ 
	  sb_len -= 2;
	  tn5250_buffer_init(&tbuf);
	  tn5250_buffer_append_data(&tbuf, functionResponse, 
				    sizeof(functionResponse));
	  
	  tn5250_buffer_append_byte(&tbuf, TN3270E_IS);
	  for(i=0; i<sb_len && sb_buf[i] != IAC; i++)
	    {
	      tn5250_buffer_append_byte(&tbuf, sb_buf[i]);
	      This->options = This->options | (1 << (sb_buf[i]+1));
	    }
	  
	  tn5250_buffer_append_byte(&tbuf, IAC);
	  tn5250_buffer_append_byte(&tbuf, SE);
	  rc = SSL_write(This->ssl_handle, (char *) tn5250_buffer_data(&tbuf),
		       tn5250_buffer_length(&tbuf));
	  if (rc<1) {
            errnum = SSL_get_error(This->ssl_handle, rc);
	    printf("Error in SSL_write: %s\n", ERR_error_string(errnum,NULL));
	    exit(5);
	  }
	  break;
	default:
	  break;
	}
      break;
    case TERMINAL_TYPE:
      sb_buf += 2;  /* Assume IS follows SB option type. */
      sb_len -= 2;
      tn5250_buffer_init(&tbuf);
      for (i=0; i<sb_len && sb_buf[i]!=IAC; i++)
	tn5250_buffer_append_byte(&tbuf, sb_buf[i]);
      tn5250_buffer_append_byte(&tbuf, 0);
      tn5250_stream_setenv(This, "TERM", (char *) tbuf.data);
      tn5250_buffer_free(&tbuf);
      break;
    case NEW_ENVIRON:
      /* TODO:
       * setNewEnvVars(This, sb_buf, sb_len);
       */
      break;
    default:
      break;
    } /* switch */
} /* ssl_stream_host_sb */

/****i* lib5250/ssl_stream_sb_var_value
 * NAME
 *    ssl_stream_sb_var_value
 * SYNOPSIS
 *    ssl_stream_sb_var_value (buf, var, value);
 * INPUTS
 *    Tn5250Buffer *       buf        - 
 *    unsigned char *      var        - 
 *    unsigned char *      value      - 
 * DESCRIPTION
 *    Utility function for constructing replies to NEW_ENVIRON requests.
 *****/
static void ssl_stream_sb_var_value(Tn5250Buffer * buf, unsigned char *var, unsigned char *value)
{
   tn5250_buffer_append_byte(buf, VAR);
   tn5250_buffer_append_data(buf, var, strlen((char *) var));
   tn5250_buffer_append_byte(buf, VALUE);
   tn5250_buffer_append_data(buf, value, strlen((char *) value));
}

/****i* lib5250/ssl_stream_sb
 * NAME
 *    ssl_stream_sb
 * SYNOPSIS
 *    ssl_stream_sb (This, sb_buf, sb_len);
 * INPUTS
 *    Tn5250Stream *       This       - 
 *    unsigned char *      sb_buf     - 
 *    int                  sb_len     - 
 * DESCRIPTION
 *    Handle telnet SB escapes, which are the option-specific negotiations.
 *****/
static void ssl_stream_sb(Tn5250Stream * This, unsigned char *sb_buf, int sb_len)
{
   Tn5250Buffer out_buf;
   int ret;

   TN5250_LOG(("GotSB:<IAC><SB>"));
   TNSB_LOG(sb_buf,sb_len);
   TN5250_LOG(("<IAC><SE>\n"));

   tn5250_buffer_init(&out_buf);

   if (sb_len <= 0)
      return;

   if (sb_buf[0] == TERMINAL_TYPE) {
      unsigned char *termtype;

      if (sb_buf[1] != SEND)
	 return;

      termtype = (unsigned char *) tn5250_stream_getenv(This, "TERM");

      tn5250_buffer_append_byte(&out_buf, IAC);
      tn5250_buffer_append_byte(&out_buf, SB);
      tn5250_buffer_append_byte(&out_buf, TERMINAL_TYPE);
      tn5250_buffer_append_byte(&out_buf, IS);
      tn5250_buffer_append_data(&out_buf, termtype, strlen((char *) termtype));
      tn5250_buffer_append_byte(&out_buf, IAC);
      tn5250_buffer_append_byte(&out_buf, SE);

      ret = SSL_write(This->ssl_handle, (char *) tn5250_buffer_data(&out_buf),
		 tn5250_buffer_length(&out_buf));
      if (ret<1) {
         errnum = SSL_get_error(This->ssl_handle, ret);
	 printf("Error in SSL_write: %s\n", ERR_error_string(errnum,NULL));
	 exit(5);
      }
      TN5250_LOG(("SentSB:<IAC><SB><TERMTYPE><IS>%s<IAC><SE>\n", termtype));

      This->status = This->status | TERMINAL;
   } else if (sb_buf[0] == NEW_ENVIRON) {
     Tn5250ConfigStr *iter;
     tn5250_buffer_append_byte(&out_buf, IAC);
     tn5250_buffer_append_byte(&out_buf, SB);
     tn5250_buffer_append_byte(&out_buf, NEW_ENVIRON);
     tn5250_buffer_append_byte(&out_buf, IS);

      if (This->config != NULL) {
	 if ((iter = This->config->vars) != NULL) {
	    do {
	      if ((strlen (iter->name) > 4) && (!memcmp (iter->name, "env.", 4))) {
		  ssl_stream_sb_var_value(&out_buf,
			(unsigned char *) iter->name + 4,
			(unsigned char *) iter->value);
	       }
	       iter = iter->next;
	    } while (iter != This->config->vars);
	 }
      }
      tn5250_buffer_append_byte(&out_buf, IAC);
      tn5250_buffer_append_byte(&out_buf, SE);

      ret = SSL_write(This->ssl_handle, (char *) tn5250_buffer_data(&out_buf),
		 tn5250_buffer_length(&out_buf));
      if (ret<1) {
         errnum = SSL_get_error(This->ssl_handle, ret);
	 printf("Error in SSL_write: %s\n", ERR_error_string(errnum,NULL));
	 exit(5);
      }
      TN5250_LOG(("SentSB:<IAC><SB>"));
      TNSB_LOG(&out_buf.data[2], out_buf.len-4);
      TN5250_LOG(("<IAC><SE>\n"));
   }
   tn5250_buffer_free(&out_buf);
}

/****i* lib5250/ssl_stream_get_byte
 * NAME
 *    ssl_stream_get_byte
 * SYNOPSIS
 *    ret = ssl_stream_get_byte (This);
 * INPUTS
 *    Tn5250Stream *       This       - 
 * DESCRIPTION
 *    Returns the next byte from the 5250 data stream, or return -1 if no data
 *    is waiting on the socket or -2 if disconnected, or -END_OF_RECORD if a 
 *    telnet EOR escape sequence was encountered.
 *****/
static int ssl_stream_get_byte(Tn5250Stream * This)
{
   unsigned char temp;
   unsigned char verb;

   do {
      if (This->state == TN5250_STREAM_STATE_NO_DATA)
	 This->state = TN5250_STREAM_STATE_DATA;

      This->rcvbufpos ++;
      if (This->rcvbufpos >= This->rcvbuflen) {
          This->rcvbufpos = 0;
          This->rcvbuflen = ssl_stream_get_next(This, This->rcvbuf, TN5250_RBSIZE);
          if (This->rcvbuflen<0) 
              return This->rcvbuflen;
      }
      temp = This->rcvbuf[This->rcvbufpos];

      switch (This->state) {
      case TN5250_STREAM_STATE_DATA:
	 if (temp == IAC)
	    This->state = TN5250_STREAM_STATE_HAVE_IAC;
	 break;

      case TN5250_STREAM_STATE_HAVE_IAC:
	switch(temp) {
	case IAC:
	  This->state = TN5250_STREAM_STATE_DATA;
	  break;

	 case DO:
	 case DONT:
	 case WILL:
	 case WONT:
	    verb = temp;
	    This->state = TN5250_STREAM_STATE_HAVE_VERB;
	    break;

	 case SB:
	    This->state = TN5250_STREAM_STATE_HAVE_SB;
	    tn5250_buffer_free(&(This->sb_buf));
	    break;

	 case EOR:
	    This->state = TN5250_STREAM_STATE_DATA;
	    return -END_OF_RECORD;

	 default:
	    TN5250_LOG(("GetByte: unknown escape 0x%02x in telnet-ssl stream.\n", temp));
	    This->state = TN5250_STREAM_STATE_NO_DATA;	/* Hopefully a good recovery. */
	 }
	 break;

      case TN5250_STREAM_STATE_HAVE_VERB:
	TN5250_LOG(("HOST, This->status  = %d %d\n", HOST, This->status));
	 if (This->status&HOST) {
	    temp = ssl_stream_host_verb(This, verb, (UCHAR) temp);
	    if (temp<1) {
               DUMP_ERR_STACK ();
	       return -2;
	    }
	    /* Implement later...
	    This->options |= temp;
	    */
	 } else
	    ssl_stream_do_verb(This, verb, (UCHAR) temp);
	 This->state = TN5250_STREAM_STATE_NO_DATA;
	 break;

      case TN5250_STREAM_STATE_HAVE_SB:
	 if (temp == IAC)
	    This->state = TN5250_STREAM_STATE_HAVE_SB_IAC;
	 else
	   tn5250_buffer_append_byte(&(This->sb_buf), (UCHAR) temp);
	 break;

      case TN5250_STREAM_STATE_HAVE_SB_IAC:
	 switch (temp) {
	 case IAC:
	    tn5250_buffer_append_byte(&(This->sb_buf), IAC);
	    /* Since the IAC code was escaped, shouldn't we be resetting the
	       state as in the following statement?  Please verify and
	       uncomment if applicable.  GJS 2/25/2000 */
	    /* This->state = TN5250_STREAM_STATE_HAVE_SB; */
	    break;

	 case SE:
	    if (This->status&HOST)
	       ssl_stream_host_sb(This, tn5250_buffer_data(&This->sb_buf),
			tn5250_buffer_length(&This->sb_buf));
	    else
	       ssl_stream_sb(This, tn5250_buffer_data(&(This->sb_buf)),
			tn5250_buffer_length(&(This->sb_buf)));

	    tn5250_buffer_free(&(This->sb_buf));
	    This->state = TN5250_STREAM_STATE_NO_DATA;
	    break;

	 default:		/* Should never happen -- server error */
	    TN5250_LOG(("GetByte: huh? Got IAC SB 0x%02X.\n", temp));
	    This->state = TN5250_STREAM_STATE_HAVE_SB;
	    break;
	 }
	 break;

      default:
	 TN5250_LOG(("GetByte: huh? Invalid state %d.\n", This->state));
	 TN5250_ASSERT(0);
	 break;			/* Avoid compiler warning. */
      }
   } while (This->state != TN5250_STREAM_STATE_DATA);
   return (int) temp;
}

/****i* lib5250/ssl_stream_write
 * NAME
 *    ssl_stream_write
 * SYNOPSIS
 *    ssl_stream_write (This, data, size);
 * INPUTS
 *    Tn5250Stream *       This       - 
 *    unsigned char *      data       - 
 *    int                  size       - 
 * DESCRIPTION
 *    Writes size bytes of data (pointed to by *data) to the 5250 data stream.
 *    This is also a temporary method to aid in the conversion process.  
 *****/
static void ssl_stream_write(Tn5250Stream * This, unsigned char *data, int size)
{
   int r;
   fd_set fdw;

   while (size>0) {

      r = SSL_write(This->ssl_handle, data, size);
      if (r < 1) {
           errnum = SSL_get_error(This->ssl_handle, r);
           if ((errnum!=SSL_ERROR_WANT_READ)&&(errnum!=SSL_ERROR_WANT_WRITE)) {
           }
           FD_ZERO(&fdw);
           FD_SET(This->sockfd, &fdw);
           if (errnum==SSL_ERROR_WANT_READ)  
                select(This->sockfd+1, &fdw, NULL, NULL, NULL);
           else
                select(This->sockfd+1, NULL, &fdw, NULL, NULL);
      }
      else {      
          data += r;
          size -= r;
      }

   }
           
   return;
}

/****i* lib5250/ssl_stream_send_packet
 * NAME
 *    ssl_stream_send_packet
 * SYNOPSIS
 *    ssl_stream_send_packet (This, length, flowtype, flags, opcode, data);
 * INPUTS
 *    Tn5250Stream *       This       - 
 *    int                  length     - 
 *    int                  flowtype   -
 *    unsigned char        flags      -
 *    unsgined char        opcode     -
 *    unsigned char *      data       - 
 * DESCRIPTION
 *    Send a packet, prepending a header and escaping any naturally
 *    occuring IAC characters.
 *****/
static void ssl_stream_send_packet(Tn5250Stream * This, int length, 
				      StreamHeader header, unsigned char *data)
{
   Tn5250Buffer out_buf;
   int n;
   int flowtype;
   unsigned char flags;
   unsigned char opcode;

   flowtype = header.h5250.flowtype;
   flags = header.h5250.flags;
   opcode = header.h5250.opcode;

   length = length + 10;

   /* Fixed length portion of header */
   tn5250_buffer_init(&out_buf);
   tn5250_buffer_append_byte(&out_buf, (UCHAR) (((short)length)>>8));
   tn5250_buffer_append_byte(&out_buf, (UCHAR) (length & 0xff));
   tn5250_buffer_append_byte(&out_buf, 0x12);	/* Record type = General data stream (GDS) */
   tn5250_buffer_append_byte(&out_buf, 0xa0);
   tn5250_buffer_append_byte(&out_buf, (UCHAR)(flowtype >> 8));
   tn5250_buffer_append_byte(&out_buf, (UCHAR) (flowtype & 0xff));

   /* Variable length portion of header */
   tn5250_buffer_append_byte(&out_buf, 4);
   tn5250_buffer_append_byte(&out_buf, flags);
   tn5250_buffer_append_byte(&out_buf, 0);
   tn5250_buffer_append_byte(&out_buf, opcode);
   tn5250_buffer_append_data(&out_buf, data, length - 10);

   ssl_stream_escape(&out_buf);

   tn5250_buffer_append_byte(&out_buf, IAC);
   tn5250_buffer_append_byte(&out_buf, EOR);

#ifndef NDEBUG
   TN5250_LOG(("SendPacket: length = %d\nSendPacket: data follows.",
	tn5250_buffer_length(&out_buf)));
   for (n = 0; n < tn5250_buffer_length(&out_buf); n++) {
      if ((n % 16) == 0) {
	 TN5250_LOG(("\nSendPacket: data: "));
      }
      TN5250_LOG(("%02X ", tn5250_buffer_data(&out_buf)[n]));
   }
   TN5250_LOG(("\n"));
#endif

   ssl_stream_write(This, tn5250_buffer_data(&out_buf), tn5250_buffer_length(&out_buf));
   tn5250_buffer_free(&out_buf);
}

void
tn3270_ssl_stream_send_packet(Tn5250Stream * This, int length,
			  StreamHeader header,
			  unsigned char * data)
{
  Tn5250Buffer out_buf;

  tn5250_buffer_init(&out_buf);

  if(This->streamtype == TN3270E_STREAM)
    {
      tn5250_buffer_append_byte(&out_buf, header.h3270.data_type);
      tn5250_buffer_append_byte(&out_buf, header.h3270.request_flag);
      tn5250_buffer_append_byte(&out_buf, header.h3270.response_flag);

      tn5250_buffer_append_byte(&out_buf, header.h3270.sequence >> 8);
      tn5250_buffer_append_byte(&out_buf, header.h3270.sequence & 0x00ff);
    }
  
  tn5250_buffer_append_data(&out_buf, data, length);
      
  ssl_stream_escape(&out_buf);

  tn5250_buffer_append_byte(&out_buf, IAC);
  tn5250_buffer_append_byte(&out_buf, EOR);
   
  ssl_stream_write(This, tn5250_buffer_data(&out_buf), 
		      tn5250_buffer_length(&out_buf));

  tn5250_buffer_free(&out_buf);

}

/****f* lib5250/ssl_stream_handle_receive
 * NAME
 *    ssl_stream_handle_receive
 * SYNOPSIS
 *    ret = ssl_stream_handle_receive (This);
 * INPUTS
 *    Tn5250Stream *       This       - 
 * DESCRIPTION
 *    Read as much data as possible in a non-blocking fasion, form it
 *    into Tn5250Record structures and queue them for retrieval.
 *****/
int ssl_stream_handle_receive(Tn5250Stream * This)
{
   int c;

   fd_set rdwait;
   struct timeval tv;

   /* 
    *  note that we have to do this here, not in _get_byte, because
    *  we need to know that the SSL's internal buffer is empty, and
    *  that SSL_read is not waiting for space in the write buffer,
    *  before we can safely call select().
    *
    *  Actually, not sure why we have to do this at all.  Doesn't the
    *  work in terminal_waitevent do this already?  -SCK
    *
    */
   if (This->msec_wait > 0) {
       tv.tv_sec = This->msec_wait / 1000;
       tv.tv_usec = (This->msec_wait % 1000) * 1000;
       FD_ZERO(&rdwait);
       FD_SET(This->sockfd, &rdwait);
       select(This->sockfd+1, &rdwait, NULL, NULL, &tv);
   }

   /* -1 = no more data, -2 = we've been disconnected */
   while ((c = ssl_stream_get_byte(This)) != -1 && c != -2) {

      if (c == -END_OF_RECORD && This->current_record != NULL) {
	 /* End of current packet. */
#ifndef NDEBUG
         if (tn5250_logfile!=NULL) 
             tn5250_record_dump(This->current_record);
#endif
	 This->records = tn5250_record_list_add(This->records, This->current_record);
	 This->current_record = NULL;
	 This->record_count++;
	 continue;
      }
      if (This->current_record == NULL) {
	 /* Start of new packet. */
	 This->current_record = tn5250_record_new();
      }
      tn5250_record_append_byte(This->current_record, (unsigned char) c);
   }

   return (c != -2);
}

/****i* lib5250/ssl_stream_escape
 * NAME
 *    ssl_stream_escape
 * SYNOPSIS
 *    ssl_stream_escape (in);
 * INPUTS
 *    Tn5250Buffer *       in         - 
 * DESCRIPTION
 *    Escape IACs in data before sending it to the host.
 *****/
static void ssl_stream_escape(Tn5250Buffer * in)
{
   Tn5250Buffer out;
   register unsigned char c;
   int n;

   tn5250_buffer_init(&out);
   for (n = 0; n < tn5250_buffer_length(in); n++) {
      c = tn5250_buffer_data(in)[n];
      tn5250_buffer_append_byte(&out, c);
      if (c == IAC)
	 tn5250_buffer_append_byte(&out, IAC);
   }
   tn5250_buffer_free(in);
   memcpy(in, &out, sizeof(Tn5250Buffer));
}

/****i* lib5250/ssl_stream_passwd_cb
 * NAME
 *    ssl_stream_passwd_cb
 * SYNOPSIS
 *    ssl_stream_passwd_cb (buf, sizeof(buf), rwflag, userdata);
 * INPUTS
 *    char  *              buf        -
 *    int                  size       -
 *    int                  rwflag     -
 *    void  *              userdata   -
 * DESCRIPTION
 *    This is a callback function that's passed to OpenSSL.  When
 *    OpenSSL needs a password for a Private Key file, it will call
 *    this function.
 *****/
int ssl_stream_passwd_cb(char *buf, int size, int rwflag, Tn5250Stream *This) {

/** FIXME:  There might be situations when we want to ask the user for
    the passphrase rather than have it supplied by a config option?  **/

    strncpy(buf, This->userdata, size);
    buf[size - 1] = '\0';
    return(strlen(buf));

}

X509 *ssl_stream_load_cert(Tn5250Stream *This, const char *file) {

    BIO *cf;
    X509 *x;

    if ((cf = BIO_new(BIO_s_file())) == NULL) {
        DUMP_ERR_STACK();
        return NULL;
    }

    if (BIO_read_filename(cf, file) <= 0) {
        DUMP_ERR_STACK();
        return NULL;
    }

    x = PEM_read_bio_X509_AUX(cf, NULL,
            (pem_password_cb *)ssl_stream_passwd_cb, This);

    BIO_free(cf);

    return (x);
}

#endif /* HAVE_LIBSSL */

/* vi:set sts=3 sw=3: */

