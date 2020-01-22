#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/types.h>
#include <sys/socket.h>

#include "server_thread.h"



pthread_t new_server_thread(struct s_thread_arg * arg)
{
    pthread_t thread;
    pthread_create(&thread, NULL, server_thread, arg);
    return thread;
}

void * server_thread(void * _arg)
{
    struct s_thread_arg * arg = (struct s_thread_arg *) _arg;
    int client = arg->socket;
    int my_id = arg->tid;
    ssize_t bytes_sent;
    int i;
    char buff[100];
    snprintf(buff, sizeof buff, "connected", my_id);
    bytes_sent = send(client, buff, 10 /* strlen(buff) */, 0);

    return EXIT_SUCCESS;
}

struct tlist * new_tlist()
{
    return calloc(1, sizeof(struct tlist));
}

void append_tnode(struct tlist * list, struct tnode * node)
{
    if (list->len == 0)
    {
        list->first = node;
        list->last = node;
    }
    else
    {
        list->last = list->last->next = node;
    }
    list->len++;
}

struct tnode * new_tnode(pthread_t thread, struct s_thread_arg arg)
{
    struct tnode * node = malloc(sizeof(struct tnode));
    if (!node)
    {
        perror("malloc");
        exit(1);
    }
    node->arg = arg;
    node->thread = thread;
    node->next = NULL;
    return node;
}


// from https://beej.us/guide/bgnet/html/#sendall
int sendall(int s, char *buf, int *len)
{
    int total = 0;        // how many bytes we've sent
    int bytesleft = *len; // how many we have left to send
    int n;

    while(total < *len) {
        n = send(s, buf+total, bytesleft, 0);
        if (n == -1) { break; }
        total += n;
        bytesleft -= n;
    }

    *len = total; // return number actually sent here

    return n==-1?-1:0; // return -1 on failure, 0 on success
}