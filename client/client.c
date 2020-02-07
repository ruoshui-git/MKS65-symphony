/*
** client.c -- a stream socket client demo
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/types.h>

#include "midi_player.h"
#include "../client_protocol.h"
#include "utils.h"
#include "const.h"
#include "socket_client.h"
#include "client.h"


// #define MAXDATASIZE 100 // max number of bytes we can get at once



static void cleanup();
static void client_exit();


int sockfd = -1; // server fd



int main(int argc, char *argv[])
{

	if (argc > 2) {
	    fprintf(stderr,"usage: client [hostname (default 'localhost')]\n");
	    exit(1);
	}

	char * server_addr = SERVER_ADDR;
	if (argc == 2)
	{
		server_addr = argv[1];
	}

	if ((sockfd = connect_to(server_addr)) == -1)
	{
		exit(EXIT_FAILURE);
	}
	printf("server: %d\n", sockfd);

	if (confirm_connection(sockfd) == -1)
	{
		client_exit();
	}

	// midi file
	char * fbuff = NULL;
	// midi file size
	uint32_t fsize = 0;

	puts("waiting for server response");

	int running = 1;

	unsigned char sbuf = 0;

	// read instruction from server and exec
	while (running)
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
			;
			uint32_t tick;
			if (read(sockfd, &tick, sizeof tick) == -1)
			{
				perror("read");
				client_exit();
			}
			player_seek(ntohl(tick));
			break;
		case CLIENT_LOOP:
			player_setloop(1);
			break;
		case CLIENT_NOLOOP:
			player_setloop(0);
			break;
		case CLIENT_NFILE: // new file
			if (fbuff)
				free(fbuff);
				
			if (!(fbuff = get_file(sockfd, &fsize)))
			{
				client_exit();
			}
			// now we got file, set up player
			player_setup();
			player_add_midi_mem(fbuff, fsize);

			// signal ready
			unsigned char sbuf = 1;

			if (send(sockfd, &sbuf, 1, 0) == -1)
			{
				perror("send");
				client_exit();
			}
			puts("server should be ready");
			break;

		case CLIENT_QUIT:
			running = 0;
			break;
		default:
			puts("Unrecognized command");
		}
	}

	if (fbuff)
		free(fbuff);
	

	close(sockfd);

	return 0;
}


static void cleanup()
{
	close(sockfd);
}

static void client_exit()
{
	cleanup();
	exit(EXIT_FAILURE);
}

char * get_file(int sockfd, uint32_t * size)
{

	uint32_t fsize = 0;
	if (read(sockfd, &fsize, 4) == -1)
	{
		perror("read");
		return NULL;
	}

	fsize = ntohl(fsize);

	printf("size of midi file: %u\n", fsize);
	puts("waiting for file...");

	char *fbuff = malloc(fsize);
	uint32_t recvsize = fsize;
	if (recvall(sockfd, fbuff, &fsize) == -1)
	{
		return NULL;
	}
	*size = fsize;
	return fbuff;
}

