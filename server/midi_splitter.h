#include "midi.h"


/** 
 * Split a Mfile by tracks. 
 * @warning Does not support format 0 since it has only one track
*/
struct Mfile ** Mfile_split_by_tracks(struct Mfile * mfile, int ndest);
