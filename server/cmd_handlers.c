// -------------------------------------------- Handlers

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fluidsynth.h>
#include <unistd.h>
#include <errno.h>
#include <assert.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <netdb.h>
#include <ctype.h>

#include <readline/readline.h>

#include "shell.h"
#include "midi.h"
#include "midi_reader.h"
#include "midi_player.h"
#include "midi_writer.h"
#include "midi_splitter.h"
#include "server_thread.h"
#include "cmd_handlers.h"
#include "utils.h"
#include "socket.h"

extern struct Mfile *mfile;

// set up sockets

socklen_t sin_size;
char s[INET6_ADDRSTRLEN];           // hold address str
int client_fd;                      // new connection descriptor
struct sockaddr_storage their_addr; // connector's address information

/* keeps track of the state of midi files */
int midi_ready = 0; // midi files are not generated until after all clients are connected
pthread_cond_t midi_ready_cond = PTHREAD_COND_INITIALIZER;
pthread_mutex_t midi_ready_cond_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_barrier_t midi_play_barrier;
int barrier_initialized = 0;
struct tlist *clients = NULL;
// int sockfd = -1;

int accepting_clients = 1; // whether server is accepting clients
int tid;                   // thread id

// structs and strs for midi files
struct Mfile *mfile;
char **midi_out_names;

// name of temp dir for midi files
char *tmp_dir = NULL;

extern const int cmds_len;
extern struct cmd cmds[];

extern int running;

/* Callback function called for each line when accept-line executed, EOF
   seen, or EOF character read.  This sets a flag and returns; it could
   also call exit(3). */
// static void
// cb_linehandler (char *line)
// {
//   /* Can use ^D (stty eof) or `exit' to exit. */
//   if (line == NULL || strcmp (line, "exit") == 0)
//     {
//       if (line == 0)
//         printf ("\n");
//       printf ("exit\n");
//       /* This function needs to be called to reset the terminal settings,
//          and calling it from the line handler keeps one extra prompt from
//          being displayed. */
//       rl_callback_handler_remove ();

//       running = 0;
//     }
//   else
//     {
//       if (*line)
//         add_history (line);
//       printf ("input line: %s\n", line);
//       free (line);
//     }
// }

void setup_connections()
{
    clients = new_tlist();
    tid = 0; // thread id
    accepting_clients = 1;
    printf("server: waiting for connections...\n");
}

void clear_connections()
{
    if (clients)
    {
        free_tlist(clients);
    }
    tid = 0;
}

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
        fprintf(stderr, "File not loaded\n");
        return -1;
    }

    if (clients->len == 0)
    {
        fprintf(stdout, "No clients connected, playing on server only\n");
        player_setup();
        player_add_midi_file(mfile->fullpath);
        player_play();
    }
    else
    {
        create_midi_files();

        // send files to clients
        pthread_mutex_lock(&midi_ready_cond_mutex);
        midi_ready = 1;

        if (!barrier_initialized)
        {
            pthread_barrier_init(&midi_play_barrier, 0, clients->len + 1);
            barrier_initialized = 1;
        }

        pthread_cond_broadcast(&midi_ready_cond);
        pthread_mutex_unlock(&midi_ready_cond_mutex);

        player_setup();
        player_add_midi_file(mfile->fullpath);
        pthread_barrier_wait(&midi_play_barrier);
        // wait for some time;
        player_play();
    }

    return 1;
}

int handle_pause()
{
    player_pause();
    return 0;
}

int handle_resume()
{
    player_play();
    return 0;
}

int handle_loop()
{
    player_setloop(1);
    return 0;
}

int handle_noloop()
{
    player_setloop(0);
    return 0;
}

int handle_restart()
{
    player_restart();
    return 0;
}

int handle_seek(char* val)
{
    int n = strlen(val);
    if (val[n-1] == '%')
    {
        // this is a percentage
        val[n-1] = '\0';
        if (!isnum(val))
        {
            fputs("Not a valid percentage\n", stderr);
            return -1;
        }
        int p = atoi(val);
        if (p > 100 || p < 0)
        {
            fputs("Percentage must be < 100 and > 0\n", stderr);
            return -1;
        }
        int tick = p * player_get_total_ticks() / 100;
        player_seek(tick);
        return 0;
    }
    else
    {
        if (!isnum(val))
        {
            fputs("Not a valid tick number\n", stderr);
            return -1;
        }
        player_seek(atoi(val));
        return 0;
    }
    
}

int handle_status()
{
    printf("-- Num clients: %d\n", clients->len);
    if (mfile)
    {
        printf("-- Midi file loaded: %s\n", mfile->filename);
    }
    int status = player_get_status();
    if (status == 0)
    {
        puts("-- Player is ready");
    }
    else if (status == 1)
    {
        puts("-- Player is playing");
    }
    else if (status == 2)
    {
        puts("-- Player has been stopped");
    }
    else
    {
        // status == -1, no player yet
        puts(" * More status will be displayed when player starts");
    }
    if (status != -1)
    {
        int cur_tick = player_get_cur_tick();
        int total_ticks = player_get_total_ticks();
        // TODO: handle total_ticks == 0, which somehow means player ended
        int p = 100 * cur_tick / total_ticks;
        printf("-- Current playback position: %d / %d ticks, %d%%\n", cur_tick, total_ticks, p);
    }
    
    
    return 1;
}

int handle_quit()
{
    
    puts("Cleaning server");
    if (clients)
    {
        struct tnode *cur = clients->first;
        while (cur)
        {
            pthread_cancel(cur->thread);
            cur = cur->next;
        }
        free(clients);
    }
    if (mfile)
    {
        free_Mfile(mfile);
    }
    pthread_cond_destroy(&midi_ready_cond);
    pthread_mutex_destroy(&midi_ready_cond_mutex);
    if (barrier_initialized)
        pthread_barrier_destroy(&midi_play_barrier);
    player_cleanup();
    running = 0;
    rl_callback_handler_remove();
    return 2;
}

int handle_reconnect()
{
    clear_connections();
    setup_connections();
}

int handle_help()
{
    fprintf(stdout, "Available commands:\n");
    for (int i = 0; i < cmds_len; i++)
    {
        fprintf(stdout, "%s\t\t%s\n", cmds[i].name, cmds[i].desc);
    }
    fputs("\n\n", stdout);
}

int handle_socket(int sockfd)
{
    // we got a new connection, accept itsin_size = sizeof their_addr;
    client_fd = accept(sockfd, (struct sockaddr *)&their_addr, &sin_size);
    if (client_fd == -1)
    {
        perror("accept");
        return 1;
    }

    // if not accepting clients, close the connection immediately
    if (!accepting_clients)
    {
        close(client_fd);
        return 2;
    }

    inet_ntop(their_addr.ss_family,
              get_in_addr((struct sockaddr *)&their_addr),
              s, sizeof s);

    fprintf(stdout, "server: got connection from %s\n", s);
    fflush(stdout);

    struct s_thread_arg *arg = malloc(sizeof(struct s_thread_arg));
    arg->socket = client_fd;
    arg->tid = tid;
    arg->midi_ready = &midi_ready;
    arg->midi_ready_cond = &midi_ready_cond;
    arg->midi_ready_cond_mutex = &midi_ready_cond_mutex;
    arg->midi_play_barrier = &midi_play_barrier;

    pthread_t thread = new_server_thread(arg);
    struct tnode *node = new_tnode(thread, arg);
    append_tnode(clients, node);

    printf("num threads: %d\n", clients->len);

    tid++;
}

// -------------------------------- helpers

// modifies global
void create_midi_files()
{
    accepting_clients = 0;
    puts("server: no longer accepting connections");

    midi_ready = 0; // (re)set midi_file conditions
    // set up files
    assert(mfile != NULL);
    int nclients = clients->len;
    struct Mfile **midi_out = Mfile_split_by_tracks(mfile, nclients);

    make_tmp_dir();

    // write out files to tmp_dir
    midi_out_names = malloc(sizeof(char *) * nclients);
    for (int i = 0; i < nclients; i++)
    {
        char fname[strlen(MIDI_OUT_FORMAT) + 2];
        sprintf(fname, MIDI_OUT_FORMAT, i);
        char *absfname = malloc(strlen(tmp_dir) + strlen(fname) + 1); // + 1 for null
        strcpy(absfname, tmp_dir);
        strcat(absfname, fname);
        Mfile_write_to_midi(midi_out[i], absfname);
        midi_out_names[i] = absfname;
    }
}

// modifies global
void make_tmp_dir(void)
{
    if (!tmp_dir)
    {
        char *wd = getcwd(NULL, 0);
        tmp_dir = malloc(strlen(wd) + 4); // wd + "tmp/"
        strcpy(tmp_dir, wd);
        strcat(tmp_dir, "tmp/");
        free(wd);
    }

    // check if tmp_dir already exist
    struct stat s;
    if (stat(tmp_dir, &s) == -1)
    {
        if (errno = ENOENT)
        {
            // tmp dir doesn't exist
            mkdir(tmp_dir, 641);
        }
        else
        {
            perror("stat");
            // clean up
            exit(1);
        }
    }
    else
    {
        if (!S_ISDIR(s.st_mode))
        {
            sys_warning("Please remove the file with name 'tmp' since a dir with this name needs to be created");
            // clean up
            exit(1);
        }
    }
}


int isnum(char *s)
{
    for (int i = 0, n = strlen(s); i < n; i++)
    {
        if (!isdigit(s[i]))
            return 0;
    }
    return 1;
}