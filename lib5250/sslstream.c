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

#if defined(__SVR4) && defined(__sun)
#include <sys/filio.h>
#endif

static int ssl_stream_get_next(Tn5250Stream* This, unsigned char* buf,
                               int size);
static void ssl_stream_do_verb(Tn5250Stream* This, unsigned char verb,
                               unsigned char what);
static void ssl_stream_sb_var_value(Tn5250Buffer* buf, unsigned char* var,
                                    unsigned char* value);
static void ssl_stream_sb(Tn5250Stream* This, unsigned char* sb_buf,
                          int sb_len);
static void ssl_stream_escape(Tn5250Buffer* buffer);
static void ssl_stream_write(Tn5250Stream* This, unsigned char* data, int size);
static int ssl_stream_get_byte(Tn5250Stream* This);

static int ssl_stream_connect(Tn5250Stream* This, const char* to);
static void ssl_stream_destroy(Tn5250Stream* This);
static void ssl_stream_disconnect(Tn5250Stream* This);
static int ssl_stream_handle_receive(Tn5250Stream* This);
static void ssl_stream_send_packet(Tn5250Stream* This, int length,
                                   StreamHeader header, unsigned char* data);
int ssl_stream_passwd_cb(char* buf, int size, int rwflag, Tn5250Stream* This);
X509* ssl_stream_load_cert(Tn5250Stream* This, const char* file);

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

#define TRANSMIT_BINARY 0
#define END_OF_RECORD   25
#define TERMINAL_TYPE   24
#define TIMING_MARK     6
#define NEW_ENVIRON     39

#define EOR  239
#define SE   240
#define SB   250
#define WILL 251
#define WONT 252
#define DO   253
#define DONT 254
#define IAC  255

#define TN5250_STREAM_STATE_NO_DATA     0 /* Dummy state */
#define TN5250_STREAM_STATE_DATA        1
#define TN5250_STREAM_STATE_HAVE_IAC    2
#define TN5250_STREAM_STATE_HAVE_VERB   3 /* e.g. DO, DONT, WILL, WONT */
#define TN5250_STREAM_STATE_HAVE_SB     4 /* SB data */
#define TN5250_STREAM_STATE_HAVE_SB_IAC 5

/* Internal Telnet option settings (bit-wise flags) */
#define RECV_BINARY 1
#define SEND_BINARY 2
#define RECV_EOR    4
#define SEND_EOR    8

#ifndef HAVE_UCHAR
typedef unsigned char UCHAR;
#endif

static const UCHAR hostInitStr[] = { IAC, DO, NEW_ENVIRON,
                                     IAC, DO, TERMINAL_TYPE };
static const UCHAR hostDoEOR[] = { IAC, DO, END_OF_RECORD };
static const UCHAR hostDoBinary[] = { IAC, DO, TRANSMIT_BINARY };
typedef struct doTable_t {
    const UCHAR* cmd;
    unsigned len;
} DOTABLE;

static const DOTABLE host5250DoTable[] = { hostInitStr,  sizeof(hostInitStr),
                                           hostDoEOR,    sizeof(hostDoEOR),
                                           hostDoBinary, sizeof(hostDoBinary),
                                           NULL,         0 };

static const UCHAR SB_Str_NewEnv[] = { IAC, SB,  NEW_ENVIRON, SEND, USERVAR,
                                       'I', 'B', 'M',         'R',  'S',
                                       'E', 'E', 'D',         0,    1,
                                       2,   3,   4,           5,    6,
                                       7,   VAR, USERVAR,     IAC,  SE };
static const UCHAR SB_Str_TermType[] = {
    IAC, SB, TERMINAL_TYPE, SEND, IAC, SE
};

/* FIXME: This should be added to Tn5250Stream structure, or something
    else better than this :) */
int errnum;

#ifdef NDEBUG
#define IACVERB_LOG(tag, verb, what)
#define TNSB_LOG(sb_buf, sb_len)
#define LOGERROR(tag, ecode)
#define DUMP_ERR_STACK()
#else
#define IACVERB_LOG    ssl_log_IAC_verb
#define TNSB_LOG       ssl_log_SB_buf
#define LOGERROR       ssl_logError
#define DUMP_ERR_STACK ssl_log_error_stack

static char* ssl_getTelOpt(unsigned char what) {
    char* wcp;
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
        snprintf(wcp = wbuf, sizeof(wbuf), "<%02X>", what);
        break;
    }
    return wcp;
}

static void ssl_log_error_stack(void) {
    FILE* errfp = tn5250_logfile ? tn5250_logfile : stderr;

    ERR_print_errors_fp(errfp);
}

static void ssl_logError(char* tag, int ecode) {
    FILE* errfp = tn5250_logfile ? tn5250_logfile : stderr;

    fprintf(errfp, "%s: ERROR (code=%d) - %s\n", tag, ecode, strerror(ecode));
}

static void ssl_log_IAC_verb(char* tag, int verb, int what) {
    char *vcp, vbuf[10];

    if (!tn5250_logfile) {
        return;
    }
    switch (verb) {
    case DO:
        vcp = "<DO>";
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
        sprintf(vcp = vbuf, "<%02X>", verb);
        break;
    }
    fprintf(tn5250_logfile, "%s:<IAC>%s%s\n", tag, vcp, ssl_getTelOpt(what));
}

static int ssl_dumpVarVal(UCHAR* buf, int len) {
    int c, i;

    for (c = buf[i = 0]; i < len && c != VAR && c != VALUE && c != USERVAR;
         c = buf[++i]) {
        if (isprint(c)) {
            putc(c, tn5250_logfile);
        }
        else {
            fprintf(tn5250_logfile, "<%02X>", c);
        }
    }
    return i;
}

static int ssl_dumpNewEnv(unsigned char* buf, int len) {
    int c, i = 0, j;

    while (i < len) {
        switch (c = buf[i]) {
        case IAC:
            return i;
        case VAR:
            fputs("\n\t<VAR>", tn5250_logfile);
            if (++i < len && buf[i] == USERVAR) {
                fputs("<USERVAR>", tn5250_logfile);
                return i + 1;
            }
            j = ssl_dumpVarVal(buf + i, len - i);
            i += j;
        case USERVAR:
            fputs("\n\t<USERVAR>", tn5250_logfile);
            if (!memcmp("IBMRSEED", &buf[++i], 8)) {
                fputs("IBMRSEED", tn5250_logfile);
                putc('<', tn5250_logfile);
                for (j = 0, i += 8; j < 8; i++, j++) {
                    if (j) {
                        putc(' ', tn5250_logfile);
                    }
                    fprintf(tn5250_logfile, "%02X", buf[i]);
                }
                putc('>', tn5250_logfile);
            }
            else {
                j = ssl_dumpVarVal(buf + i, len - i);
                i += j;
            }
            break;
        case VALUE:
            fputs("<VALUE>", tn5250_logfile);
            i++;
            j = ssl_dumpVarVal(buf + i, len - i);
            i += j;
            break;
        default:
            fputs(ssl_getTelOpt(c), tn5250_logfile);
        } /* switch */
    }     /* while */
    return i;
}

static void ssl_log_SB_buf(unsigned char* buf, int len) {
    int c, i, type;

    if (!tn5250_logfile) {
        return;
    }
    fprintf(tn5250_logfile, "%s", ssl_getTelOpt(type = *buf++));
    switch (c = *buf++) {
    case IS:
        fputs("<IS>", tn5250_logfile);
        break;
    case SEND:
        fputs("<SEND>", tn5250_logfile);
        break;
    default:
        fputs(ssl_getTelOpt(c), tn5250_logfile);
    }
    len -= 2;
    i = (type == NEW_ENVIRON) ? ssl_dumpNewEnv(buf, len) : 0;
    while (i < len) {
        switch (c = buf[i++]) {
        case IAC:
            fputs("<IAC>", tn5250_logfile);
            if (i < len) {
                fputs(ssl_getTelOpt(buf[i++]), tn5250_logfile);
            }
            break;
        default:
            if (isprint(c)) {
                putc(c, tn5250_logfile);
            }
            else {
                fprintf(tn5250_logfile, "<%02X>", c);
            }
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
int tn5250_ssl_stream_init(Tn5250Stream* This) {
    int len;
    char methstr[5];
    const SSL_METHOD* meth = NULL;

    TN5250_LOG(("tn5250_ssl_stream_init() entered.\n"));

    /*  initialize SSL library */

    SSL_load_error_strings();
    OPENSSL_init_ssl(0, NULL);

    /*  which SSL method do we use? */

    /* Ignore the user's choice of ssl_method (which isn't documented
     * anyway...) if it was either "ssl2" or "ssl3". Both are insecure,
     * and this is only safe supported method left.
     *
     * This is a Gentoo-specific modification that lets us build
     * against LibreSSL and newer OpenSSL with its insecure protocols
     * disabled.
     */
    meth = SSLv23_client_method();
    TN5250_LOG(("SSL Method = SSLv23_client_method()\n"));

    /*  create a new SSL context */

    This->ssl_context = SSL_CTX_new(meth);
    if (This->ssl_context == NULL) {
        DUMP_ERR_STACK();
        return -1;
    }

    /* if a certificate authority file is defined, load it into this context */

    if (This->config != NULL &&
        tn5250_config_get(This->config, "ssl_ca_file")) {
        if (SSL_CTX_load_verify_locations(
                This->ssl_context,
                tn5250_config_get(This->config, "ssl_ca_file"), NULL) < 1) {
            DUMP_ERR_STACK();
            return -1;
        }
    }

    This->userdata = NULL;

    /* if a PEM passphrase is defined, set things up so that it can be used */

    if (This->config != NULL &&
        tn5250_config_get(This->config, "ssl_pem_pass")) {
        TN5250_LOG(("SSL: Setting password callback\n"));
        len = strlen(tn5250_config_get(This->config, "ssl_pem_pass"));
        This->userdata = malloc(len + 1);
        strncpy(This->userdata, tn5250_config_get(This->config, "ssl_pem_pass"),
                len);
        SSL_CTX_set_default_passwd_cb(This->ssl_context,
                                      (pem_password_cb*)ssl_stream_passwd_cb);
        SSL_CTX_set_default_passwd_cb_userdata(This->ssl_context, (void*)This);
    }

    /* If a certificate file has been defined, load it into this context as well
     */

    if (This->config != NULL &&
        tn5250_config_get(This->config, "ssl_cert_file")) {

        if (tn5250_config_get(This->config, "ssl_check_exp")) {
            X509* client_cert;
            time_t tnow;
            int extra_time;
            TN5250_LOG(("SSL: Checking expiration of client cert\n"));
            client_cert = ssl_stream_load_cert(
                This, tn5250_config_get(This->config, "ssl_cert_file"));
            if (client_cert == NULL) {
                TN5250_LOG(("SSL: Unable to load client certificate!\n"));
                return -1;
            }
            extra_time = tn5250_config_get_int(This->config, "ssl_check_exp");
            tnow = time(NULL) + extra_time;
            if (ASN1_UTCTIME_cmp_time_t(X509_get_notAfter(client_cert), tnow) ==
                -1) {
                if (extra_time > 1) {
                    printf("SSL error: client certificate will be expired\n");
                    TN5250_LOG(("SSL: client certificate will be expired\n"));
                }
                else {
                    printf("SSL error: client certificate has expired\n");
                    TN5250_LOG(("SSL: client certificate has expired\n"));
                }
                return -1;
            }
            X509_free(client_cert);
        }

        TN5250_LOG(("SSL: Loading certificates from certificate file\n"));
        if (SSL_CTX_use_certificate_file(
                This->ssl_context,
                tn5250_config_get(This->config, "ssl_cert_file"),
                SSL_FILETYPE_PEM) <= 0) {
            DUMP_ERR_STACK();
            return -1;
        }
        TN5250_LOG(("SSL: Loading private keys from certificate file\n"));
        if (SSL_CTX_use_PrivateKey_file(
                This->ssl_context,
                tn5250_config_get(This->config, "ssl_cert_file"),
                SSL_FILETYPE_PEM) <= 0) {
            DUMP_ERR_STACK();
            return -1;
        }
    }

    This->ssl_handle = NULL;

    This->connect = ssl_stream_connect;
    This->disconnect = ssl_stream_disconnect;
    This->handle_receive = ssl_stream_handle_receive;
    This->send_packet = ssl_stream_send_packet;
    This->destroy = ssl_stream_destroy;
    TN5250_LOG(("tn5250_ssl_stream_init() success.\n"));
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
static int ssl_stream_connect(Tn5250Stream* This, const char* to) {
    int ioctlarg = 1;
    // Should hold a hostname + :port/service name
    char address[512], *host, *port;
    int r;
    X509* server_cert;
    long certvfy;

    TN5250_LOG(("tn5250_ssl_stream_connect() entered.\n"));

    /* Figure out the internet address. */
    strncpy(address, to, sizeof(address));
    // If this is an IPv6 address, the port separate is after the brackets
    if ((host = strchr(address, '['))) {
        host++;
        char* host_end = strrchr(address, ']');
        if (host_end == NULL) {
            _tn5250_set_error(TN5250_ERROR_INTERNAL,
                              TN5250_INTERNALERROR_INVALIDADDRESS);
            return -1;
        }
        if ((port = strchr(host_end, ':'))) {
            *port++ = '\0';
        }
        *host_end = '\0';
    }
    else if ((port = strchr(address, ':'))) {
        // IPv4 or hostname, only colon is for port
        *port++ = '\0';
    }

    if (host == NULL) {
        host = address;
    }
    if (port == NULL) {
        port = "telnets";
    }

    struct addrinfo* result;
    struct addrinfo hints = { .ai_family = AF_UNSPEC,
                              .ai_socktype = SOCK_STREAM };

    r = getaddrinfo(host, port, &hints, &result);
    if (r == EAI_NONAME && strcmp(port, "telnets") == 0) {
        hints.ai_flags |= AI_NUMERICSERV;
        freeaddrinfo(result);
        r = getaddrinfo(host, "992", &hints, &result);
    }

    if (r != 0) {
        freeaddrinfo(result);
        _tn5250_set_error(TN5250_ERROR_GAI, r);
        return r;
    }

    This->ssl_handle = SSL_new(This->ssl_context);
    if (This->ssl_handle == NULL) {
        _tn5250_set_error(TN5250_ERROR_SSL, ERR_peek_error());
        DUMP_ERR_STACK();
        TN5250_LOG(("sslstream: SSL_new() failed!\n"));
        return -1;
    }

    // Provide host name for SNI; IBM i doesn't do anything with this AFAIK,
    // but a proxy might. Note that 0 indicates failure.
    r = SSL_set_tlsext_host_name(This->ssl_handle, host);
    if (r == 0) {
        errnum = SSL_get_error(This->ssl_handle, r);
        TN5250_LOG(("sslstream: SSL_set_tlsext_host_name() failed!, host=%s "
                    "errnum=%d\n",
                    host, errnum));
        // Not fatal, can continue?
    }

    for (struct addrinfo* addr = result; addr; addr = addr->ai_next) {
        This->sockfd =
            socket(addr->ai_family, addr->ai_socktype, addr->ai_protocol);
        if (WAS_INVAL_SOCK(This->sockfd)) {
            _tn5250_set_error(TN5250_ERROR_ERRNO, LAST_ERROR);
            TN5250_LOG(("sslstream: socket() failed, errno=%d\n", errno));
            freeaddrinfo(result);
            return -1;
        }

        if ((r = SSL_set_fd(This->ssl_handle, This->sockfd)) == 0) {
            _tn5250_set_error(TN5250_ERROR_SSL, ERR_peek_error());
            errnum = SSL_get_error(This->ssl_handle, r);
            DUMP_ERR_STACK();
            TN5250_LOG(("sslstream: SSL_set_fd() failed, errnum=%d\n", errnum));
            freeaddrinfo(result);
            return errnum;
        }

        r = connect(This->sockfd, addr->ai_addr, addr->ai_addrlen);
        if (r == 0) {
            break;
        }
    }

    freeaddrinfo(result);
    if (WAS_ERROR_RET(r)) {
        _tn5250_set_error(TN5250_ERROR_ERRNO, LAST_ERROR);
        TN5250_LOG(("sslstream: connect() failed, errno=%d\n", errno));
        return -1;
    }

    if ((r = SSL_connect(This->ssl_handle) < 1)) {
        _tn5250_set_error(TN5250_ERROR_SSL, ERR_peek_error());
        errnum = SSL_get_error(This->ssl_handle, r);
        DUMP_ERR_STACK();
        TN5250_LOG(("sslstream: SSL_connect() failed, errnum=%d\n", errnum));
        return errnum;
    }

    TN5250_LOG(("Connected with SSL\n"));
    TN5250_LOG(("Using %s cipher with a %d bit secret key\n",
                SSL_get_cipher_name(This->ssl_handle),
                SSL_get_cipher_bits(This->ssl_handle, NULL)));

    server_cert = SSL_get_peer_certificate(This->ssl_handle);

    if (server_cert == NULL) {
        _tn5250_set_error(TN5250_ERROR_INTERNAL,
                          TN5250_INTERNALERROR_INVALIDCERT);
        TN5250_LOG(("sslstream: Server did not present a certificate!\n"));
        return -1;
    }
    else {
        time_t tnow = time(NULL);
        int extra_time = 0;
        if (This->config != NULL &&
            tn5250_config_get(This->config, "ssl_check_exp") != NULL) {
            extra_time = tn5250_config_get_int(This->config, "ssl_check_exp");
            tnow += extra_time;
            if (ASN1_UTCTIME_cmp_time_t(X509_get_notAfter(server_cert), tnow) ==
                -1) {
                if (extra_time > 1) {
                    printf("SSL error: server certificate will be expired\n");
                    TN5250_LOG(("SSL: server certificate will be expired\n"));
                }
                else {
                    printf("SSL error: server certificate has expired\n");
                    TN5250_LOG(("SSL: server certificate has expired\n"));
                }
                _tn5250_set_error(TN5250_ERROR_SSL, ERR_peek_error());
                return -1;
            }
        }
        TN5250_LOG(
            ("SSL Certificate issued by: %s\n",
             X509_NAME_oneline(X509_get_issuer_name(server_cert), 0, 0)));
        certvfy = SSL_get_verify_result(This->ssl_handle);
        if (certvfy == X509_V_OK) {
            TN5250_LOG(("SSL Certificate successfully verified!\n"));
        }
        else {
            TN5250_LOG(
                ("SSL Certificate verification failed, reason: %d\n", certvfy));
            if (This->config != NULL &&
                tn5250_config_get_bool(This->config, "ssl_verify_server")) {
                // XXX: Stringify certvfy?
                _tn5250_set_error(TN5250_ERROR_INTERNAL,
                                  TN5250_INTERNALERROR_INVALIDCERT);
                return -1;
            }
        }
    }

    /* Set socket to non-blocking mode. */
    TN5250_LOG(("SSL must be Non-Blocking\n"));
    TN_IOCTL(This->sockfd, FIONBIO, &ioctlarg);

    This->state = TN5250_STREAM_STATE_DATA;
    TN5250_LOG(("tn5250_ssl_stream_connect() success.\n"));
    return 0;
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
static void ssl_stream_disconnect(Tn5250Stream* This) {
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
static void ssl_stream_destroy(Tn5250Stream* This) { /* noop */
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
static int ssl_stream_get_next(Tn5250Stream* This, unsigned char* buf,
                               int size) {

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
                select(This->sockfd + 1, NULL, &wrwait, NULL, NULL);
                break;
            case SSL_ERROR_WANT_READ:
                return -1;
                break;
            default:
                return -2;
                break;
            }
        }
    } while (rc < 1);

    return rc;
}

static int ssl_sendWill(Tn5250Stream* This, unsigned char what) {
    UCHAR buff[3] = { IAC, WILL };
    buff[2] = what;
    TN5250_LOG(("SSL_Write: %x %x %x\n", buff[0], buff[1], buff[2]));
    return SSL_write(This->ssl_handle, buff, 3);
}

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
static void ssl_stream_do_verb(Tn5250Stream* This, unsigned char verb,
                               unsigned char what) {
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

    IACVERB_LOG("GotVerb(3)", verb, what);
    TN5250_LOG(("SSL_Write: %x %x %x\n", reply[0], reply[1], reply[2]));
    ret = SSL_write(This->ssl_handle, (char*)reply, 3);
    if (ret < 1) {
        errnum = SSL_get_error(This->ssl_handle, ret);
        printf("Error writing to socket: %s\n", ERR_error_string(errnum, NULL));
        exit(5);
    }
}

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
static void ssl_stream_sb_var_value(Tn5250Buffer* buf, unsigned char* var,
                                    unsigned char* value) {
    tn5250_buffer_append_byte(buf, VAR);
    tn5250_buffer_append_data(buf, var, strlen((char*)var));
    tn5250_buffer_append_byte(buf, VALUE);
    tn5250_buffer_append_data(buf, value, strlen((char*)value));
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
static void ssl_stream_sb(Tn5250Stream* This, unsigned char* sb_buf,
                          int sb_len) {
    Tn5250Buffer out_buf;
    int ret;

    TN5250_LOG(("GotSB:<IAC><SB>"));
    TNSB_LOG(sb_buf, sb_len);
    TN5250_LOG(("<IAC><SE>\n"));

    tn5250_buffer_init(&out_buf);

    if (sb_len <= 0) {
        return;
    }

    if (sb_buf[0] == TERMINAL_TYPE) {
        unsigned char* termtype;

        if (sb_buf[1] != SEND) {
            return;
        }

        termtype = (unsigned char*)tn5250_stream_getenv(This, "TERM");

        tn5250_buffer_append_byte(&out_buf, IAC);
        tn5250_buffer_append_byte(&out_buf, SB);
        tn5250_buffer_append_byte(&out_buf, TERMINAL_TYPE);
        tn5250_buffer_append_byte(&out_buf, IS);
        tn5250_buffer_append_data(&out_buf, termtype, strlen((char*)termtype));
        tn5250_buffer_append_byte(&out_buf, IAC);
        tn5250_buffer_append_byte(&out_buf, SE);

        ret = SSL_write(This->ssl_handle, (char*)tn5250_buffer_data(&out_buf),
                        tn5250_buffer_length(&out_buf));
        if (ret < 1) {
            errnum = SSL_get_error(This->ssl_handle, ret);
            printf("Error in SSL_write: %s\n", ERR_error_string(errnum, NULL));
            exit(5);
        }
        TN5250_LOG(("SentSB:<IAC><SB><TERMTYPE><IS>%s<IAC><SE>\n", termtype));

        This->status = This->status | TERMINAL;
    }
    else if (sb_buf[0] == NEW_ENVIRON) {
        Tn5250ConfigStr* iter;
        tn5250_buffer_append_byte(&out_buf, IAC);
        tn5250_buffer_append_byte(&out_buf, SB);
        tn5250_buffer_append_byte(&out_buf, NEW_ENVIRON);
        tn5250_buffer_append_byte(&out_buf, IS);

        if (This->config != NULL) {
            if ((iter = This->config->vars) != NULL) {
                do {
                    if ((strlen(iter->name) > 4) &&
                        (!memcmp(iter->name, "env.", 4))) {
                        ssl_stream_sb_var_value(&out_buf,
                                                (unsigned char*)iter->name + 4,
                                                (unsigned char*)iter->value);
                    }
                    iter = iter->next;
                } while (iter != This->config->vars);
            }
        }
        tn5250_buffer_append_byte(&out_buf, IAC);
        tn5250_buffer_append_byte(&out_buf, SE);

        ret = SSL_write(This->ssl_handle, (char*)tn5250_buffer_data(&out_buf),
                        tn5250_buffer_length(&out_buf));
        if (ret < 1) {
            errnum = SSL_get_error(This->ssl_handle, ret);
            printf("Error in SSL_write: %s\n", ERR_error_string(errnum, NULL));
            exit(5);
        }
        TN5250_LOG(("SentSB:<IAC><SB>"));
        TNSB_LOG(&out_buf.data[2], out_buf.len - 4);
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
static int ssl_stream_get_byte(Tn5250Stream* This) {
    unsigned char temp;
    unsigned char verb;

    do {
        if (This->state == TN5250_STREAM_STATE_NO_DATA) {
            This->state = TN5250_STREAM_STATE_DATA;
        }

        This->rcvbufpos++;
        if (This->rcvbufpos >= This->rcvbuflen) {
            This->rcvbufpos = 0;
            This->rcvbuflen =
                ssl_stream_get_next(This, This->rcvbuf, TN5250_RBSIZE);
            if (This->rcvbuflen < 0) {
                return This->rcvbuflen;
            }
        }
        temp = This->rcvbuf[This->rcvbufpos];

        switch (This->state) {
        case TN5250_STREAM_STATE_DATA:
            if (temp == IAC) {
                This->state = TN5250_STREAM_STATE_HAVE_IAC;
            }
            break;

        case TN5250_STREAM_STATE_HAVE_IAC:
            switch (temp) {
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
                TN5250_LOG(
                    ("GetByte: unknown escape 0x%02x in telnet-ssl stream.\n",
                     temp));
                This->state = TN5250_STREAM_STATE_NO_DATA; /* Hopefully a good
                                                              recovery. */
            }
            break;

        case TN5250_STREAM_STATE_HAVE_VERB:
            TN5250_LOG(("This->status  = %d\n", This->status));
            ssl_stream_do_verb(This, verb, (UCHAR)temp);
            This->state = TN5250_STREAM_STATE_NO_DATA;
            break;

        case TN5250_STREAM_STATE_HAVE_SB:
            if (temp == IAC) {
                This->state = TN5250_STREAM_STATE_HAVE_SB_IAC;
            }
            else {
                tn5250_buffer_append_byte(&(This->sb_buf), (UCHAR)temp);
            }
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
                ssl_stream_sb(This, tn5250_buffer_data(&(This->sb_buf)),
                              tn5250_buffer_length(&(This->sb_buf)));

                tn5250_buffer_free(&(This->sb_buf));
                This->state = TN5250_STREAM_STATE_NO_DATA;
                break;

            default: /* Should never happen -- server error */
                TN5250_LOG(("GetByte: huh? Got IAC SB 0x%02X.\n", temp));
                This->state = TN5250_STREAM_STATE_HAVE_SB;
                break;
            }
            break;

        default:
            TN5250_LOG(("GetByte: huh? Invalid state %d.\n", This->state));
            TN5250_ASSERT(0);
            break; /* Avoid compiler warning. */
        }
    } while (This->state != TN5250_STREAM_STATE_DATA);
    return (int)temp;
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
static void ssl_stream_write(Tn5250Stream* This, unsigned char* data,
                             int size) {
    int r;
    fd_set fdw;

    while (size > 0) {

        r = SSL_write(This->ssl_handle, data, size);
        if (r < 1) {
            errnum = SSL_get_error(This->ssl_handle, r);
            if ((errnum != SSL_ERROR_WANT_READ) &&
                (errnum != SSL_ERROR_WANT_WRITE)) {}
            FD_ZERO(&fdw);
            FD_SET(This->sockfd, &fdw);
            if (errnum == SSL_ERROR_WANT_READ) {
                select(This->sockfd + 1, &fdw, NULL, NULL, NULL);
            }
            else {
                select(This->sockfd + 1, NULL, &fdw, NULL, NULL);
            }
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
static void ssl_stream_send_packet(Tn5250Stream* This, int length,
                                   StreamHeader header, unsigned char* data) {
    Tn5250Buffer out_buf;
    int n;
    int flowtype;
    unsigned char flags;
    unsigned char opcode;

    flowtype = header.flowtype;
    flags = header.flags;
    opcode = header.opcode;

    length = length + 10;

    /* Fixed length portion of header */
    tn5250_buffer_init(&out_buf);
    tn5250_buffer_append_byte(&out_buf, (UCHAR)(((short)length) >> 8));
    tn5250_buffer_append_byte(&out_buf, (UCHAR)(length & 0xff));
    tn5250_buffer_append_byte(
        &out_buf, 0x12); /* Record type = General data stream (GDS) */
    tn5250_buffer_append_byte(&out_buf, 0xa0);
    tn5250_buffer_append_byte(&out_buf, (UCHAR)(flowtype >> 8));
    tn5250_buffer_append_byte(&out_buf, (UCHAR)(flowtype & 0xff));

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
int ssl_stream_handle_receive(Tn5250Stream* This) {
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
        select(This->sockfd + 1, &rdwait, NULL, NULL, &tv);
    }

    /* -1 = no more data, -2 = we've been disconnected */
    while ((c = ssl_stream_get_byte(This)) != -1 && c != -2) {

        if (c == -END_OF_RECORD && This->current_record != NULL) {
            /* End of current packet. */
#ifndef NDEBUG
            if (tn5250_logfile != NULL) {
                tn5250_record_dump(This->current_record);
            }
#endif
            This->records =
                tn5250_record_list_add(This->records, This->current_record);
            This->current_record = NULL;
            This->record_count++;
            continue;
        }
        if (This->current_record == NULL) {
            /* Start of new packet. */
            This->current_record = tn5250_record_new();
        }
        tn5250_record_append_byte(This->current_record, (unsigned char)c);
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
static void ssl_stream_escape(Tn5250Buffer* in) {
    Tn5250Buffer out;
    register unsigned char c;
    int n;

    tn5250_buffer_init(&out);
    for (n = 0; n < tn5250_buffer_length(in); n++) {
        c = tn5250_buffer_data(in)[n];
        tn5250_buffer_append_byte(&out, c);
        if (c == IAC) {
            tn5250_buffer_append_byte(&out, IAC);
        }
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
int ssl_stream_passwd_cb(char* buf, int size, int rwflag, Tn5250Stream* This) {

    /** FIXME:  There might be situations when we want to ask the user for
        the passphrase rather than have it supplied by a config option?  **/

    strncpy(buf, This->userdata, size);
    buf[size - 1] = '\0';
    return (strlen(buf));
}

X509* ssl_stream_load_cert(Tn5250Stream* This, const char* file) {

    BIO* cf;
    X509* x;

    if ((cf = BIO_new(BIO_s_file())) == NULL) {
        DUMP_ERR_STACK();
        return NULL;
    }

    if (BIO_read_filename(cf, file) <= 0) {
        DUMP_ERR_STACK();
        return NULL;
    }

    x = PEM_read_bio_X509_AUX(cf, NULL, (pem_password_cb*)ssl_stream_passwd_cb,
                              This);

    BIO_free(cf);

    return (x);
}

#endif /* HAVE_LIBSSL */
