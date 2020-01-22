#include <pthread.h>

#ifndef SERVER_THREAD_H
#define SERVER_THREAD_H
struct s_thread_arg
{
    int socket;
    int tid;
};

struct tlist
{
    struct tnode * first;
    struct tnode * last;
    int len;
};

struct tnode
{
    pthread_t thread;
    struct s_thread_arg arg;
    struct tnode * next;
};

#endif

/** Create a new server thread to handle connection */
pthread_t new_server_thread(struct s_thread_arg * arg);
void * server_thread(void * _arg);

struct tlist * new_tlist();
void append_tnode(struct tlist * list, struct tnode * node);
struct tnode * new_tnode(pthread_t thread, struct s_thread_arg arg);
