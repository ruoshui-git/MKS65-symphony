#include <fluidsynth.h>
#include "fluidsup.h"

int network_event_handler(void * data, fluid_midi_event_t *event);

int main(int argc, char** argv)
{
    int i;
    fluid_settings_t* settings;
    fluid_synth_t* synth;
    fluid_player_t* player;
    fluid_audio_driver_t* adriver;
    settings = new_fluid_settings();
    synth = new_fluid_synth(settings);
    player = new_fluid_player(synth); // default
    // player = new_fluid_player(NULL); // no synth

    fluid_settings_setstr(settings, "audio.driver", "pulseaudio");
    fluid_settings_setstr(settings, "synth.midi-channels", "256");

    // fluid_player_set_playback_callback(player, fluid_synth_handle_midi_event, synth); // default
    fluid_player_set_playback_callback(player, network_event_handler, synth);

    /* process command line arguments */
    for (i = 1; i < argc; i++) {
        if (fluid_is_soundfont(argv[i])) {
           fluid_synth_sfload(synth, argv[1], 1);
        }
        if (fluid_is_midifile(argv[i])) {
            fluid_player_add(player, argv[i]);
        }
    }
    /* start the synthesizer thread */
    adriver = new_fluid_audio_driver(settings, synth);

    /* play the midi files, if any */
    fluid_player_play(player);
    /* wait for playback termination */
    fluid_player_join(player);
    /* cleanup */
    delete_fluid_audio_driver(adriver);
    delete_fluid_player(player);
    delete_fluid_synth(synth);
    delete_fluid_settings(settings);
    return 0;
}

/**
 * Handle MIDI event from MIDI router, used as a callback function.
 * @param data FluidSynth instance
 * @param event MIDI event to handle
 * @return #FLUID_OK on success, #FLUID_FAILED otherwise
 */
int network_event_handler(void * data, fluid_midi_event_t *event)
{
    fluid_synth_t *synth = (fluid_synth_t *) data;
    int type = fluid_midi_event_get_type(event);
    int chan = fluid_midi_event_get_channel(event);

    // if (chan != 15) return FLUID_OK;
    // printf("get channel %d;\n", chan);


    switch(type)
    {
    case NOTE_ON:
        return fluid_synth_noteon(synth, chan,
                                  fluid_midi_event_get_key(event),
                                  fluid_midi_event_get_velocity(event));

    case NOTE_OFF:
        return fluid_synth_noteoff(synth, chan, fluid_midi_event_get_key(event));

    case CONTROL_CHANGE:
        return fluid_synth_cc(synth, chan,
                              fluid_midi_event_get_control(event),
                              fluid_midi_event_get_value(event));

    case PROGRAM_CHANGE:
        return fluid_synth_program_change(synth, chan, fluid_midi_event_get_program(event));

    case CHANNEL_PRESSURE:
        return fluid_synth_channel_pressure(synth, chan, fluid_midi_event_get_program(event));

    case KEY_PRESSURE:
        // return fluid_synth_key_pressure(synth, chan,
        //                                 fluid_midi_event_get_key(event),
        //                                 fluid_midi_event_get_value(event));

    case PITCH_BEND:
        return fluid_synth_pitch_bend(synth, chan, fluid_midi_event_get_pitch(event));

    case MIDI_SYSTEM_RESET:
        return fluid_synth_system_reset(synth);

    case MIDI_SYSEX:
        return fluid_synth_sysex(synth, event->paramptr, event->param1, NULL, NULL, NULL, FALSE);

    case MIDI_TEXT:
    case MIDI_LYRIC:
    case MIDI_SET_TEMPO:
        return FLUID_OK;
    }

    return FLUID_FAILED;
}

// definitions are here https://github.com/FluidSynth/fluidsynth/blob/master/src/midi/fluid_midi.h
