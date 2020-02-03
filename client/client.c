/*
** client.c -- a stream socket client demo
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>

#include <arpa/inet.h>

#include "midi_player.h"
#include "../client_protocol.h"

#define PORT "14440" // the port client will be connecting to

#define SERVER_ADDR "localhost"

#define MAXDATASIZE 100 // max number of bytes we can get at once

int recvall(int sockfd, void *buf, unsigned int *len);

// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
	if (sa->sa_family == AF_INET)
	{
		return &(((struct sockaddr_in *)sa)->sin_addr);
	}

	return &(((struct sockaddr_in6 *)sa)->sin6_addr);
}

int main(int argc, char *argv[])
{
	int sockfd, numbytes;
	char buf[MAXDATASIZE];
	struct addrinfo hints, *servinfo, *p;
	int rv;
	char s[INET6_ADDRSTRLEN];

	// if (argc != 2) {
	//     // fprintf(stderr,"usage: client hostname\n");
	//     // exit(1);
	// }

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;

	if ((rv = getaddrinfo(/* argv[1] */ SERVER_ADDR, PORT, &hints, &servinfo)) != 0)
	{
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
		return 1;
	}

	// loop through all the results and connect to the first we can
	for (p = servinfo; p != NULL; p = p->ai_next)
	{
		if ((sockfd = socket(p->ai_family, p->ai_socktype,
							 p->ai_protocol)) == -1)
		{
			perror("client: socket");
			continue;
		}

		if (connect(sockfd, p->ai_addr, p->ai_addrlen) == -1)
		{
			perror("client: connect");
			close(sockfd);
			continue;
		}

		break;
	}

	if (p == NULL)
	{
		fprintf(stderr, "client: failed to connect\n");
		return 2;
	}

	inet_ntop(p->ai_family, get_in_addr((struct sockaddr *)p->ai_addr),
			  s, sizeof s);
	printf("client: connecting to %s\n", s);

	freeaddrinfo(servinfo); // all done with this structure

	if ((numbytes = recv(sockfd, buf, 10, 0)) == -1)
	{
		perror("recv");
		goto cleanup;
	}

	buf[numbytes] = '\0';

	printf("client: received '%s'\n, numbytes: %d\n", buf, numbytes);

	u_int32_t id;
	// if (recv(sockfd, &id, 4, 0) == -1)
	if (read(sockfd, &id, 4) == -1)
	{
		perror("recv");
		goto cleanup;
	}

	puts("data received");

	id = ntohl(id);

	printf("id: %u\n", id);

	puts("waiting for server response");

new_file:;
	uint32_t fsize = 0;
	if (read(sockfd, &fsize, 4) == -1)
	{
		perror("read");
		goto cleanup;
	}

	printf("size of midi file: %uld\n", fsize);
	puts("waiting for file...");

	char *fbuff = malloc(fsize);
	uint32_t recvsize = fsize;
	if (recvall(sockfd, fbuff, &fsize) == -1)
	{
		goto cleanup;
	}

	// now we got file, set up player
	player_setup();
	player_add_midi_mem(fbuff, fsize);

	// signal ready

	unsigned char sbuf = 1;

	if (send(sockfd, &sbuf, 1, 0) == -1)
	{
		perror("send");
		goto cleanup;
	}

	// read instruction from server and exec
	while (1)
	{
		read(sockfd, &sbuf, 1);
		switch (sbuf)
		{
		case CLIENT_PLAY:
			player_play();
			break;
		case CLIENT_PAUSE:
			player_pause();
			break;
		case CLIENT_RESUME:
			player_play();
			break;
		case CLIENT_SEEK: // followed by another 4 byte with a tick value
			puts("Cannot seek due to library limitation");
			break;
			break;
		case CLIENT_LOOP:
			player_setloop(1);
			break;
		case CLIENT_NOLOOP:
			player_setloop(0);
			break;
		case CLIENT_NFILE: // new file
			free(fbuff);
			goto new_file;
			break;
		default:
			puts("Unrecognized command");
		}
	}

	close(sockfd);

	return 0;

cleanup:
	close(sockfd);
	exit(1);
}

int recvall(int sockfd, void *buf, unsigned int *len)
{
	int total = 0;		  // how many bytes we've received
	int bytesleft = *len; // how many we have left to receive
	int n;

	while (total < *len)
	{
		n = recv(sockfd, buf + total, bytesleft, 0);
		if (n == -1)
		{
			break;
		}
		total += n;
		bytesleft -= n;
		printf("bytes received: %d, bytesleft: %d\n", n, bytesleft);
	}

	*len = total; // return number actually sent here

	if (n == -1)
	{
		perror("recv");
	}

	return n == -1 ? -1 : 0; // return -1 on failure, 0 on success
}