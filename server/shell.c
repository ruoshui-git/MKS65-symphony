#include <stdio.h>
#include <unistd.h>
#include <readline/readline.h>
#include <readline/history.h>
#include <pthread.h>

#include "server.h"

int main(int argc, char* argv[])
{
    int pipefd[2];
    if (pipe(pipefd) == -1)
    {
        perror("pipe");
    }
    int server_fd = pipefd[1]; // for writing to server
    
    int control_fd = pipefd[0]; // server read from here
    int sockfd = setup_server();

    pthread_t = run_server(sockfd, control_fd);

    // done setting up server, start prompt for input

    char * line;
    while (1)
    {
        line = readline(">");
        puts(line);
        free(line);
    }
    return 0;
}