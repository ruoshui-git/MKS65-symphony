#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>

#include <readline/readline.h>
#include <readline/history.h>

#include <fluidsynth.h>

#include "server.h"
#include "midi.h"
#include "midi_reader.h"
#include "midi_splitter.h"
#include "server_local_protocol.h"

/** 
 * Count number of c in str
*/
int count_char(char *str, char c);

char **parse_line(char *line);

struct Mfile * mfile = NULL;
char ** midi_out_names = NULL;
int server_fd = -1; // for writing to server

int main(int argc, char *argv[])
{
    int pipefd[2];
    if (pipe(pipefd) == -1)
    {
        perror("pipe");
    }
    int server_fd = pipefd[1]; // for writing to server

    int control_fd = pipefd[0]; // server read from here
    int sockfd = setup_server();

    pthread_t main_server = run_server(sockfd, control_fd);

    // done setting up server, start prompt for input

    char *line;
    char **args;

    while (1)
    {
        line = readline("> ");


        char ** args = parse_line(line);

        



        free(line);
        free(args);
        line = NULL;
        args = NULL;
    }
    return 0;
}

char **parse_line(char *line)
{
    // malloc the size of the number of words, separated by ' '
    char ** args = malloc(sizeof(char *) * (1 + count_char(line, ' ')));
    int i = 0;
    char * sep = " ";
    args[i++] = strtok(line, sep);
    char * token = strtok(NULL, sep);
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

int handle_load(char * filepath)
{
    if (!fluid_is_midifile(filepath))
    {
        fprintf(stderr, "%s is not a midi file\n", filepath);
        return -1;
    }
    mfile = Mfile_from_file(filepath);
    if (!mfile)
    {
        fprintf(stderr, "Failed to load midi file: %s\n", filepath);
        return -1;
    }
    return 0;
}

int handle_play(int server_fd)
{
    if (!mfile)
    {
        fprintf(stderr, "Which midi file to play?");
        return -1;
    }
    struct server_ctl cmd;
    cmd.control = SERVER_START_PLAYER;
    cmd.value = 0; // not needed;
    if (write(server_fd, &cmd, sizeof(cmd)) == -1)
    {
        perror("write");
        return -1;
    }
    return 0;
}


static cleanup()
{
    
}