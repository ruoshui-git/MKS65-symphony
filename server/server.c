#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>

#include <sys/time.h>
#include <unistd.h>

#include <pthread.h>

#include "server_thread.h"
#include "server_local_protocol.h"
#include "server.h"

// from https://beej.us/guide/bgnet/html/#a-simple-stream-server

#define PORT "14440" // the port users will be connecting to

#define BACKLOG 20 // how many pending connections queue will hold

struct server_arg
{
    int sockfd;
    int control_fd;
};

/** Thread start routine for the main server thread */
void * main_server_thread(void * _arg);

/** Read from control pipe and do the according action */
void handle_control(int control_fd);

/* keeps track of the state of midi files */
sig_atomic_t midi_ready = 0; // midi files are not generated until after all clients are connected
pthread_cond_t midi_ready_cond = PTHREAD_COND_INITIALIZER;
pthread_mutex_t midi_ready_cond_mutex = PTHREAD_MUTEX_INITIALIZER;
struct tlist *clients = NULL;
// int sockfd = -1;

// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
	if (sa->sa_family == AF_INET)
	{
		return &(((struct sockaddr_in *)sa)->sin_addr);
	}

	return &(((struct sockaddr_in6 *)sa)->sin6_addr);
}
// get port, IPv4 or IPv6:
in_port_t get_in_port(struct sockaddr *sa)
{
	if (sa->sa_family == AF_INET)
	{
		return (((struct sockaddr_in *)sa)->sin_port);
	}

	return (((struct sockaddr_in6 *)sa)->sin6_port);
}

int setup_server(void)
{
	int sockfd;  // listen on sock_fd
	struct addrinfo hints, *servinfo, *p;
	// struct sigaction sa; // not using fork()
	int yes = 1;
	int rv;

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE; // use my IP

	if ((rv = getaddrinfo(NULL, PORT, &hints, &servinfo)) != 0)
	{
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
		return 1;
	}

	// loop through all the results and bind to the first we can
	for (p = servinfo; p != NULL; p = p->ai_next)
	{
		if ((sockfd = socket(p->ai_family, p->ai_socktype,
							 p->ai_protocol)) == -1)
		{
			perror("server: socket");
			continue;
		}

		if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes,
					   sizeof(int)) == -1)
		{
			perror("setsockopt");
			exit(1);
		}

		if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1)
		{
			close(sockfd);
			perror("server: bind");
			continue;
		}

		// inet_ntop(p->ai_family, get_in_addr((struct sockaddr *)p->ai_addr),s, sizeof s);
		fprintf(stdout, "Listening on port %d\n", ntohs(get_in_port((struct sockaddr *)p->ai_addr)));

		break;
	}

	freeaddrinfo(servinfo); // all done with this structure

	if (p == NULL)
	{
		fprintf(stderr, "server: failed to bind\n");
		exit(1);
	}

	if (listen(sockfd, BACKLOG) == -1)
	{
		perror("listen");
		exit(1);
	}

	return sockfd;
}

pthread_t run_server(int sockfd, int control_fd)
{
	struct server_arg sarg;
	sarg.sockfd = sockfd;
	sarg.control_fd = control_fd;

	pthread_t thread;
	if (pthread_create(&thread, NULL, main_server_thread, &sarg))
	{
		perror("pthread_create");
		exit(1);
	}
	return thread;
}

void * main_server_thread(void * _arg)
{
	struct server_arg arg = *((struct server_arg *) _arg);
	int control_fd = arg.control_fd;
	int sockfd = arg.sockfd;

	// int control_fd; // will be from stdin
	socklen_t sin_size;
	char s[INET6_ADDRSTRLEN];			// hold address str
	int client_fd;						// new connection descriptor
	struct sockaddr_storage their_addr; // connector's address information
	fd_set readfds, readfds_copy;
	FD_ZERO(&readfds);
	FD_SET(control_fd, &readfds);
	FD_SET(sockfd, &readfds);

	// /* keeps track of the state of midi files */
	// sig_atomic_t midi_ready = 0; // midi files are not generated until after all clients are connected
	// pthread_cond_t midi_ready_cond = PTHREAD_COND_INITIALIZER;
	// pthread_mutex_t midi_ready_cond_mutex = PTHREAD_MUTEX_INITIALIZER;
	// struct tlist * clients = new_tlist();

	clients = new_tlist();
	printf("server: waiting for connections...\n");

	int tid = 0; // thread id
	while (1)
	{ // main accept() loop
		readfds_copy = readfds;
		if (select(2, &readfds_copy, NULL, NULL, NULL) == -1)
		{
			perror("select");
			exit(4);
		}

		if (FD_ISSET(control_fd, &readfds_copy))
		{
			// handle communication with main thread
			handle_control(control_fd);
		}
		else
		{
			// we got a new connection, accept itsin_size = sizeof their_addr;
			client_fd = accept(sockfd, (struct sockaddr *)&their_addr, &sin_size);
			if (client_fd == -1)
			{
				perror("accept");
				continue;
			}

			inet_ntop(their_addr.ss_family,
					  get_in_addr((struct sockaddr *)&their_addr),
					  s, sizeof s);

			fprintf(stdout, "server: got connection from %s\n", s);

			struct s_thread_arg *arg = malloc(sizeof(struct s_thread_arg));
			arg->socket = client_fd;
			arg->tid = tid;
			arg->midi_ready = &midi_ready;
			arg->midi_ready_cond = &midi_ready_cond;
			arg->midi_ready_cond_mutex = &midi_ready_cond_mutex;

			pthread_t thread = new_server_thread(arg);
			struct tnode *node = new_tnode(thread, arg);
			append_tnode(clients, node);

			printf("num threads: %d\n", clients->len);

			tid++;
		}
	}
}