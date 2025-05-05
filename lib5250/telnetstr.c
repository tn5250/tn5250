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

#if defined(__SVR4) && defined(__sun)
#include <sys/filio.h>
#endif

#ifdef accept
#undef accept
#endif

static int telnet_stream_get_next(Tn5250Stream* This, unsigned char* buf,
                                  int size);
static void telnet_stream_do_verb(Tn5250Stream* This, unsigned char verb,
                                  unsigned char what);
static void telnet_stream_sb_var_value(Tn5250Buffer* buf, unsigned char* var,
                                       unsigned char* value);
static void telnet_stream_sb(Tn5250Stream* This, unsigned char* sb_buf,
                             int sb_len);
static void telnet_stream_escape(Tn5250Buffer* buffer);
static void telnet_stream_write(Tn5250Stream* This, unsigned char* data,
                                int size);
static int telnet_stream_get_byte(Tn5250Stream* This);

static int telnet_stream_connect(Tn5250Stream* This, const char* to);
static int telnet_stream_accept(Tn5250Stream* This, SOCKET_TYPE masterSock);
static void telnet_stream_destroy(Tn5250Stream* This);
static void telnet_stream_disconnect(Tn5250Stream* This);
static int telnet_stream_handle_receive(Tn5250Stream* This);
static void telnet_stream_send_packet(Tn5250Stream* This, int length,
                                      StreamHeader header, unsigned char* data);

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

#ifdef NDEBUG
#define IACVERB_LOG(tag, verb, what)
#define TNSB_LOG(sb_buf, sb_len)
#define LOGERROR(tag, ecode)
#else
#define IACVERB_LOG log_IAC_verb
#define TNSB_LOG    log_SB_buf
#define LOGERROR    logError

static char* getTelOpt(unsigned char what) {
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

static void logError(char* tag, int ecode) {
    FILE* errfp = tn5250_logfile ? tn5250_logfile : stderr;

    fprintf(errfp, "%s: ERROR (code=%d) - %s\n", tag, ecode, strerror(ecode));
}

static void log_IAC_verb(char* tag, int verb, int what) {
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
    fprintf(tn5250_logfile, "%s:<IAC>%s%s\n", tag, vcp, getTelOpt(what));
}

static int dumpVarVal(UCHAR* buf, int len) {
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

static int dumpNewEnv(unsigned char* buf, int len) {
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
            j = dumpVarVal(buf + i, len - i);
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
                j = dumpVarVal(buf + i, len - i);
                i += j;
            }
            break;
        case VALUE:
            fputs("<VALUE>", tn5250_logfile);
            i++;
            j = dumpVarVal(buf + i, len - i);
            i += j;
            break;
        default:
            fputs(getTelOpt(c), tn5250_logfile);
        } /* switch */
    }     /* while */
    return i;
}

static void log_SB_buf(unsigned char* buf, int len) {
    int c, i, type;

    if (!tn5250_logfile) {
        return;
    }
    fprintf(tn5250_logfile, "%s", getTelOpt(type = *buf++));
    switch (c = *buf++) {
    case IS:
        fputs("<IS>", tn5250_logfile);
        break;
    case SEND:
        fputs("<SEND>", tn5250_logfile);
        break;
    default:
        fputs(getTelOpt(c), tn5250_logfile);
    }
    len -= 2;
    i = (type == NEW_ENVIRON) ? dumpNewEnv(buf, len) : 0;
    while (i < len) {
        switch (c = buf[i++]) {
        case IAC:
            fputs("<IAC>", tn5250_logfile);
            if (i < len) {
                fputs(getTelOpt(buf[i++]), tn5250_logfile);
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

/****f* lib5250/tn5250_telnet_stream_init
 * NAME
 *    tn5250_telnet_stream_init
 * SYNOPSIS
 *    ret = tn5250_telnet_stream_init (This);
 * INPUTS
 *    Tn5250Stream *       This       -
 * DESCRIPTION
 *    DOCUMENT ME!!!
 *****/
int tn5250_telnet_stream_init(Tn5250Stream* This) {
    This->connect = telnet_stream_connect;
    This->disconnect = telnet_stream_disconnect;
    This->handle_receive = telnet_stream_handle_receive;
    This->send_packet = telnet_stream_send_packet;
    This->destroy = telnet_stream_destroy;
    return 0; /* Ok */
}

/****i* lib5250/telnet_stream_connect
 * NAME
 *    telnet_stream_connect
 * SYNOPSIS
 *    ret = telnet_stream_connect (This, to);
 * INPUTS
 *    Tn5250Stream *       This       -
 *    const char *         to         -
 * DESCRIPTION
 *    Connects to server.  The `to' parameter is in the form
 *    host[:port].
 *****/
static int telnet_stream_connect(Tn5250Stream* This, const char* to) {
    int ioctlarg = 1;
    // Should hold a hostname + :port/service name
    char address[512], *host, *port;
    int r;

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
        port = "telnet";
    }

    struct addrinfo* result;
    struct addrinfo hints = { .ai_family = AF_UNSPEC,
                              .ai_socktype = SOCK_STREAM };

    r = getaddrinfo(host, port, &hints, &result);
    if (r == EAI_NONAME && strcmp(port, "telnet") == 0) {
        hints.ai_flags |= AI_NUMERICSERV;
        freeaddrinfo(result);
        r = getaddrinfo(host, "23", &hints, &result);
    }

    if (r != 0) {
        freeaddrinfo(result);
        _tn5250_set_error(TN5250_ERROR_GAI, r);
        return r;
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

        r = connect(This->sockfd, addr->ai_addr, addr->ai_addrlen);
        if (r == 0) {
            break;
        }
    }

    freeaddrinfo(result);
    if (WAS_ERROR_RET(r)) {
        _tn5250_set_error(TN5250_ERROR_ERRNO, LAST_ERROR);
        return LAST_ERROR;
    }
    /* Set socket to non-blocking mode. */
#ifdef FIONBIO
    TN5250_LOG(("Non-Blocking\n"));
    TN_IOCTL(This->sockfd, FIONBIO, &ioctlarg);
#endif

    This->state = TN5250_STREAM_STATE_DATA;
    return 0;
}

/****i* lib5250/telnet_stream_disconnect
 * NAME
 *    telnet_stream_disconnect
 * SYNOPSIS
 *    telnet_stream_disconnect (This);
 * INPUTS
 *    Tn5250Stream *       This       -
 * DESCRIPTION
 *    Disconnect from the remote host.
 *****/
static void telnet_stream_disconnect(Tn5250Stream* This) {
    printf("Closing...\n");
    TN_CLOSE(This->sockfd);
}

/****i* lib5250/telnet_stream_destroy
 * NAME
 *    telnet_stream_destroy
 * SYNOPSIS
 *    telnet_stream_destroy (This);
 * INPUTS
 *    Tn5250Stream *       This       -
 * DESCRIPTION
 *    DOCUMENT ME!!!
 *****/
static void telnet_stream_destroy(Tn5250Stream* This) { /* noop */
}

/****i* lib5250/telnet_stream_get_next
 * NAME
 *    telnet_stream_get_next
 * SYNOPSIS
 *    ret = telnet_stream_get_next (This, buf, size);
 * INPUTS
 *    Tn5250Stream *       This       -
 *    unsigned char *      buf        -
 *    int                  size       -
 * DESCRIPTION
 *    Gets the next buffer of data from the socket.  The
 *    return value is the length of the data received,
 *    or -1 for no data to receive, or -2 if disconnected
 *****/
static int telnet_stream_get_next(Tn5250Stream* This, unsigned char* buf,
                                  int size) {
    int rc;
    fd_set fdr;
    struct timeval tv;

    FD_ZERO(&fdr);
    FD_SET(This->sockfd, &fdr);
    tv.tv_sec = This->msec_wait / 1000;
    tv.tv_usec = (This->msec_wait % 1000) * 1000;
    select(This->sockfd + 1, &fdr, NULL, NULL, &tv);
    if (!FD_ISSET(This->sockfd, &fdr)) {
        return -1; /* No data on socket. */
    }

    rc = recv(This->sockfd, buf, size, 0);
    if (WAS_ERROR_RET(rc)) {
        if (LAST_ERROR != ERR_AGAIN && LAST_ERROR != ERR_INTR) {
            TN5250_LOG(
                ("Error reading from socket: %s\n", strerror(LAST_ERROR)));
            return -2;
        }
        else {
            return -1;
        }
    }
    /* If the socket was readable, but there is no data, that means that we
       have been disconnected. */
    if (rc == 0) {
        return -2;
    }

    return rc;
}

static int sendWill(SOCKET_TYPE sock, unsigned char what) {
    UCHAR buff[3] = { IAC, WILL };
    buff[2] = what;
    return send(sock, buff, 3, 0);
}

/****i* lib5250/telnet_stream_do_verb
 * NAME
 *    telnet_stream_do_verb
 * SYNOPSIS
 *    telnet_stream_do_verb (This, verb, what);
 * INPUTS
 *    Tn5250Stream *       This       -
 *    unsigned char        verb       -
 *    unsigned char        what       -
 * DESCRIPTION
 *    Process the telnet DO, DONT, WILL, or WONT escape sequence.
 *****/
static void telnet_stream_do_verb(Tn5250Stream* This, unsigned char verb,
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
    ret = send(This->sockfd, (char*)reply, 3, 0);
    if (WAS_ERROR_RET(ret)) {
        printf("Error writing to socket: %s\n", strerror(LAST_ERROR));
        exit(5);
    }
}

/****i* lib5250/telnet_stream_sb_var_value
 * NAME
 *    telnet_stream_sb_var_value
 * SYNOPSIS
 *    telnet_stream_sb_var_value (buf, var, value);
 * INPUTS
 *    Tn5250Buffer *       buf        -
 *    unsigned char *      var        -
 *    unsigned char *      value      -
 * DESCRIPTION
 *    Utility function for constructing replies to NEW_ENVIRON requests.
 *****/
static void telnet_stream_sb_var_value(Tn5250Buffer* buf, unsigned char* var,
                                       unsigned char* value) {
    tn5250_buffer_append_byte(buf, VAR);
    tn5250_buffer_append_data(buf, var, strlen((char*)var));
    tn5250_buffer_append_byte(buf, VALUE);
    tn5250_buffer_append_data(buf, value, strlen((char*)value));
}

/****i* lib5250/telnet_stream_sb
 * NAME
 *    telnet_stream_sb
 * SYNOPSIS
 *    telnet_stream_sb (This, sb_buf, sb_len);
 * INPUTS
 *    Tn5250Stream *       This       -
 *    unsigned char *      sb_buf     -
 *    int                  sb_len     -
 * DESCRIPTION
 *    Handle telnet SB escapes, which are the option-specific negotiations.
 *****/
static void telnet_stream_sb(Tn5250Stream* This, unsigned char* sb_buf,
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

        ret = send(This->sockfd, (char*)tn5250_buffer_data(&out_buf),
                   tn5250_buffer_length(&out_buf), 0);
        if (WAS_ERROR_RET(ret)) {
            printf("Error writing to socket: %s\n", strerror(LAST_ERROR));
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
                        telnet_stream_sb_var_value(
                            &out_buf, (unsigned char*)iter->name + 4,
                            (unsigned char*)iter->value);
                    }
                    iter = iter->next;
                } while (iter != This->config->vars);
            }
        }
        tn5250_buffer_append_byte(&out_buf, IAC);
        tn5250_buffer_append_byte(&out_buf, SE);

        ret = send(This->sockfd, (char*)tn5250_buffer_data(&out_buf),
                   tn5250_buffer_length(&out_buf), 0);
        if (WAS_ERROR_RET(ret)) {
            printf("Error writing to socket: %s\n", strerror(LAST_ERROR));
            exit(5);
        }
        TN5250_LOG(("SentSB:<IAC><SB>"));
        TNSB_LOG(&out_buf.data[2], out_buf.len - 4);
        TN5250_LOG(("<IAC><SE>\n"));
    }
    tn5250_buffer_free(&out_buf);
}

/****i* lib5250/telnet_stream_get_byte
 * NAME
 *    telnet_stream_get_byte
 * SYNOPSIS
 *    ret = telnet_stream_get_byte (This);
 * INPUTS
 *    Tn5250Stream *       This       -
 * DESCRIPTION
 *    Returns the next byte from the 5250 data stream, or return -1 if no data
 *    is waiting on the socket or -2 if disconnected, or -END_OF_RECORD if a
 *    telnet EOR escape sequence was encountered.
 *****/
static int telnet_stream_get_byte(Tn5250Stream* This) {
    int temp;
    unsigned char verb;

    do {
        if (This->state == TN5250_STREAM_STATE_NO_DATA) {
            This->state = TN5250_STREAM_STATE_DATA;
        }

        This->rcvbufpos++;
        if (This->rcvbufpos >= This->rcvbuflen) {
            This->rcvbufpos = 0;
            This->rcvbuflen =
                telnet_stream_get_next(This, This->rcvbuf, TN5250_RBSIZE);
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
                    ("GetByte: unknown escape 0x%02x in telnet stream.\n",
                     temp));
                This->state = TN5250_STREAM_STATE_NO_DATA; /* Hopefully a good
                                                              recovery. */
            }
            break;

        case TN5250_STREAM_STATE_HAVE_VERB:
            TN5250_LOG(("This->status  = %d\n", This->status));
            telnet_stream_do_verb(This, verb, (UCHAR)temp);
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
                telnet_stream_sb(This, tn5250_buffer_data(&(This->sb_buf)),
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

/****i* lib5250/telnet_stream_write
 * NAME
 *    telnet_stream_write
 * SYNOPSIS
 *    telnet_stream_write (This, data, size);
 * INPUTS
 *    Tn5250Stream *       This       -
 *    unsigned char *      data       -
 *    int                  size       -
 * DESCRIPTION
 *    Writes size bytes of data (pointed to by *data) to the 5250 data stream.
 *    This is also a temporary method to aid in the conversion process.
 *****/
static void telnet_stream_write(Tn5250Stream* This, unsigned char* data,
                                int size) {
    int r;
    int last_error = 0;
    fd_set fdw;

    /* There was an easier way to do this, but I can't remember.  This
       just makes sure that non blocking writes that don't have enough
       buffer space get completed anyway. */
    do {
        FD_ZERO(&fdw);
        FD_SET(This->sockfd, &fdw);
        r = select(This->sockfd + 1, NULL, &fdw, NULL, NULL);
        if (WAS_ERROR_RET(r)) {
            last_error = LAST_ERROR;
            switch (last_error) {
            case ERR_INTR:
            case ERR_AGAIN:
                r = 0;
                continue;

            default:
                perror("select");
                exit(5);
            }
        }
        if (FD_ISSET(This->sockfd, &fdw)) {
            r = send(This->sockfd, (char*)data, size, 0);
            if (WAS_ERROR_RET(r)) {
                last_error = LAST_ERROR;
                if (last_error != ERR_AGAIN) {
                    perror("Error writing to socket");
                    exit(5);
                }
            }
            if (r > 0) {
                data += r;
                size -= r;
            }
        }
    } while (size && (r >= 0 || last_error == ERR_AGAIN));
}

/****i* lib5250/telnet_stream_send_packet
 * NAME
 *    telnet_stream_send_packet
 * SYNOPSIS
 *    telnet_stream_send_packet (This, length, flowtype, flags, opcode, data);
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
static void telnet_stream_send_packet(Tn5250Stream* This, int length,
                                      StreamHeader header,
                                      unsigned char* data) {
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

    telnet_stream_escape(&out_buf);

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

    telnet_stream_write(This, tn5250_buffer_data(&out_buf),
                        tn5250_buffer_length(&out_buf));
    tn5250_buffer_free(&out_buf);
}

/****f* lib5250/telnet_stream_handle_receive
 * NAME
 *    telnet_stream_handle_receive
 * SYNOPSIS
 *    ret = telnet_stream_handle_receive (This);
 * INPUTS
 *    Tn5250Stream *       This       -
 * DESCRIPTION
 *    Read as much data as possible in a non-blocking fasion, form it
 *    into Tn5250Record structures and queue them for retrieval.
 *****/
int telnet_stream_handle_receive(Tn5250Stream* This) {
    int c;

    /* -1 = no more data, -2 = we've been disconnected */
    while ((c = telnet_stream_get_byte(This)) != -1 && c != -2) {

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

/****i* lib5250/telnet_stream_escape
 * NAME
 *    telnet_stream_escape
 * SYNOPSIS
 *    telnet_stream_escape (in);
 * INPUTS
 *    Tn5250Buffer *       in         -
 * DESCRIPTION
 *    Escape IACs in data before sending it to the host.
 *****/
static void telnet_stream_escape(Tn5250Buffer* in) {
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
