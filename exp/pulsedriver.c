#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <assert.h>
#include <stdlib.h>

#include <pulse/error.h>
#include <pulse/sample.h>
#include <pulse/pulseaudio.h>

#include <pulse/simple.h>

#include <fluidsynth.h>

#include "const.h"

int pulse_play();

#define BUFSIZE 1024

int main(int argc, char*argv[]) {
    int i;

    fluid_settings_t * settings;
    fluid_synth_t * synth;
    fluid_player_t * player;
    settings = new_fluid_settings();
    synth = new_fluid_synth(settings);
    player = new_fluid_player(synth);

    // add file to player
    fluid_player_add(player, MPATH);

    // 1 = reassign presets for all midi channels
    fluid_synth_sfload(synth, SFPATH, 1);

    fluid_player_play(player);

    // pulseaudio stuff
    /* The Sample format to use */
    static const pa_sample_spec ss = {
        .format = PA_SAMPLE_FLOAT32NE,
        .rate = 44100,
        .channels = 2
    };
    pa_simple *s = NULL;
    int ret = 1;
    int error;



    /* replace STDIN with the specified file if needed */
    // if (argc > 1) {
    //     int fd;
    //     if ((fd = open(argv[1], O_RDONLY)) < 0) {
    //         fprintf(stderr, __FILE__": open() failed: %s\n", strerror(errno));
    //         goto finish;
    //     }
    //     if (dup2(fd, STDIN_FILENO) < 0) {
    //         fprintf(stderr, __FILE__": dup2() failed: %s\n", strerror(errno));
    //         goto finish;
    //     }
    //     close(fd);
    // }
    debug: puts("connecting to pulseaudio");
    /* Create a new playback stream */
    if (!(s = pa_simple_new(NULL, APP_NAME, PA_STREAM_PLAYBACK, NULL, "Music", &ss, NULL, NULL, &error))) {
        fprintf(stderr, __FILE__": pa_simple_new() failed: %s\n", pa_strerror(error));
        goto finish;
    }

    debug1: puts("trying to play");




    for (;;) {
        int buf[BUFSIZE];
        ssize_t r;
#if 0
        pa_usec_t latency;
        if ((latency = pa_simple_get_latency(s, &error)) == (pa_usec_t) -1) {
            fprintf(stderr, __FILE__": pa_simple_get_latency() failed: %s\n", pa_strerror(error));
            goto finish;
        }
        fprintf(stderr, "%0.0f usec    \r", (float)latency);
#endif
        /* Read some data ... */
        // if ((r = read(STDIN_FILENO, buf, sizeof(buf))) <= 0) {
        //     if (r == 0) /* EOF */
        //         break;
        //     fprintf(stderr, __FILE__": read() failed: %s\n", strerror(errno));
        //     goto finish;
        // }


        memset(buf, 0, BUFSIZE);

        fluid_synth_write_float(synth, BUFSIZE/2, buf, 0, 2, buf, 1, 2);



        /* ... and play it */
        if (pa_simple_write(s, buf, (size_t) (BUFSIZE), &error) < 0) {
            fprintf(stderr, __FILE__": pa_simple_write() failed: %s\n", pa_strerror(error));
            goto finish;
        }
    }
    /* Make sure that every single sample was played */
    if (pa_simple_drain(s, &error) < 0) {
        fprintf(stderr, __FILE__": pa_simple_drain() failed: %s\n", pa_strerror(error));
        goto finish;
    }
    ret = 0;
finish:
    if (s)
        pa_simple_free(s);
    return ret;


}

int pulse_play(fluid_synth_t * synth)
{

    return 0;
}
