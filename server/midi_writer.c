
#include <stdio.h>
#include <stdlib.h>

#include "midi.h"
#include "midifile.h"
#include "utils.h"


// private stuff
static int load_out_file(char * filepath);
static void load_out_src(struct Mfile * mf);
static void unload_out_file();
int my_putc(char c);
int handle_write_track(int track_index);
int handle_write_tempotrack();
void handle_out_error(char * msg);
void write_track(struct Mtrack * track);
void write_event(struct Mevent * e);
void Mfile_write_to_midi(struct Mfile * m, char * filepath);
void bind_out_fncs(void);


static struct Mfile * mfile = NULL;
static FILE * fout = NULL;


static void load_out_src(struct Mfile * mf)
{
    mfile = mf;
}

static int load_out_file(char * filepath)
{
    if (!(fout = fopen(filepath, "w")))
    {
        perror("fopen");
        return -1;
    }
}

static void unload_out_file()
{
    fclose(fout);
}

int my_putc(char c)
{
    return fputc(c, fout);
}

int handle_write_track(int track_index)
{
    // if (mfile->has_tempotrack)
    // {
    //     track_index++;
    // }

    write_track(mfile->tracks[track_index]);
}

int handle_write_tempotrack()
{
    if (!mfile->has_tempotrack)
        return -1;
    struct Mtrack * tempotrack = mfile->tracks[0];

    write_track(mfile->tracks[0]);
    return 0;
}

void handle_out_error(char * msg)
{
    writer_error(msg);
    unload_out_file();
    exit(1);
}

void write_track(struct Mtrack * track)
{
    struct Mevent * e = track->first;
    while (e)
    {
        write_event(e);
        e = e->next;
    }
}

void write_event(struct Mevent * e)
{
    switch (e->mtype)
    {
        case MEVENT_META: ;
            struct Mevent_meta * meta = &(e->event_data.meta);
            mf_write_meta_event(e->deltatime, meta->type, meta->data, meta->len);
            break;
        case MEVENT_REG: ;
            struct Mevent_regular * reg = &(e->event_data.regular);
            mf_write_midi_event(e->deltatime, reg->type, reg->chan, reg->data, reg->len);
            break;
        case MEVENT_SYSEX: ;
            struct Mevent_sysex * sysex = &(e->event_data.sysex);
            mf_write_midi_event(e->deltatime, system_exclusive, 0, sysex->msg, sysex->len);
            break;
        default:
            writer_error("Unrecognized event");
    }
}

void Mfile_write_to_midi(struct Mfile * m, char * filepath)
{
    load_out_src(m);
    load_out_file(filepath);
    bind_out_fncs();

    mfwrite(mfile->format, mfile->ntracks, mfile->division, fout);
}

void bind_out_fncs(void)
{
    // Mf_writetempotrack = handle_write_tempotrack;
    Mf_writetrack = handle_write_track;
    Mf_putc = my_putc;
    Mf_error = handle_out_error;
}