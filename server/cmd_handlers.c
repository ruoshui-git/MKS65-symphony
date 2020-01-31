// -------------------------------------------- Handlers

#include <stdio.h>
#include <stdlib.h>

#include "server.h"
#include "midi.h"
#include "midi_reader.h"

extern struct Mfile * mfile;
extern int cmd_len;

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

    return 1;

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

int handle_status()
{
    printf("Num clients: %d\n", clients->len);
    return 1;
}

int handle_quit()
{
    
    struct tnode * cur = clients->first;
    while (cur)
    {
        pthread_cancel(cur->thread);
        cur = cur->next;
    }
    free(clients);
    cleanup();
    exit(EXIT_SUCCESS);
}

int handle_reconnect()
{
    return write_to_server(SERVER_RECONNECT, 0 /* not used */);
}

int handle_help()
{
    fprintf(stdout, "Available commands:\n");
    for (int i = 0; i < cmds_len; i++)
    {
        fprintf(stdout, "%s\t\t%s\n", cmds[i].name, cmds[i].desc);
    }
}

