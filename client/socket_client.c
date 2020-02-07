
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>


#include "const.h"
#include "socket_client.h"
#include "../client_protocol.h"

int connect_to(char * server_addr)
{
	struct addrinfo hints, *servinfo, *p;
	int rv, sockfd;
	char s[INET6_ADDRSTRLEN];

	

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;

	if ((rv = getaddrinfo(/* argv[1] */ server_addr, PORT, &hints, &servinfo)) != 0)
	{
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
		return -1;
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
		return -1;
	}

	inet_ntop(p->ai_family, get_in_addr((struct sockaddr *)p->ai_addr),
			  s, sizeof s);
	printf("client: connecting to %s\n", s);

	freeaddrinfo(servinfo); // all done with this structure

	return sockfd;
}

int confirm_connection(int sockfd)
{
	char buf[10];

	int numbytes;

	if ((numbytes = recv(sockfd, buf, 10, 0)) == -1)
	{
		perror("recv");
		return -1;
	}

	buf[numbytes - 1] = '\0';

	printf("client: received '%s'\n, numbytes: %d\n", buf, numbytes);

	uint32_t id;
	// if (recv(sockfd, &id, 4, 0) == -1)
	if (read(sockfd, &id, 4) == -1)
	{
		perror("recv");
		return -1;
	}

	puts("Connection confirmed");

	id = ntohl(id);

	printf("id: %u\n", id);
	return 0;
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

// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
	if (sa->sa_family == AF_INET)
	{
		return &(((struct sockaddr_in *)sa)->sin_addr);
	}

	return &(((struct sockaddr_in6 *)sa)->sin6_addr);
}