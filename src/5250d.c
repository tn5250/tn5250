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
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>

int 
make_socket (unsigned short int port)
{
  int sock;
  int on = 1;
  struct sockaddr_in name;

  /* Create the socket. */
  sock = socket (PF_INET, SOCK_STREAM, 0);
  if (sock < 0)
    {
      perror ("socket");
      exit (EXIT_FAILURE);
    }

  /* Give the socket a name. */
  name.sin_family = AF_INET;
  name.sin_port = htons (port);
  name.sin_addr.s_addr = htonl (INADDR_ANY);
  setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));
  if (bind (sock, (struct sockaddr *) &name, sizeof (name)) < 0)
    {
      perror ("bind");
      exit (EXIT_FAILURE);
    }

  return sock;
}

void
process_client(int sockfd)
{
  Tn5250Stream * hoststream;
  Tn5250Host * host;
  int aidkey;

  hoststream = tn5250_stream_host(sockfd, 1000);

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
int sockfd;
int snsize;
struct sockaddr_in sn = { AF_INET };
fd_set active_fd_set, read_fd_set;
int i;
struct sockaddr_in clientname;
size_t size;

#define PORT 2023

int
main(void)
{
  printf("Starting server...\n");

  tn5250_daemon(0,0,1);

  sock = make_socket (PORT);

  /* Create the socket and set it up to accept connections. */
  if (listen (sock, 1) < 0)
    {
      perror ("listen");
      exit (EXIT_FAILURE);
    }
  
  while(1) 
    {
      snsize = sizeof(sn);
      sockfd = accept(sock, (struct sockaddr *)&sn, &snsize);

      if(sockfd < 0) {
	perror("accept");
	exit(1);
      }

      printf("Incoming connection...\n");
      
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
