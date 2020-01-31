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
#include "cmd_handlers.h"
#include "utils.h"

// from https://beej.us/guide/bgnet/html/#a-simple-stream-server

#define PORT "14440" // the port users will be connecting to

#define BACKLOG 20 // how many pending connections queue will hold

struct Mfile *mfile;
char **midi_out_names;

/** Thread start routine for the main server thread */
void *main_server_thread(void *_arg);

/** Read from control pipe and do the action */
void handle_control(int control_fd);

/** Make tmp dir if it doesn't exist */
void make_tmp_dir();

/** Write out the midi files to the tmp folder */
void create_midi_files();

/** 
 * Count number of c in str
*/
int count_char(char *str, char c);
char **parse_line(char *line, int *len_ptr);

// name of temp dir for midi files
char *tmp_dir = NULL;

/* keeps track of the state of midi files */
int midi_ready = 0; // midi files are not generated until after all clients are connected
pthread_cond_t midi_ready_cond = PTHREAD_COND_INITIALIZER;
pthread_mutex_t midi_ready_cond_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_barrier_t midi_play_barrier;
int barrier_initialized = 0;
struct tlist *clients = NULL;
// int sockfd = -1;


int accepting_clients = 1; // whether server is accepting clients

int server_fd = -1; // for writing to server
pthread_t main_server;
char **args;
char *line;

struct cmd cmds[] =
    {
        {"load", 1, "load [*.mid] - load a midi file", handle_load},
        {"play", 0, "stop accepting new connections and start playing midi file; file has to be loaded", handle_play},
        {"pause", 0, "pause midi playback", handle_pause},
        {"resume", 0, "resume midi playback", handle_resume},
        // {"restart", 0, "restart midi playback with current clients", handle_restart},
        // seek is also no supported
        {"loop", 0, "loop midi playback", handle_loop},
        {"noloop", 0, "don't loop midi playback", handle_noloop},
        {"status", 0, "print current status of server", handle_status},
        {"quit", 0, "quit", handle_quit},
        {"reconnect", 0, "close all current clients and connect new ones", handle_reconnect},
        {"help", 0, "print help message", handle_help}};

const int cmds_len = sizeof(cmds) / sizeof(struct cmd);

int main(int argc, char *argv[])
{

    int sockfd = setup_server();

	// set up sockets

	socklen_t sin_size;
	char s[INET6_ADDRSTRLEN];			// hold address str
	int client_fd;						// new connection descriptor
	struct sockaddr_storage their_addr; // connector's address information
	fd_set readfds, readfds_copy;
	FD_ZERO(&readfds);
	FD_SET(STDIN_FILENO, &readfds);
	FD_SET(sockfd, &readfds);
	clients = new_tlist();
	printf("server: waiting for connections...\n");
	int tid = 0;		   // thread id


	// set up reading from stdin
    char *cmd;
    int nwords;
    int command_found = 0;

    while (1)
    {
        // not using readline
        fprintf(stdout, "> ");
        line = malloc(1000);
        if(!fgets(line, 1000, stdin))
        {
            perror("fgets");
        }
        line[strlen(line) - 1] = '\0'; // replace \n with \0

        // if (!(line = readline("> ")))
        // {
            // continue;
        // }
        if (strlen(line) == 0)
        {
            puts("");
            continue;
        }


        args = parse_line(line, &nwords);
        cmd = args[0];

        
        struct cmd *cur_cmd;
        for (int i = 0; i < cmds_len; i++)
        {
            cur_cmd = &cmds[i];
            if (strcmp(cmd, cur_cmd->name) == 0)
            {
                if (cur_cmd->num_args == 0)
                {
                    (cur_cmd->fn)();
                }
                else if (cur_cmd->num_args == 1)
                {
                    if (nwords < 2)
                    {
                        // only command is present; no argument supplies
                        fprintf(stderr, "%s: missing argument\n", cmd);
                    }
                    (cur_cmd->fn)(args[1]);
                }
                // there are no commands expecting 2 args

                // since command is executed, add to history and then stop comparing
                // add_history(line);
                command_found = 1;
                break;
            }
        }
        if (!command_found)
        {
            fprintf(stderr, "%s: command not found\n", cmd);
        }
        else
        {
            command_found = 0; // reset
        }
    }
    return 0;
}

void *main_server_thread(void *_arg)
{

	

	// /* keeps track of the state of midi files */
	// sig_atomic_t midi_ready = 0; // midi files are not generated until after all clients are connected
	// pthread_cond_t midi_ready_cond = PTHREAD_COND_INITIALIZER;
	// pthread_mutex_t midi_ready_cond_mutex = PTHREAD_MUTEX_INITIALIZER;
	// struct tlist * clients = new_tlist();


	while (1)
	{ // main accept() loop

		// select readline https://tiswww.case.edu/php/chet/readline/readline.html#SEC41
		readfds_copy = readfds;
		if (select(2, &readfds_copy, NULL, NULL, NULL) == -1)
		{
			perror("select");
			exit(4);
		}

		puts("selecting");
		if (FD_ISSET(STDIN_FILENO, &readfds_copy))
		{
			
			
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


//-------------------------------------------- HELPERS

char **parse_line(char *line, int *len_ptr)
{
    // malloc the size of the number of words, separated by ' '
    *len_ptr = count_char(line, ' ') + 1;
    char **args = malloc(sizeof(char *) * (*len_ptr));

    if (args == NULL)
    {
        perror("malloc");
        exit(1);
    }
    int i = 0;
    char *sep = " ";
    args[i++] = strtok(line, sep);
    char *token = strtok(NULL, sep);
    while (token != NULL)
    {
        args[i++] = token;
        token = strtok(NULL, sep);
    }
    return args;
}

int count_char(char *str, char c)
{
    int i, sum, len;
    for (i = 0, sum = 0, len = strlen(str); i < len; i++)
    {
        if (str[i] == c)
        {
            sum++;
        }
    }
    return sum;
}

static void cleanup()
{
    free(line);
    free(args);
    line = NULL;
    args = NULL;
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



// modifies global
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

// modifies global
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
