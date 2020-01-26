#include "midi.h"

/** 
 * Read a midi file and return a malloc'd Mfile representing the midi file
 * @param filepath Path of midi file
 * @return mfile
*/
struct Mfile * Mfile_from_file(char * filepath);