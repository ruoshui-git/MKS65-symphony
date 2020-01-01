#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

#include "midi.h"
#include "midifile.h"

static FILE *F;

void bindfcns();

int division; /* from the file header */

long tempo = 500000; /* the default tempo is 120 beats/minute */

struct Mfile *mfile = NULL;

int filegetc()
{
    int x;
    x = getc(F);
    return (x);
}

/**
 * @return 0 on success, -1 on failure
*/
int loadfile(const char *file)
{
    F = fopen(file, "r");
    if (!F)
    {
        perror("fopen");
        return -1;
    }
}

/**
 * Handle header in parser.
 * @warning mfile must not be NULL
 */
void handle_header(int format, int ntracks, int division)
{
    mfile->header = make_mheader(division);
}

void handle_track_start()
{
    puts("tstart");
}

void handle_track_end()
{
    puts("tend");
}

/**
 * @param chan Channel
 * @param pitch
 * @param vel Velocity
 */
void handle_noteon(int chan, int pitch, int vel)
{
    
}

void handle_noteoff(int chan, int pitch, int vel)
{
}

void handle_pressure(int chan, int pitch, int pressure)
{
}

void handle_parameter(int chan, int control, int value)
{
}

void handle_pitchbend(int chan, int msb, int lsb)
{
}

void handle_program(int chan, int program)
{
}

void handle_chan_pressure(int chan, int pressure)
{
}

void handle_sysex(int len, const char *msg)
{
}

void handle_metamisc(int type, int len, char *msg)
{
}

/**
 * Sequencer-specific
 */
void handle_metaspecial(int type, int len, char *msg)
{
}

void handle_metatext(int type, int len, char *msg)
{
    static char *ttype[] = {
        NULL,
        "Text Event",       /* type=0x01 */
        "Copyright Notice", /* type=0x02 */
        "Sequence/Track Name",
        "Instrument Name", /* ...       */
        "Lyric",
        "Marker",
        "Cue Point", /* type=0x07 */
        "Unrecognized"};
    int unrecognized = (sizeof(ttype) / sizeof(char *)) - 1;
    register int n, c;
    register char *p = msg;

    if (type < 1 || type > unrecognized)
        type = unrecognized;
    prtime();
    printf("Meta Text, type=0x%02x (%s)  leng=%d\n", type, ttype[type], len);
    printf("     Text = <");
    for (n = 0; n < len; n++)
    {
        c = *p++;
        printf((isprint(c) || isspace(c)) ? "%c" : "\\0x%02x", c);
    }
    printf(">\n");
}

/**
 * @param num Sequence number
 */
void handle_metaseq(int num)
{
}

/**
 * Meta event, end of track
 */
void handle_metaeot()
{
    handle_track_end();
}

/**
 * @param sf Sharps and Flats
 * @param mi Minor
 */
void handle_keysig(int sf, int mi)
{
}

/**
 * Tempo, microseconds-per-MIDI-quarter-note=%ld\n
 */
void handle_tempo(long tempo)
{
}

void handle_timesig(int nn, int dd, int cc, int bb)
{
    int denom = 1;
    while (dd-- > 0)
        denom *= 2;
    printf("Time signature=%d/%d  MIDI-clocks/click=%d  32nd-notes/24-MIDI-clocks=%d\n",
           nn, denom, cc, bb);
}

void handle_smpte(hr, mn, se, fr, ff)
{
    printf("SMPTE, hour=%d minute=%d second=%d frame=%d fract-frame=%d\n",
           hr, mn, se, fr, ff);
}

/**
 * Handle unrecognized byte data
 */
void handle_arbitrary(int len, char *msg)
{

}

/**
 * Meta event 0x21
 */
void port_spec(long channum)
{
    printf("All commands on this track is sent to port (device) %ld\n", channum);
}

void handle_error(char *s)
{

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
    Mf_parameter = handle_parameter;
    Mf_pitchbend = handle_pitchbend;
    Mf_program = handle_program;
    Mf_chanpressure = handle_chan_pressure;
    Mf_sysex = handle_sysex;
    Mf_metamisc = handle_metamisc;
    Mf_seqspecific = handle_metaspecial;
    Mf_seqnum = handle_metaseq;
    Mf_text = handle_metatext;
    Mf_eot = handle_metaeot;
    Mf_timesig = handle_timesig;
    Mf_smpte = handle_smpte;
    Mf_tempo = handle_tempo;
    Mf_keysig = handle_keysig;
    Mf_arbitrary = handle_arbitrary;
    Mf_error = handle_error;

    portspec = port_spec;
}