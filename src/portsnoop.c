/*
	portsnoop.c - Quick hack so we can inspect the data stream as a man in the middle.

	Run us from inetd.
*/

#include "config.h"

#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <netdb.h>
#ifdef HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif
#include <arpa/inet.h>
#include <unistd.h>

#include "utility.h"

int				sockfd;
struct sockaddr_in		destaddr;
fd_set				fdr;
unsigned char			buf[1024];
int				buf_len;
u_long				arg;
FILE*				logfile;

void log(char d, unsigned char *buf, int len);

int main(int argc, char **argv)
{
	if(argc != 4)
	{
		fprintf(stderr,"portsnoop: Syntax is `portsnoop destip destport logfile'.\n");
		exit(255);
	}
	if((logfile = fopen(argv[3],"w")) == NULL)
	{
		perror("fopen");
		exit(255);
	}
	close(2);
	dup2(fileno(logfile),2);
	if((destaddr.sin_addr.s_addr = inet_addr(argv[1])) == INADDR_NONE)
	{
		perror("inet_addr");
		exit(255);
	}
	if((destaddr.sin_port = htons(atoi(argv[2]))) == 0)
	{
		fprintf(stderr,"portsnoop: Invalid port.\n");
		exit(255);
	}
	destaddr.sin_family = AF_INET;

	if((sockfd = socket(PF_INET, SOCK_STREAM, 0)) == -1)
	{
		perror("socket");
		exit(255);
	}
	if(connect(sockfd, (struct sockaddr*)&destaddr, sizeof (struct sockaddr_in)) == -1)
	{
		perror("connect");
		exit(255);
	}
	arg = 1;
	if(ioctl(sockfd, FIONBIO, &arg) == -1)
	{
		perror("ioctl");
		exit(255);
	}
	arg = 1;
	if(ioctl(0, FIONBIO, &arg) == -1)
	{
		perror("ioctl");
		exit(255);
	}

	tn5250_settransmap("en");

	while(1)
	{
		FD_ZERO(&fdr);
		FD_SET(sockfd, &fdr);
		FD_SET(0, &fdr);
		if(select (sockfd+1, &fdr, NULL, NULL, NULL) == -1)
		{
			continue;
		}
		if(FD_ISSET(sockfd, &fdr))
		{
			buf_len = recv(sockfd, buf, sizeof(buf), 0);
			if(buf_len == -1)
			{
				perror("recv");
				exit(255);
			}
			log('<', buf, buf_len);
			if (write(1,buf,buf_len) != buf_len)
			{
				perror("write");
				exit(255);
			}
		}
		if(FD_ISSET(0, &fdr))
		{
			buf_len = read(0, buf, sizeof(buf));
			if(buf_len == -1)
			{
				perror("read");
				exit(255);
			}
			log('>', buf, buf_len);
			if(send(sockfd, buf, buf_len, 0) != buf_len)
			{
				perror("send");
				exit(255);
			}
		}
	}
}

void log(char d, unsigned char *buf, int len)
{
	int x;
	char hr[17];

	hr[16] = 0;
	if(len == 0) {
		fprintf(logfile,"%c closed connection.\n",d);
		exit(0);
	}
	while(len>0)
	{
		fprintf(logfile,"%c ",d);
		for(x=0; x<16; x++)
		{
			if(x<len)
			{
				hr[x] = tn5250_ebcdic2ascii(buf[x]);
				if(!isprint(hr[x]) || iscntrl(hr[x]))
					hr[x] = '.';
				fprintf(logfile,"%02X ", (unsigned)buf[x]);
			}
			else
			{
				hr[x] = 0;
				fprintf(logfile,"   ");
			}
		}
		fprintf(logfile,"  %s\n",hr);

		len -= 16;
		buf += 16;
	}
}
