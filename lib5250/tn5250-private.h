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
#ifndef PRIVATE_H
#define PRIVATE_H

/*#include "tn5250-autoconfig.h"*/
#include "config.h"

#if defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#endif

#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <signal.h>

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#ifdef HAVE_LOCALE_H
#include <locale.h>
#endif

#ifdef HAVE_SYS_TIME_H
#include <sys/time.h>
#endif

#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif

#ifdef HAVE_SYS_WAIT_H
#include <sys/wait.h>
#endif

#ifdef HAVE_FCNTL_H
#include <fcntl.h>
#endif

#ifdef HAVE_PWD_H
#include <pwd.h>
#endif

#include "buffer.h"
#include "record.h"
#include "stream-private.h"
#include "utility.h"
#include "dbuffer.h"
#include "field.h"
#include "codes5250.h"
#include "scrollbar.h"
#include "session.h"
#include "printsession.h"
#include "display.h"
#include "macro.h"
#include "menu.h"
#include "wtd.h"
#include "window.h"
#include "terminal.h"
#include "debug.h"
#include "scs.h"
#include "conf.h"

extern char* version_string;

#if !defined(_WIN32)
#include <sys/time.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <netdb.h>
#include <unistd.h>
#endif

#ifdef HAVE_SYS_FILIO_H
#include <sys/filio.h>
#endif

typedef enum {
    TN5250_ERROR_UNKNOWN,
    TN5250_ERROR_INTERNAL,
    TN5250_ERROR_ERRNO,
    TN5250_ERROR_GAI,
    TN5250_ERROR_SSL,
} Tn5250ErrorType;

typedef enum {
    TN5250_INTERNALERROR_UNKNOWN,
    TN5250_INTERNALERROR_INVALIDADDRESS,
    TN5250_INTERNALERROR_INVALIDCERT,
} Tn5250InternalErrorCode;

void _tn5250_set_error(Tn5250ErrorType type, int code);

/** Start of REALLY ugly network portability layer. **/

#if defined(_WIN32)
#define TN_CLOSE closesocket
#define TN_IOCTL ioctlsocket
/* end _WIN32 */

#else
#define TN_CLOSE close
#define TN_IOCTL ioctl

#endif

#if defined(_WIN32)
/* Windows' braindead socketing */
#define LAST_ERROR        (WSAGetLastError())
#define ERR_INTR          WSAEINTR
#define ERR_AGAIN         WSAEWOULDBLOCK
#define WAS_ERROR_RET(r)  ((r) == SOCKET_ERROR)
#define WAS_INVAL_SOCK(r) ((r) == INVALID_SOCKET)
#else
/* Real, UNIX socketing */
#define LAST_ERROR        (errno)
#define ERR_INTR          EINTR
#define ERR_AGAIN         EAGAIN
#define WAS_ERROR_RET(r)  ((r) < 0)
#define WAS_INVAL_SOCK(r) ((r) < 0)
#endif

#ifndef INADDR_NONE
#define INADDR_NONE 0xffffffff
#endif

/* END: of really ugly network portability layer. */

#endif /* PRIVATE_H */
