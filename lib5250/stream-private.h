/* TN5250 - An implementation of the 5250 telnet protocol.
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
#ifndef STREAM_PRIVATE_H
#define STREAM_PRIVATE_H

#include "stream.h"

#ifdef HAVE_LIBSSL
#include <openssl/ssl.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

#define TN5250_RBSIZE 8192
struct _Tn5250Stream {
    int (*connect)(struct _Tn5250Stream* This, const char* to);
    int (*accept)(struct _Tn5250Stream* This, SOCKET_TYPE masterSock);
    void (*disconnect)(struct _Tn5250Stream* This);
    int (*handle_receive)(struct _Tn5250Stream* This);
    void (*send_packet)(struct _Tn5250Stream* This, int length,
                        StreamHeader header, unsigned char* data);
    void(/*@null@*/ *destroy)(struct _Tn5250Stream /*@only@*/* This);

    struct _Tn5250Config* config;

    Tn5250Record /*@null@*/* records;
    Tn5250Record /*@dependent@*/ /*@null@*/* current_record;
    int record_count;

    Tn5250Buffer sb_buf;

    SOCKET_TYPE sockfd;
    int status;
    int state;
    int streamtype;
    long msec_wait;
    unsigned char options;

    unsigned char rcvbuf[TN5250_RBSIZE];
    int rcvbufpos;
    int rcvbuflen;

#ifdef HAVE_LIBSSL
    SSL* ssl_handle;
    SSL_CTX* ssl_context;
    void* userdata;
#endif

#ifndef NDEBUG
    FILE* debugfile;
#endif
};

#ifdef __cplusplus
}
#endif

#endif
