#include <stdio.h>

#include "reader.h"
#include "writer.h"
#include "utils.h"

#define MIDI_FILE "beethoven_symphony_5.mid"
// #define MIDI_FILE "exp/midifile/mozart40.mid"
// #define MIDI_FILE "out.mid"

#define OUT_FILE "out.mid"

int main (void)
{
    
    struct Mfile * mfile = Mfile_from_file(MIDI_FILE);

    
    Mfile_write_to_midi(mfile, OUT_FILE);

    return 0;
}