/*
  An example of how to use FluidSynth.
  To compile it on Linux:
  $ gcc -o example example.c `pkg-config fluidsynth --libs`
  To compile it on Windows:
    ...
  Author: Peter Hanappe.
  This code is in the public domain. Use it as you like.
*/
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <string.h>

#include <fluidsynth.h>

int main(int argc, char **argv)
{
    fluid_settings_t *settings;
    fluid_synth_t *synth;
    fluid_audio_driver_t *adriver;
    int sfont_id;
    int i, key;
    /* Create the settings. */
    settings = new_fluid_settings();
    /* Change the settings if necessary*/
    /* Create the synthesizer. */
    fluid_settings_setstr(settings, "audio.driver", "pulseaudio");

    synth = new_fluid_synth(settings);

    
    /* Create the audio driver. The synthesizer starts playing as soon
       as the driver is created. */
    // adriver = new_fluid_audio_driver(settings, synth);

    /* Load a SoundFont and reset presets (so that new instruments
     * get used from the SoundFont) */
    sfont_id = fluid_synth_sfload(synth, "timbres_of_heaven.sf2", 1);
    /* Initialize the random number generator */
    srand(getpid());
    for(i = 0; i < 12; i++)
    {
        /* Generate a random key */
        key = 60 + (int)(12.0f * rand() / (float) RAND_MAX);
        /* Play a note */
        fluid_synth_noteon(synth, 0, key, 80);
        /* Sleep for 1 second */
        sleep(0.5);
        /* Stop the note */
        fluid_synth_noteoff(synth, 0, key);
    }

    enum { SAMPLES = 512 };

    // USECASE4: multi-channel rendering, i.e. render all audio and effects channels to dedicated audio buffers
    // ofc it‘s not a good idea to allocate all the arrays on the stack
    {
        // lookup number of audio and effect (stereo-)channels of the synth
        // see "synth.audio-channels", "synth.effects-channels" and "synth.effects-groups" settings respectively
        int n_aud_chan = fluid_synth_count_audio_channels(synth);
        
        printf("num_channels: %d\n", n_aud_chan);

        // by default there are two effects stereo channels (reverb and chorus) ...
        int n_fx_chan = 0 ;// fluid_synth_count_effects_channels(synth);
        
        // ... for each effects unit. Each unit takes care of the effects of one MIDI channel.
        // If there are less units than channels, it wraps around and one unit may render effects of multiple
        // MIDI channels.
        // n_fx_chan *= fluid_synth_count_effects_groups();

        // for simplicity, allocate one single sample pool
        float samp_buf[SAMPLES * (n_aud_chan + n_fx_chan) * 2];
        // array of buffers used to setup channel mapping
        float *dry[n_aud_chan * 2], *fx[n_fx_chan * 2];
        // setup buffers to mix dry stereo audio to
        // buffers are alternating left and right for each n_aud_chan,
        // please review documentation of fluid_synth_process()
        for(int i = 0; i < n_aud_chan * 2; i++)
        {
            dry[i] = &samp_buf[i * SAMPLES];
        }
        // setup buffers to mix effects stereo audio to
        // similar channel layout as above, revie fluid_synth_process()
        // for(int i = 0; i < n_fx_chan * 2; i++)
        // {
        //     fx[i] = &samp_buf[n_aud_chan * 2 * SAMPLES + i * SAMPLES];
        // }
        
        // dont forget to zero sample buffer(s) before each rendering
        memset(samp_buf, 0, sizeof(samp_buf));
        int err = fluid_synth_process(synth, SAMPLES, 0, NULL, n_aud_chan * 2, dry);
        if(err == FLUID_FAILED)
        {
            puts("oops");
        }
    }

    /* Clean up */
    // delete_fluid_audio_driver(adriver);
    delete_fluid_synth(synth);
    delete_fluid_settings(settings);
    return 0;
}