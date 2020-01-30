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
#include "utils.h"
#include "shell.h"
#include "midi_reader.h"


/** 
 * Count number of c in str
*/
int count_char(char *str, char c);
char **parse_line(char *line, int *len_ptr);

/** 
 * Write struct server_ctl @param server_control and @param value to @param server_fd
 * @return -1 on error, 0 on success
*/
int write_to_server(enum server_control_set server_control, int value);

// free stuff
static void cleanup();


// Handler declarations
int handle_load(char *filepath);
int handle_play();
int handle_pause();
int handle_resume();
int handle_restart();
int handle_loop();
int handle_noloop();
int handle_seek(int value);
int handle_status();
int handle_quit();
int handle_reconnect();



// defined in server.c
extern struct Mfile *mfile;
extern char **midi_out_names;

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
        {"restart", 0, "restart midi playback with current clients", handle_restart},
        {"loop", 0, "loop midi playback", handle_loop},
        {"noloop", 0, "don't look midi playback", handle_noloop},
        {"status", 0, "print current status of server", handle_status},
        {"quit", 0, "quit", handle_quit},
        {"reconnect", 0, "close all current clients and connect new ones", handle_reconnect}};

int main(int argc, char *argv[])
{
    int pipefd[2];
    if (pipe(pipefd) == -1)
    {
        perror("pipe");
    }
    server_fd = pipefd[1]; // for writing to server

    int control_fd = pipefd[0]; // server read from here
    int sockfd = setup_server();

    main_server = run_server(sockfd, control_fd);

    // done setting up server, start prompt for input

    char *cmd;
    int nwords;

    while (1)
    {
        if (!(line = readline("> ")))
        {
            continue;
        }

        parse_line(line, &nwords);

        cmd = args[0];

        int ncmds = sizeof(cmds) / sizeof(struct cmd *);
        struct cmd *cur_cmd;
        for (int i = 0; i < ncmds; i++)
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
                        break;
                    }
                    (cur_cmd->fn)(args[1]);
                }
                // there are no commands expecting 2 args

                // since command is executed, add to history and then stop comparing
                add_history(line);
                break;
            }
        }
        // if (strcmp(cmd, "load") == 0)
        // {

        // }
        // else if (strcmp(cmd, "play") == 0)
        // {
        //     /* code */
        // }
        // else if (strcmp(cmd, "pause") == 0)
        // {
        //     /* code */
        // }
        // else if (strcmp(cmd, "seek") == 0)
        // {
        //     /* code */
        // }
        // else if (strcmp(cmd, "restart") == 0)
        // {

        // }
        // else if (strcmp(cmd, "help") == 0)
        // {

        // }
        // else if (strcmp(cmd, "status") == 0)
        // {

        // }
        // else if (strcmp(cmd, "quit") == 0)
        // {

        // }
        // else
        // {
        //     fprintf(stdout, "%s: command no found", cmd);
        // }
    }
    return 0;
}


// -------------------------------------------- Handlers

int handle_load(char *filepath)
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

int handle_play()
{
    if (!mfile)
    {
        fprintf(stderr, "Which midi file to play?\n");
        return -1;
    }
    return write_to_server(SERVER_START_PLAYER, 0 /* not used */);
}

int handle_pause()
{
    return write_to_server(SERVER_SET_PLAYER, PLAYER_PAUSE);
}

int handle_resume()
{
    return write_to_server(SERVER_SET_PLAYER, PLAYER_RESUME);
}

int handle_restart()
{
    return write_to_server(SERVER_SET_PLAYER, PLAYER_RESTART);
}

int handle_loop()
{
    return write_to_server(SERVER_SET_PLAYER, PLAYER_LOOP);
}

int handle_noloop()
{
    return write_to_server(SERVER_SET_PLAYER, PLAYER_NOLOOP);
}

int handle_seek(int value)
{
    if (value < 0 || value > 100)
    {
        sys_error("Cannot seek a percentage > 100 or < 0");
        return -1;
    }
    return write_to_server(SERVER_SEEK_PLAYER, value);
}

int handle_status()
{
    return write_to_server(SERVER_PRINT_STATUS, 0 /* not used */);
}

int handle_quit()
{
    if (write_to_server(SERVER_QUIT, 0 /* not used */) == -1)
    {
        sys_error("Failed to communicate to main server. Force exiting...");
        cleanup();
        exit(EXIT_FAILURE);
    }
    // wait for main server to terminate
    pthread_join(main_server, NULL);
    cleanup();
    exit(EXIT_SUCCESS);
}

int handle_reconnect()
{
    return write_to_server(SERVER_RECONNECT, 0 /* not used */);
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

int write_to_server(enum server_control_set server_control, int value)
{
    struct server_ctl ctl;
    ctl.control = server_control;
    ctl.value = value;
    if (write(server_fd, &ctl, sizeof ctl) == -1)
    {
        perror("write");
        return -1;
    }
    return 0;
}

static void cleanup()
{
    free(line);
    free(args);
    line = NULL;
    args = NULL;
}