#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <netdb.h>
#include <arpa/inet.h>

#include "midi_reader.h"
#include "midi_writer.h"
#include "midi_splitter.h"
#include "utils.h"
#include "midi.h"

// #define MIDI_FILE "beethoven_symphony_5.mid"
#define MIDI_FILE "mozart_1.mid"
// #define MIDI_FILE "out.mid"

#define OUT_FILE "out1.mid"

void *get_in_addr(struct sockaddr *sa)
{
	if (sa->sa_family == AF_INET) {
		return &(((struct sockaddr_in*)sa)->sin_addr);
	}

	return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

int main(void)
{
    char hostname[100];
    int hlen;
    if (gethostname(hostname, 100) == -1)
    {
        perror("gethostname");
    }
    struct hostent * entry = gethostbyname(hostname);

    char host_address[INET6_ADDRSTRLEN];
    inet_ntop(entry->h_addrtype, entry->h_addr_list[0], host_address, sizeof host_address);
    puts(host_address); 
    


    return 0;
    int num_connections = 20;
    struct Mfile *mfile = Mfile_from_file(MIDI_FILE);

    struct Mfile **mfiles = Mfile_split_by_tracks(mfile, num_connections);

    struct Mfile * tmp;
    char buff[100];
    for (int i = 0; i < num_connections; i++)
    {
        sprintf(buff, "myout%i.mid", i);
        Mfile_write_to_midi(mfiles[i], buff);

        tmp = mfiles[i];

        // DEBUG
        printf("-----------------------------------------------------------File %d:\n", i);
        // printf("name: %s\n", tmp->music_name);
        printf("ntracks: %d\n", tmp->ntracks);
        for (int j = 0; j < tmp->ntracks; j++)
        {
            struct Mtrack * trk = tmp->tracks[j];
            printf("----track %d\n", j);
            printf("name: %s\n", trk->name);
            printf("instrment: %s\n", trk->instrument);
            printf("track len: %d\n", trk->len);
        }
    }
    // Mfile_write_to_midi(mfile, OUT_FILE);
    return 0;
}