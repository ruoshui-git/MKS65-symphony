#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#include "server_thread.h"

int sendall(int sockfd, void *buf, int *len);


pthread_t new_server_thread(struct s_thread_arg * arg)
{
    pthread_t thread;
    if (pthread_create(&thread, NULL, server_thread, arg) != 0)
    {
        perror("pthread_create");
        exit(1);
    }
    return thread;
}

void * server_thread(void * _arg)
{
    struct s_thread_arg * arg = (struct s_thread_arg *) _arg;
    int client = arg->socket;
    int my_id = arg->tid;
    sig_atomic_t * midi_ready = arg->midi_ready;
    pthread_mutex_t * midi_ready_cond_mutex = arg->midi_ready_cond_mutex;
    pthread_cond_t * midi_ready_cond = arg->midi_ready_cond;

    ssize_t bytes_sent;
    int i;
    char buff[100];
    snprintf(buff, sizeof buff, "connected");

    // send "connected signal"
    printf("bytes of 'connected' sent: %ld\n", send(client, buff, 10 /* strlen(buff) */, 0));

    // send client id, == thread id
    u_int32_t client_id = htonl(my_id);
    int len = 4;
    printf("sendall status: %d\n", sendall(client, &client_id, &len));
    // send(client, &client_id, 32, 0);

    // start waiting for file to be ready
    pthread_mutex_lock(midi_ready_cond_mutex);
    while (!*midi_ready)
    {
        pthread_cond_wait(midi_ready_cond, midi_ready_cond_mutex);
    }
    pthread_mutex_unlock(midi_ready_cond_mutex);

    // acquire file and send to client

    

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

struct tnode * new_tnode(pthread_t thread, struct s_thread_arg * arg)
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


/** 
 * Attempts to send all data
 * 
 * // from https://beej.us/guide/bgnet/html/#sendall
*/
int sendall(int sockfd, void *buf, int *len)
{
    int total = 0;        // how many bytes we've sent
    int bytesleft = *len; // how many we have left to send
    int n;

    while(total < *len) {
        n = send(sockfd, buf+total, bytesleft, 0);
        if (n == -1) { break; }
        total += n;
        bytesleft -= n;
        printf("bytessent: %d, bytesleft: %d\n", n, bytesleft);
    }

    *len = total; // return number actually sent here

    if (n == -1)
    { perror("send"); }

    return n==-1?-1:0; // return -1 on failure, 0 on success
}