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
 * If you do not wish that, delete this exception notice. 
 */

#include "tn5250-private.h"
#include "host5250.h"
#include "tn5250d.h"
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>

void
process_client(int sockfd)
{
  Tn5250Stream * hoststream;
  Tn5250Host * host;
  int aidkey;

  hoststream = tn5250_stream_host(sockfd, 0);

  if(hoststream != NULL) {
    host = tn5250_host_new(hoststream);
    printf("Successful connection\n");
    aidkey = SendTestScreen(host);
    tn5250_host_destroy(host);
    printf("aidkey = %d\n", aidkey);
    _exit(0);
  } else {
    printf("Connection failed.\n");
    _exit(1);
  }
}

int childpid;
int sock;
int mgr_sock;
int sockfd;
int snsize;
struct sockaddr_in sn = { AF_INET };
int i;
struct sockaddr_in clientname;
size_t size;
fd_set rset;

int
main(void)
{
  int rc;

  printf("Starting tn5250d server...\n");
  
  tn5250_daemon(0,0,1);

  sock = tn5250_make_socket (CLIENT_PORT);
  mgr_sock = tn5250_make_socket(MANAGE_PORT);

  /* Create the client socket and set it up to accept connections. */
  if (listen (sock, 1) < 0)
    {
      syslog(LOG_INFO, "listen (CLIENT_PORT): %s\n", strerror(errno));
      exit (EXIT_FAILURE);
    }

  /* Create the manager socket and set it up to accept connections. */
  if (listen (mgr_sock, 1) < 0)
    {
      syslog(LOG_INFO, "listen (MANAGER_PORT): %s\n", strerror(errno));
      exit(EXIT_FAILURE);
    }

  syslog(LOG_INFO, "tn5250d server started\n");
  
  while(1) 
    {
      FD_ZERO(&rset);
      FD_SET(sock,&rset);

      if( select(sock+1, &rset, NULL, NULL, NULL) < 0 ) {
	if( errno == EINTR ) {
	  continue;
	} else {
	  syslog(LOG_INFO, "select: %s\n", strerror(errno));
	  exit(1);
	}
      }

      snsize = sizeof(sn);
      
      sockfd = accept(sock, (struct sockaddr *)&sn, &snsize);

      if(sockfd < 0) {
	switch(errno) {
	case EINTR:
	case EWOULDBLOCK:
	case ECONNABORTED:
	  continue;
	default:
	  syslog(LOG_INFO, "accept: %s\n", strerror(errno));
	  exit(1);
	}
      } 

      syslog(LOG_INFO, "Incoming connection...\n");
      
      if( (childpid = fork()) < 0) {
	perror("fork");
      } else if(childpid > 0) {
	close(sockfd);

      } else {
	close(sock);
	process_client(sockfd);
      }

    }
  
  return(0);
}
