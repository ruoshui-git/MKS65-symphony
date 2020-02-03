#include <fluidsynth.h>
#include "midi_player.h"
#include "utils.h"

fluid_settings_t *settings = NULL;
fluid_synth_t *synth = NULL;
fluid_player_t *player = NULL;
fluid_audio_driver_t *adriver = NULL;

// int fluid_player_seek(fluid_player_t *player, int ticks) __attribute__((weak));

void player_setup()
{
    
    puts("fluid version: "FLUIDSYNTH_VERSION);
    if (!settings)
        settings = new_fluid_settings();
    if (!synth)
        synth = new_fluid_synth(settings);
    if (player)
        delete_fluid_player(player);
    player = new_fluid_player(synth); // default
    // player = new_fluid_player(NULL); // no synth

    fluid_settings_setstr(settings, "audio.driver", "pulseaudio");
    // fluid_settings_setstr(settings, "synth.midi-channels", "256");
    fluid_settings_setstr(settings, "midi.driver", "alsa_seq");


    fluid_synth_sfload(synth, MIDI_SOUNDFONT, 1);
    if (!adriver)
        adriver = new_fluid_audio_driver(settings, synth);
}

void player_add_midi_file(char *path)
{
    fluid_player_add(player, path);
}

void player_clear_midi_files()
{
    if (!player)
        return;
    fluid_player_stop(player);
    delete_fluid_player(player);
    new_fluid_player(synth);
}

void player_play()
{
    fluid_player_play(player);
}

void player_pause()
{
    fluid_player_stop(player);
}

int player_get_total_ticks()
{
    return fluid_player_get_total_ticks(player);
}

int player_get_cur_tick()
{
    return fluid_player_get_current_tick(player);
}

void player_seek(int tick)
{
    fluid_player_seek(player, tick);
}

void player_setloop(int looping)
{
    fluid_player_set_loop(player, looping);
}

void player_restart()
{
    fluid_player_seek(player, 0);
}

void player_cleanup()
{
    if (adriver)
        delete_fluid_audio_driver(adriver);
    if (player)
        delete_fluid_player(player);
    if (synth)
        delete_fluid_synth(synth);
    if (settings)
        delete_fluid_settings(settings);
}

void player_add_midi_mem(char *buff, size_t len)
{
    fluid_player_add_mem(player, buff, len);
}

int player_get_status()
{
    if (!player) return -1;
    int status = fluid_player_get_status(player);
    if (status == FLUID_PLAYER_DONE) return 2;
    if (status == FLUID_PLAYER_PLAYING) return 1;
    if (status == FLUID_PLAYER_READY) return 0;
}
