#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include "midi.h"
#include "utils.h"

/** 
 * Private, helper
 * Give an array, stating how many tracks in src should a dest track contain.
 * Useful for mixing tracks.
 * Ex: 3 computers playing 5 tracks, this function will return an array with len 3,
 *   with values [2, 2, 1], meaning first computer (dest) will play 2 tracks, 
 *   second will play 2 tracks, third will play 1 track
 * Not useful if there are more computers than tracks. In that case, just loop around 
 * computers and send each one a different track.
*/
int* track_num_map(int src_len, int dest_len);

struct Mfile ** Mfile_split_by_tracks(struct Mfile * mfile, int ndest)
{
    if (mfile->format == 0)
    {
        sys_warning("Cannot split midi file of type 0 by tracks. Try splitting by channels");
        return NULL;
    }

    struct Mfile ** mfiles = calloc(ndest, sizeof(struct Mfile *));

    int src_ntracks = mfile->ntracks;
    int format = mfile->format;
    int nsrc = src_ntracks;
    if (format == 1)
        nsrc --;
    int division = mfile->division;
    char * filename = mfile->filename;
    char * music_name = mfile->music_name;
    char * info_text = mfile->info_text;
    char * cp_info = mfile->copyright_info;

    int i;

    if (ndest >= nsrc)
    {
        // more computers than tracks, each get one track
        struct Mfile * dmfile;
        for (i = 0; i < ndest; i++)
        {
            dmfile = mfiles[i] = Mfile_from_headers(format, division, filename, music_name, info_text, cp_info);

            // copy tracks
            if (format == 1)
            {

                dmfile->ntracks = 2;
                dmfile->has_tempotrack = 1;
                dmfile->tracks = malloc(dmfile->ntracks * sizeof(struct Mtrack *));
                
                // copy tempo map
                int src_i = (i % nsrc) + 1;
                assert(src_i < src_ntracks);
                dmfile->tracks[0] = Mtrack_copy(mfile->tracks[0]);
                dmfile->tracks[1] = Mtrack_copy(mfile->tracks[src_i]);
            }
            else
            {
                // format == 2
                dmfile->ntracks = 1;
                dmfile->has_tempotrack = 0;

                int src_i = i % nsrc + 1;
                dmfile->tracks = malloc(dmfile->ntracks * sizeof(struct Mtrack *));
                dmfile->tracks[0] = Mtrack_copy(mfile->tracks[src_i]);
            }
            
        }
    }
    else
    {
        // mix some tracks, since less computer than tracks
        int* dest_map = track_num_map(nsrc, ndest);

        struct Mfile * dmfile;
        for (i = 0; i < ndest; i++)
        {
            dmfile = mfiles[i] = Mfile_from_headers(format, division, filename, music_name, info_text, cp_info);
            
            if (format == 1)
            {
                dmfile->ntracks = dest_map[i] + 1;
                dmfile->tracks = malloc(dmfile->ntracks * sizeof(struct Mtrack *));
                dmfile->tracks[0] = Mtrack_copy(mfile->tracks[0]);
                

                for (int dtrack_i = 0, dest_total = dest_map[i]; dtrack_i < dest_total; dtrack_i++)
                {
                    int src_i = i + dtrack_i * ndest + 1;
                    assert(src_i < src_ntracks);
                    dmfile->tracks[dtrack_i + 1] = Mtrack_copy(mfile->tracks[src_i]);
                }

            }
            else
            {
                // format == 2
                dmfile->ntracks = dest_map[i];
                dmfile->tracks = malloc(dmfile->ntracks * sizeof(struct Mtrack *));

                for (int dtrack_i = 0, dest_total = dest_map[i]; dtrack_i < dest_total; dtrack_i++)
                {
                    int src_i = i + dtrack_i * ndest;
                    assert(src_i < src_ntracks);
                    dmfile->tracks[dtrack_i] = Mtrack_copy(mfile->tracks[src_i]);
                }
            }
            
        }
    }

    return mfiles;
}

int* track_num_map(int src_len, int dest_len)
{
    int * map = malloc(dest_len);
    int base = src_len / dest_len; // int division
    int additional = src_len % dest_len;
    int i;
    for (i = 0; i < dest_len; i++)
    {
        map[i] = base;
        if (additional > 0)
        {
            map[i]++;
            additional--;
        }
    }
    return map;
}