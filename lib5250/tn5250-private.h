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

#if defined(WIN32) || defined(WINE)
#include <windows.h>
#include <winsock.h>
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

#ifdef HAVE_SYSLOG_H
#include <syslog.h>
#endif

#ifdef HAVE_PWD_H
#include <pwd.h>
#endif

#include "buffer.h"
#include "record.h"
#include "stream.h"
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

extern char *version_string;

#if !defined(WINE) && !defined(WIN32)
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

/** Start of REALLY ugly network portability layer. **/
#if defined(WINE)
#define TN_SOCKET		WINSOCK_socket
#define TN_CONNECT		WINSOCK_connect
#define TN_SELECT		WINSOCK_select
#define TN_GETHOSTBYNAME	WINSOCK_gethostbyname
#define TN_GETSERVBYNAME	WINSOCK_getservbyname
#define TN_SEND			WINSOCK_send
#define TN_RECV			WINSOCK_recv
#define TN_CLOSE		WINSOCK_closesocket
#define TN_IOCTL		WINSOCK_ioctlsocket

/* Prototypes needed by WINE's winsock implementation so that the
 * names don't clash with system functions. */
struct hostent *WINAPI WINSOCK_gethostbyname(const char *name);
INT WINAPI WINSOCK_socket(INT af, INT type, INT protocol);
INT WINAPI WINSOCK_connect(SOCKET s, struct sockaddr *name, INT namelen);
INT WINAPI WINSOCK_recv(SOCKET s, char *buf, INT len, INT flags);
INT WINAPI WINSOCK_send(SOCKET s, char *buf, INT len, INT flags);
void WINAPI WINSOCK_closesocket(SOCKET s);
INT WINAPI WINSOCK_select(INT nfds, ws_fd_set32 * ws_readfds,
		   ws_fd_set32 * ws_writefds, ws_fd_set32 * ws_exceptfds,
			  struct timeval *timeout);

#define fd_set ws_fd_set32
#undef FD_ZERO
#undef FD_SET
#undef FD_ISSET
#define FD_ZERO(x) WS_FD_ZERO((x))
#define FD_SET(x,y) WS_FD_SET((x),(y))
#define FD_ISSET(x,y) WS_FD_ISSET((x),(y))
/* end WINE */

#elif defined(WIN32)
#define TN_SOCKET		socket
#define TN_CONNECT		connect
#define TN_SELECT		select
#define TN_GETHOSTBYNAME	gethostbyname
#define TN_GETSERVBYNAME	getservbyname
#define TN_SEND			send
#define TN_RECV		        recv
#define TN_CLOSE		closesocket
#define TN_IOCTL		ioctlsocket
/* end WIN32 */

#else
#define TN_SOCKET		socket
#define TN_CONNECT		connect
#define TN_SELECT		select
#define TN_GETHOSTBYNAME	gethostbyname
#define TN_GETSERVBYNAME	getservbyname
#define TN_SEND			send
#define TN_RECV		        recv
#define TN_CLOSE		close
#define TN_IOCTL		ioctl

#endif

#if defined(WINE) || defined(WIN32)
/* Windows' braindead socketing */
#define LAST_ERROR		(WSAGetLastError ())
#define ERR_INTR		WSAEINTR
#define ERR_AGAIN 		WSAEWOULDBLOCK
#define WAS_ERROR_RET(r)	((r) == SOCKET_ERROR)
#define WAS_INVAL_SOCK(r)	((r) == INVALID_SOCKET)
#else
/* Real, UNIX socketing */
#define LAST_ERROR		(errno)
#define ERR_INTR		EINTR
#define ERR_AGAIN		EAGAIN
#define WAS_ERROR_RET(r)	((r) < 0)
#define WAS_INVAL_SOCK(r)	((r) < 0)
#endif

#ifndef INADDR_NONE
#define INADDR_NONE 0xffffffff
#endif

/* END: of really ugly network portability layer. */

#endif				/* PRIVATE_H */

/* vi:set cindent sts=3 sw=3: */
