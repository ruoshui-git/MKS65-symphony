#include <pthread.h>
#include <signal.h>

#ifndef SERVER_THREAD_H
#define SERVER_THREAD_H
struct s_thread_arg
{
    int socket;
    int tid;
    sig_atomic_t * midi_ready;
    pthread_cond_t * midi_ready_cond;
    pthread_mutex_t * midi_ready_cond_mutex;
    pthread_barrier_t * midi_play_barrier;
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
    struct s_thread_arg * arg;
    struct tnode * next;
};

#define MIDI_OUT_FORMAT "out_%d.mid"

#endif

/** Create a new server thread to handle connection */
pthread_t new_server_thread(struct s_thread_arg * arg);
void * server_thread(void * _arg);

struct tlist * new_tlist();
void append_tnode(struct tlist * list, struct tnode * node);
struct tnode * new_tnode(pthread_t thread, struct s_thread_arg * arg);

void * free_tnode(struct tnode * n);
void * free_tlist(struct tlist * l);
