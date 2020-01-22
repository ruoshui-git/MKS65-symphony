#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <assert.h>

#include "midi.h"
#include "midifile.h"
#include "utils.h"


// private headers
int filegetc();
int loadfile(char *file);
void unloadfile(void);
void handle_header(int format, int ntracks, int division);
void handle_track_start();
void handle_track_end();
void handle_noteon(int chan, int pitch, int vel);
void handle_noteoff(int chan, int pitch, int vel);
void handle_pressure(int chan, int pitch, int pressure);
void handle_control_change(int chan, int control, int value);
void handle_pitchbend(int chan, int msb, int lsb);
void handle_program(int chan, int program);
void handle_chan_pressure(int chan, int pressure);
void handle_sysex(int len, char *msg);
void handle_metamisc(int type, int len, char *msg);
void handle_metaspecial(int len, char *msg);
void handle_metatext(int type, int len, char *msg);
void handle_metaeot();

void handle_metaseq(int num);
void handle_keysig(int sf, int mi);
void handle_tempo(long tempo);
void handle_timesig(int nn, int dd, int cc, int bb);
void handle_smpte(int hr, int mn, int se, int fr, int ff);

void my_handle_metaseq(char * data);
void my_handle_keysig(char * data);
void my_handle_tempo(char * data);
void my_handle_timesig(char * data);
void my_handle_smpte(char * data);

void handle_arbitrary(int len, char * msg);
void port_spec(long channum);
void handle_error(char *s);
void bindfcns(void);
// end of headers

static FILE *F;

int fcns_bound = 0;

int division; /* from the file header */

long tempo = 500000; /* the default tempo is 120 beats/minute */

static struct Mfile *mfile = NULL;

struct Mfile * Mfile_from_file(char * filepath)
{
    if (!fcns_bound)
    {
        bindfcns();
    }

    if (loadfile(filepath) == -1)
    {
        sys_error("Failed to load midi file");
        return NULL;
    }

    mfread();

    unloadfile();

    return mfile;
}

int filegetc()
{
    return fgetc(F);
}

/**
 * @param file File path
 * @return 0 on success, -1 on failure
*/
int loadfile(char *file)
{
    F = fopen(file, "r");
    if (!F)
    {
        perror("fopen");
        return -1;
    }

    mfile = new_mfile();

    // add filename
    char * filename = strrchr(file, '/');
    if (!filename) // there's no '/' in file path
        filename = file;
    mfile->filename = strdup(filename);

    return 0;
}

/**
 * Handle header in parser.
 * @warning mfile must not be NULL
 */
void handle_header(int format, int ntracks, int division)
{
    assert(mfile != NULL);

    mfile->division = division;
    mfile->format = format;
    mfile->ntracks = ntracks;

    mfile->tracks = calloc(ntracks, sizeof(struct Mtrack *));

}

/** 
 * Initialize a new track and point last_track field of Mfile struct to the last track
*/
void handle_track_start()
{
    assert(mfile != NULL);

    mfile->cur_track = new_mtrack();

    // puts("tstart");
}

void handle_track_end()
{
    // detect tempo track
    if (! (mfile->has_tempotrack))
    {
        // the tempo track will be the first track of a format 1 midi file
        if (mfile->format == 1 && mfile->cur_track_index == 0)
        {
            struct Mtrack * cur_track = mfile->cur_track;
            cur_track->is_tempotrack = 1;
            mfile->has_tempotrack = 1;
            if (cur_track->name)
                mfile->music_name = strdup(cur_track->name);
            if (cur_track->text)
                mfile->info_text = strdup(cur_track->text);
        }
    }

    // append current track and increment track index
    mfile->tracks[mfile->cur_track_index++] = mfile->cur_track;
    mfile->cur_track = NULL;

    assert(mfile->cur_track_index <= mfile->ntracks);

    // puts("tend");
}

/**
 * @param chan Channel
 * @param pitch
 * @param vel Velocity
 */
void handle_noteon(int chan, int pitch, int vel)
{
    append_mevent(mfile->cur_track, new_mreg_event(Mf_currtime, note_on, chan, pitch, vel));
}

void handle_noteoff(int chan, int pitch, int vel)
{
    append_mevent(mfile->cur_track, new_mreg_event(Mf_currtime, note_off, chan, pitch, vel));
}

void handle_pressure(int chan, int pitch, int pressure)
{
    append_mevent(mfile->cur_track, new_mreg_event(Mf_currtime, poly_aftertouch, chan, pitch, pressure));
}

void handle_control_change(int chan, int control, int value)
{
    append_mevent(mfile->cur_track, new_mreg_event(Mf_currtime, control_change, chan, control, value));
}

void handle_pitchbend(int chan, int msb, int lsb)
{
    // order of lsb and msb: https://ccrma.stanford.edu/~craig/articles/linuxmidi/misc/essenmidi.html
    append_mevent(mfile->cur_track, new_mreg_event(Mf_currtime, pitch_wheel, chan, lsb, msb));
}

void handle_program(int chan, int program)
{
    append_mevent(mfile->cur_track, new_mreg_event(Mf_currtime, program_chng, chan, program, 0));
}

void handle_chan_pressure(int chan, int pressure)
{
    append_mevent(mfile->cur_track, new_mreg_event(Mf_currtime, channel_aftertouch, chan, pressure, 0));
}

void handle_sysex(int len, char *msg)
{
    append_mevent(mfile->cur_track, new_msysex_event(Mf_currtime, len, msg));
}

void handle_metamisc(int type, int len, char *msg)
{
    append_mevent(mfile->cur_track, new_mmeta_event(Mf_currtime, type, len, msg));
}


/**
 * type == 0x00
 * @param num Sequence number
 */
void handle_metaseq(int num)
{}
void my_handle_metaseq(char * data)
{
    append_mevent(mfile->cur_track, new_mmeta_event(Mf_currtime, sequence_number, 2, data));
}

/** 
 * Track text is handled here
*/
void handle_metatext(int type, int len, char *msg)
{
    struct Mtrack * cur_track = mfile->cur_track;
    append_mevent(cur_track, new_mmeta_event(Mf_currtime, type, len, msg));
    
    char * str = malloc(len + 1);
    memcpy(str, msg, len);
    str[len] = '\0';

    switch (type)
    {
    case text_event:
        if (!cur_track->text)
            cur_track->text = str;
        break;
    case copyright_notice:
        if (!mfile->copyright_info)
            mfile->copyright_info = str;
        break;
    case sequence_name:
        if (!cur_track->name)
            cur_track->name = str;
        break;
    case instrument_name:
        if (!cur_track->instrument)
            cur_track->instrument = str;
        break;
    default:
        break;
    }

    // static char *ttype[] = {
    //     NULL,
    //     "Text Event",       /* type=0x01 */
    //     "Copyright Notice", /* type=0x02 */
    //     "Sequence/Track Name",
    //     "Instrument Name", /* ...       */
    //     "Lyric",
    //     "Marker",
    //     "Cue Point", /* type=0x07 */
    //     "Unrecognized"};
    // int unrecognized = (sizeof(ttype) / sizeof(char *)) - 1;
    // register int n, c;
    // register char *p = msg;

    // if (type < 1 || type > unrecognized)
    //     type = unrecognized;
    // prtime();
    // printf("Meta Text, type=0x%02x (%s)  leng=%d\n", type, ttype[type], len);
    // printf("     Text = <");
    // for (n = 0; n < len; n++)
    // {
    //     c = *p++;
    //     printf((isprint(c) || isspace(c)) ? "%c" : "\\0x%02x", c);
    // }
    // printf(">\n");
}


/**
 * Meta event, end of track
 */
void handle_metaeot()
{
    append_mevent(mfile->cur_track, new_mmeta_event(Mf_currtime, end_of_track, 0, NULL));
}


/**
 * Tempo, microseconds-per-MIDI-quarter-note=%ld\n
 */
void handle_tempo(long tempo)
{
    // printf("temple: %ld\n", tempo);
}
void my_handle_tempo(char * data) //long tempo)
{
    append_mevent(mfile->cur_track, new_mmeta_event(Mf_currtime, set_tempo, 3, data));
}

void handle_smpte(int hr, int mn, int se, int fr, int ff)
{}
void my_handle_smpte(char * data)
{
    // I don't care about the details
    append_mevent(mfile->cur_track, new_mmeta_event(Mf_currtime, smpte_offset, 5, data));
    // printf("SMPTE, hour=%d minute=%d second=%d frame=%d fract-frame=%d\n",
    //        hr, mn, se, fr, ff);
}

/**
 * 
 */
void handle_timesig(int nn, int dd, int cc, int bb)
{}
void my_handle_timesig(char * data)
{
    append_mevent(mfile->cur_track, new_mmeta_event(Mf_currtime, time_signature, 4, data));
    // int denom = 1;
    // while (dd-- > 0)
    //     denom *= 2;
    // printf("Time signature=%d/%d  MIDI-clocks/click=%d  32nd-notes/24-MIDI-clocks=%d\n",
    //        nn, denom, cc, bb);
}

/**
 * @param sf Sharps and Flats
 * @param mi Minor
 */
void handle_keysig(int sf, int mi)
{}
void my_handle_keysig(char * data)
{
    append_mevent(mfile->cur_track, new_mmeta_event(Mf_currtime, key_signature, 2, data));
}

/**
 * Sequencer-specific
 */
void handle_metaspecial(int len, char *msg)
{
    append_mevent(mfile->cur_track, new_mmeta_event(Mf_currtime, sequencer_specific, len, msg));
    // ignore this for now
}

/**
 * Handle unrecognized byte data
 */
void handle_arbitrary(int len, char *msg)
{
}

/**
 * Meta event 0x21 not being used now
 */
void port_spec(long channum)
{
    // printf("All commands on this track is sent to port (device) %ld\n", channum);
}

void handle_error(char *s)
{
    parser_error(s);
    unloadfile();
    exit(1);
}

void bindfcns(void)
{

    Mf_getc = filegetc;
    Mf_header = handle_header;
    Mf_trackstart = handle_track_start;
    Mf_trackend = handle_track_end;
    Mf_noteon = handle_noteon;
    Mf_noteoff = handle_noteoff;
    Mf_pressure = handle_pressure;
    Mf_parameter = handle_control_change;
    Mf_pitchbend = handle_pitchbend;
    Mf_program = handle_program;
    Mf_chanpressure = handle_chan_pressure;
    Mf_sysex = handle_sysex;
    Mf_metamisc = handle_metamisc;
    Mf_seqspecific = handle_metaspecial;
    Mf_text = handle_metatext;
    Mf_eot = handle_metaeot;


    // don't bind these functions to prevent program from using them
    // Mf_seqnum = handle_metaseq;
    // Mf_timesig = handle_timesig;
    // Mf_smpte = handle_smpte;
    // Mf_tempo = handle_tempo;
    // Mf_keysig = handle_keysig;

    My_seqnum = my_handle_metaseq;
    My_timesig = my_handle_timesig;
    My_smpte = my_handle_smpte;
    My_tempo = my_handle_tempo;
    My_keysig = my_handle_keysig;

    Mf_arbitrary = handle_arbitrary;
    Mf_error = handle_error;


}

void unloadfile(void)
{
    fclose(F);
}

/** 
 * Check mfile representation
*/
int check_rep()
{
    return mfile != NULL;
}