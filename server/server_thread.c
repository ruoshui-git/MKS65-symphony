#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#include "server_thread.h"
#include "../client_protocol.h"

int sendall(int sockfd, void *buf, int *len);

/** Get absolute midi file name based on id */
char *get_midi_filename(int id);

extern char *tmp_dir; // defined in server.c

pthread_t new_server_thread(struct s_thread_arg *arg)
{
    pthread_t thread;
    if (pthread_create(&thread, NULL, server_thread, arg) != 0)
    {
        perror("pthread_create");
        exit(1);
    }
    return thread;
}

void *server_thread(void *_arg)
{
    struct s_thread_arg *arg = (struct s_thread_arg *)_arg;
    int client = arg->socket;
    int my_id = arg->tid;
    sig_atomic_t *midi_ready = arg->midi_ready;
    pthread_mutex_t *midi_ready_cond_mutex = arg->midi_ready_cond_mutex;
    pthread_cond_t *midi_ready_cond = arg->midi_ready_cond;

    ssize_t bytes_sent;
    int i;
    char buff[100];
    snprintf(buff, sizeof buff, "connected");

    // send "connected signal"
    printf("bytes of 'connected' sent: %ld\n", send(client, buff, 10 /* strlen(buff) */, 0));

    // send client id, == thread id
    u_int32_t client_id = htonl(my_id);
    int len = 4;
    sendall(client, &client_id, &len);
    // send(client, &client_id, 32, 0);

    // start waiting for file to be ready
    pthread_mutex_lock(midi_ready_cond_mutex);
    while (!*midi_ready)
    {
        pthread_cond_wait(midi_ready_cond, midi_ready_cond_mutex);
    }
    pthread_mutex_unlock(midi_ready_cond_mutex);

    // acquire file and send to client
    char *fname = get_midi_filename(my_id);
    FILE *f = fopen(fname, "r");
    if (!f)
    {
        perror("fopen");
        exit(1);
    }

    // get file size
    fseek(f, 0, SEEK_END);
    uint32_t fsize = ftell(f);
    uint32_t nfsize = htonl(fsize);

    len = 4;

    // send file size first
    sendall(client, &nfsize, &len);

    // read and send file data
    char buf[fsize];
    fseek(f, 0, SEEK_SET);
    fread(buf, fsize, 1, f);
    if (ferror(f))
    {
        perror("fread");
        goto handle_error;
    }

    if (sendall(client, buf, &fsize) == -1)
    {
        goto handle_error;
    }

    // wait for client to respond
    unsigned char cbuff;
    if (recv(client, &cbuff, 1, 0) == -1)
    {
        perror("recv");
    }

    // now file is on client side
    // call barrier and start playing
    
    cbuff = CLIENT_PLAY;
    if (send(client, &cbuff, 1, 0) == -1)
    {
        perror("send");
    }

    // wait for more commands, and send them
    

    return EXIT_SUCCESS;

    handle_error:
        close(client);
        int val = 1;
        pthread_exit(&val);
}

struct tlist *new_tlist()
{
    return calloc(1, sizeof(struct tlist));
}

void append_tnode(struct tlist *list, struct tnode *node)
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

struct tnode *new_tnode(pthread_t thread, struct s_thread_arg *arg)
{
    struct tnode *node = malloc(sizeof(struct tnode));
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

void * free_tnode(struct tnode * n)
{
    free(n->arg);
    free(n);
    return NULL;
}

void * free_tlist(struct tlist * l)
{
    struct tnode * n = l->first, *next;
    while (n)
    {
        next = n->next;
        free_tnode(n);
        n = next;
    }
    free(l);
    return NULL;
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

    while (total < *len)
    {
        n = send(sockfd, buf + total, bytesleft, 0);
        if (n == -1)
        {
            break;
        }
        total += n;
        bytesleft -= n;
        printf("bytessent: %d, bytesleft: %d\n", n, bytesleft);
    }

    *len = total; // return number actually sent here

    if (n == -1)
    {
        perror("send");
    }

    return n == -1 ? -1 : 0; // return -1 on failure, 0 on success
}

char *get_midi_filename(int id)
{
    char fname[strlen(MIDI_OUT_FORMAT) + 2];
    sprintf(fname, MIDI_OUT_FORMAT, id);
    char *absfname = malloc(strlen(tmp_dir) + strlen(fname) + 1); // + 1 for null
    strcpy(absfname, tmp_dir);
    strcat(absfname, fname);
    return absfname;
}