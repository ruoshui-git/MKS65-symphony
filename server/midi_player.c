#include <fluidsynth.h>
#include "midi_player.h"
#include "utils.h"

fluid_settings_t *settings;
fluid_synth_t *synth;
fluid_player_t *player;
fluid_audio_driver_t *adriver;

int fluid_player_seek(fluid_player_t *player, int ticks) __attribute__((weak));

void player_setup()
{
    settings = new_fluid_settings();
    synth = new_fluid_synth(settings);
    player = new_fluid_player(synth); // default
    // player = new_fluid_player(NULL); // no synth

    fluid_settings_setstr(settings, "audio.driver", "pulseaudio");
    fluid_settings_setstr(settings, "synth.midi-channels", "256");

    fluid_synth_sfload(synth, MIDI_SOUNDFONT, 1);
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

void player_seek(int tick)
{
    if (fluid_player_seek)
    {
        fluid_player_seek(player, tick);
    }
    else
    {
        sys_warning("Does not support seek due to library limitations");
    }
}

void player_setloop(int looping)
{
    fluid_player_set_loop(player, looping);
}
