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

	if (confirm_connection(sockfd) == -1)
	{
		client_exit();
	}

	int running = 1;
	while (running)
	{
		puts("waiting for server response");
		
		uint32_t fsize = 0;
		if (read(sockfd, &fsize, 4) == -1)
		{
			perror("read");
			client_exit();
		}

		printf("size of midi file: %uld\n", fsize);
		puts("waiting for file...");

		char *fbuff = malloc(fsize);
		uint32_t recvsize = fsize;
		if (recvall(sockfd, fbuff, &fsize) == -1)
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

		int reading = 1;
		// read instruction from server and exec
		while (reading)
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
				reading = 0;
				break;

			case CLIENT_QUIT:
				reading = 0;
				running = 0;
				break;
			default:
				puts("Unrecognized command");
			}
		}


		free(fbuff);
	}
	

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