#include <stdio.h>

#include "reader.h"
#include "utils.h"

// #define MIDI_FILE "beethoven_symphony_5.mid"
#define MIDI_FILE "exp/midifile/mozart40.mid"

#define OUT_FILE "out.mid"

int main (void)
{
    
    struct Mfile * mfile = Mfile_from_file(MIDI_FILE);

    return 0;
}