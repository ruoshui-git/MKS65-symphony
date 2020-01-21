#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "midi.h"
#include "midifile.h"

struct Mfile * new_mfile(void)
{
    return calloc(1, sizeof(struct Mfile));
}

/** 
 * @return new empty Mtrack
*/
struct Mtrack * new_mtrack(void)
{
    return calloc(1, sizeof(struct Mtrack));
}


struct Mevent * new_mreg_event(unsigned long curtime, unsigned int type, unsigned int chan, unsigned int param1, unsigned int param2)
{
    struct Mevent * e = calloc(1, sizeof(struct Mevent));
    e->curtime = curtime;
    // e->deltatime, e->next will be set upon append
    e->mtype = MEVENT_REG;

    // now set event data

    struct Mevent_regular * edata = &(e->event_data.regular);
    edata->chan = chan;
    edata->type = type;

    // only 0xc0 and 0xd0 have one param; others all have two
    if (type == program_chng || type == channel_aftertouch)
    {
        edata->len = 1;
    }
    else
    {
        edata->len = 2;
    }

    edata->data[0] = param1;
    edata->data[1] = param2;

    return e;
}


struct Mevent * new_msysex_event(unsigned long curtime, int len, char * msg)
{
    struct Mevent * e = calloc (1, sizeof(struct Mevent));
    e->curtime = curtime;
    e->mtype = MEVENT_SYSEX;
    
    struct Mevent_sysex * edata = &(e->event_data.sysex);
    edata->len = len;
    if (msg)
        edata->msg = strdup(msg);
    return e;
}

struct Mevent * new_mmeta_event(unsigned long curtime, unsigned char type, unsigned long len, char * data)
{
    struct Mevent * e = calloc(1, sizeof(struct Mevent));
    e->curtime = curtime;
    e->mtype = MEVENT_META;
    struct Mevent_meta * edata = &(e->event_data.meta);
    edata->len = len;
    edata->type = type;

    if (len == 0)
    {
        return e;
    }
    
    // copy data into a malloc'd buffer
    char * data_local = malloc(len);
    memcpy(data_local, data, len);

    // reverse copying isn't needed since all are converted to bytes
    // if (type == sequence_number)
    // {
    //     reverse_memcpy(data_local, data, len);
    // }
    // else
    // {
    //     memcpy(data_local, data, len);
    // }
    
    edata->data = data_local;
    return e;
}

void append_mevent(struct Mtrack * track, struct Mevent * e)
{
    // puts("adding new event");
    if (track->len == 0)
    {
        // first element
        track->first = e;
        track->last = e;
        track->len = 1;
        // delta-time will just be 0
    }
    else
    {
        // set delta-time
        e->deltatime = e->curtime - track->last->curtime;

        // append e
        track->last = track->last->next = e;
        track->len++;
    }
    // puts("event ending");
}


// utility functions

/** 
 * Copy n bytes from src to dest, from right to left
 * https://stackoverflow.com/a/2242531/11993619
*/
void reverse_memcpy(char * restrict dest, char * restrict src, size_t n)
{
    size_t i;
    for (i = 0; i < n; i++)
    {
        dest[n - 1 - i] = src[i];
    }
}