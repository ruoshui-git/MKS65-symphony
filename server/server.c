#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <netdb.h>
#include <signal.h>
#include <assert.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <sys/stat.h>

#include <pthread.h>

#include "server_thread.h"
#include "server.h"
#include "midi_reader.h"
#include "midi_splitter.h"
#include "midi_writer.h"
#include "midi_player.h"
#include "utils.h"

// from https://beej.us/guide/bgnet/html/#a-simple-stream-server

#define PORT "14440" // the port users will be connecting to

#define BACKLOG 20 // how many pending connections queue will hold

struct Mfile *mfile;
char **midi_out_names;

struct server_arg
{
	int sockfd;
	int control_fd;
};

/** Thread start routine for the main server thread */
void *main_server_thread(void *_arg);

/** Read from control pipe and do the according action */
void handle_control(int control_fd);

/** Make tmp dir if it doesn't exist */
void make_tmp_dir();

/** Write out the midi files to the tmp folder */
void create_midi_files();


/* keeps track of the state of midi files */
sig_atomic_t midi_ready = 0; // midi files are not generated until after all clients are connected
pthread_cond_t midi_ready_cond = PTHREAD_COND_INITIALIZER;
pthread_mutex_t midi_ready_cond_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_barrier_t midi_play_barrier;
int barrier_initialized = 0;
struct tlist *clients = NULL;
// int sockfd = -1;

// name of temp dir for midi files
char *tmp_dir = NULL;

int accepting_clients = 1; // whether server is accepting clients

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
	int sockfd; // listen on sock_fd
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

void *main_server_thread(void *_arg)
{
	struct server_arg arg = *((struct server_arg *)_arg);
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

	int tid = 0;		   // thread id
	struct server_ctl ctl; // command from main thread
	while (1)
	{ // main accept() loop
		readfds_copy = readfds;
		if (select(2, &readfds_copy, NULL, NULL, NULL) == -1)
		{
			perror("select");
			exit(4);
		}

		puts("selecting");
		if (FD_ISSET(control_fd, &readfds_copy))
		{
			// handle communication with main thread
			// handle_control(control_fd);
			read(control_fd, &ctl, sizeof(ctl));

			printf("control: %d, val: %d\n", ctl.control, ctl.value);

			if (ctl.control == SERVER_START_PLAYER)
			{
				create_midi_files();

				// send files to clients
				pthread_mutex_lock(&midi_ready_cond_mutex);
				midi_ready = 1;

				if (!barrier_initialized)
				{
					pthread_barrier_init(&midi_play_barrier, 0, clients->len + 1);
					barrier_initialized = 1;
				}

				pthread_cond_broadcast(&midi_ready_cond);
				pthread_mutex_unlock(&midi_ready_cond_mutex);
				
				
				player_setup();
				player_add_midi_file(mfile->fullpath);
				pthread_barrier_wait(&midi_play_barrier);
				// wait for some time;
				player_play();

			}
			else if (ctl.control == SERVER_SET_PLAYER)
			{
				
			}
			else if (ctl.control == SERVER_SEEK_PLAYER)
			{
				sys_warning("Cannot seek due to library limitations");
				continue;
			}
			else if (ctl.control == SERVER_PRINT_STATUS)
			{
				printf("Num clients: %d\n", clients->len);
				
			}
			else if (ctl.control == SERVER_RECONNECT)
			{
				
			}
			else if (ctl.control == SERVER_QUIT)
			{
				struct tnode * cur = clients->first;
				while (cur)
				{
					pthread_cancel(cur->thread);
					cur = cur->next;
				}
				free(clients);
				return 0;
			}
			else
			{

				sys_error("Unexpected internal command");
				// clean up
				exit(1);
			}
			
			
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

			// if not accepting clients, close the connection immediately
			if (!accepting_clients)
			{
				close(client_fd);
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
			arg->midi_play_barrier = &midi_play_barrier;

			pthread_t thread = new_server_thread(arg);
			struct tnode *node = new_tnode(thread, arg);
			append_tnode(clients, node);

			printf("num threads: %d\n", clients->len);

			tid++;
		}
	}
}

void make_tmp_dir(void)
{
	if (!tmp_dir)
	{
		char *wd = getcwd(NULL, 0);
		tmp_dir = malloc(strlen(wd) + 4); // wd + "tmp/"
		strcpy(tmp_dir, wd);
		strcat(tmp_dir, "tmp/");
		free(wd);
	}

	// check if tmp_dir already exist
	struct stat s;
	if (stat(tmp_dir, &s) == -1)
	{
		if (errno = ENOENT)
		{
			// tmp dir doesn't exist
			mkdir(tmp_dir, 641);
		}
		else
		{
			perror("stat");
			// clean up
			exit(1);
		}
	}
	else
	{
		if (!S_ISDIR(s.st_mode))
		{
			sys_warning("Please remove the file with name 'tmp' since a dir with this name needs to be created");
			// clean up
			exit(1);
		}
	}
}

void create_midi_files()
{
	accepting_clients = 0;
	puts("server: no longer accepting connections");

	midi_ready = 0; // (re)set midi_file conditions
	// set up files
	assert(mfile != NULL);
	int nclients = clients->len;
	struct Mfile **midi_out = Mfile_split_by_tracks(mfile, nclients);

	make_tmp_dir();

	// write out files to tmp_dir
	midi_out_names = malloc(sizeof(char *) * nclients);
	for (int i = 0; i < nclients; i++)
	{
		char fname[strlen(MIDI_OUT_FORMAT) + 2];
		sprintf(fname, MIDI_OUT_FORMAT, i);
		char *absfname = malloc(strlen(tmp_dir) + strlen(fname) + 1); // + 1 for null
		strcpy(absfname, tmp_dir);
		strcat(absfname, fname);
		Mfile_write_to_midi(midi_out[i], absfname);
		midi_out_names[i] = absfname;
	}
}
