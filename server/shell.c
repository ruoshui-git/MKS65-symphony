#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <netdb.h>
#include <signal.h>
#include <assert.h>

#include <pthread.h>

/* Standard readline include files. */
#include <readline/readline.h>
#include <readline/history.h>

#include "server_thread.h"
#include "shell.h"
#include "midi_reader.h"
#include "midi_splitter.h"
#include "midi_writer.h"
#include "midi_player.h"
#include "cmd_handlers.h"
#include "utils.h"
#include "socket.h"

// from https://beej.us/guide/bgnet/html/#a-simple-stream-server

char **args;
char *line;
int sockfd = -1;

/** Reset cli session for next prompt */
static void session_cleanup();

/** Clear shell before exiting */
static void shell_cleanup();

static void cb_linehandler(char *);
static void sighandler(int);

int sigwinch_received;
const char *prompt = "> ";

int running = 0;

/* Handle SIGWINCH and window size changes when readline is not active and
   reading a character. */
static void
sighandler(int sig)
{
    if (sig == SIGWINCH)
    {
        sigwinch_received = 1;
    }
    else if (sig == SIGINT)
    {
        // condition on ^C is too complicated

        // fputs("\nDo you really want to quit? y/[n] ", stdout);
        // char c = getchar();
        // putchar(c);
        // putchar('\n');
        // if (c == 'y' || c == 'Y')
        // {
            puts("quit");
            shell_exit();
        // }
        // else
        // {
        //     return;
        // }
    }
}

struct cmd cmds[] =
    {
        {"load", 1, "load [*.mid] - load a midi file", handle_load},
        {"play", 0, "stop accepting new connections and start playing midi file; file has to be loaded", handle_play},
        {"pause", 0, "pause midi playback", handle_pause},
        {"resume", 0, "resume midi playback", handle_resume},
        {"restart", 0, "restart midi playback with current clients", handle_restart},
        {"seek", 1, "'seek [tick] | [percent]%' - seek playback to a certain tick or percentage", handle_seek},
        {"loop", 0, "loop midi playback", handle_loop},
        {"noloop", 0, "don't loop midi playback", handle_noloop},
        {"status", 0, "print current status of server", handle_status},
        {"quit", 0, "quit", handle_quit},
        {"exit", 0, "quit", handle_quit},
        {"reconnect", 0, "close all current clients and connect new ones", handle_reconnect},
        {"help", 0, "print help message", handle_help}};

const int cmds_len = sizeof(cmds) / sizeof(struct cmd);

/* Callback function called for each line when accept-line executed, EOF
   seen, or EOF character read.  This sets a flag and returns; it could
   also call exit(3). */
static void cb_linehandler(char *line)
{
    /* Can use ^D (stty eof) or `exit' to exit. */

    if (line == 0)
    {
        puts("quit");
        shell_exit();
    }
    if (strlen(line) == 0)
    {
        return;
    }

    // add line to history now before we split the string    
    add_history(line);

    // set up reading from stdin
    char *cmd;
    int nwords;
    int command_found = 0;

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
                    break;
                }
                (cur_cmd->fn)(args[1]);
            }
            // there are no commands expecting 2 args
            command_found = 1;
            break;
        }
    }
    if (command_found)
    {
        command_found = 0; // reset
    }
    else
    {
        fprintf(stderr, "%s: command not found\n", cmd);
    }

    free(line);
}

int main(int argc, char *argv[])
{

    sockfd = setup_server();
    setup_connections();

    
    puts("Orchestra\nType 'help' to see help messages");


    /* Handle window size changes when readline is not active and reading
     characters. */
    signal(SIGWINCH, sighandler);
    signal(SIGINT, sighandler);
    /* Install the line handler. */
    rl_callback_handler_install(prompt, cb_linehandler);

    fd_set readfds, readfds_copy;
    FD_ZERO(&readfds);
    FD_SET(sockfd, &readfds);
    FD_SET(fileno(rl_instream), &readfds);

    running = 1;

    while (running)
    {

        // select readline https://tiswww.case.edu/php/chet/readline/readline.html#SEC41
        readfds_copy = readfds;
        if (select(max(sockfd, fileno(rl_instream)) + 1, &readfds_copy, NULL, NULL, NULL) == -1)
        {
            perror("select");
            shell_cleanup();
            exit(4);
        }
        if (sigwinch_received)
        {
            rl_resize_terminal();
            sigwinch_received = 0;
        }
        if (FD_ISSET(fileno(rl_instream), &readfds_copy))
        {
            rl_callback_read_char();
        }
        else if (FD_ISSET(sockfd, &readfds_copy)) // socket
        {
            handle_socket(sockfd);
        }
        else
        {
            puts("Got unknown fd from select set");
        }
        session_cleanup();
    }
    shell_cleanup();
    return 0;
}

//-------------------------------------------- HELPERS

static void session_cleanup()
{
    if (line)
        free(line);
    if (args)
        free(args);
    line = NULL;
    args = NULL;
}

static void shell_cleanup()
{
    puts("Cleaning shell");
    /* This function needs to be called to reset the terminal settings,
        and calling it from the line handler keeps one extra prompt from
        being displayed. */
    rl_callback_handler_remove();

    session_cleanup();
    close(sockfd);
}

void shell_exit()
{
    running = 0;
    handle_quit();
    shell_cleanup();
    exit(1);
}