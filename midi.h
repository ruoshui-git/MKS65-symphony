#ifndef MIDI_H
#define MIDI_H

// represents a midi file
struct Mfile
{
    int format; // 0, 1, 2
    int division;

    // whether file has tempo track or not
    int has_tempotrack;

    int ntracks;
    // array of tracks, len == ntracks
    struct Mtrack ** tracks;

    // used for reading a midi file
    struct Mtrack * cur_track;
    int cur_track_index;

    // info about music
    char * music_name;
    char * info_text;
    char * copyright_info;

};

// struct Mheader
// {
//     int type; // 0 = delta-time / quarter note; 1 = delta-time / SMTPE frame
    
//     int ppqn;
//     int frames_per_sec;
//     int resolution;
// };

struct Mtrack
{
    int is_tempotrack;
    struct Mevent * first;
    struct Mevent * last;
    int len;

    // string info about a track
    char * name;
    char * text;
    char * instrument;
};

/** Defines the type of event in this program */
enum Mtype
{
    MEVENT_REG = 0,
    MEVENT_SYSEX = 1,
    MEVENT_META = 2
};


struct Mevent_regular
{
    /* event type, defined by midi spec */
    int type;
    int chan;
    char data[2];
    int len;
};

struct Mevent_sysex
{
    char * msg;
    int len;
};

struct Mevent_meta
{
    /* event type, defined by midi spec */
    int type;
    int len;
    char * data;
};

union Mevent_data
{
    struct Mevent_regular regular;
    struct Mevent_sysex sysex;
    struct Mevent_meta meta;
};

struct Mevent
{
    /* 
     * Type of event, defined in this program
    */
    enum Mtype mtype;
    /* delta time exists in every kind of event, so it's put in here, not every specific type struct */
    unsigned long deltatime;
    /* used for calculating deltatime */
    unsigned long curtime;

    /* actual event data */
    union Mevent_data event_data;

    struct Mevent * next;
};

/** 
 * @return new empty Mfile
 * 
*/
struct Mfile * new_mfile(void);

/** 
 * @param division A field in midi file header chunk
 * @return new Mheader
*/
// struct Mheader * new_mheader(int division);

/** 
 * @return new empty Mtrack
*/
struct Mtrack * new_mtrack(void);

/** 
 * Make a new regular event
 * @param curtime Current time, in delta-time
 * @param type Type of event, defined by midi spec
 * @param chan Channel if regular event
 * @param param1 Parameter 1 of regular message
 * @param param2 Parameter 2 of regular message
 * @return new Mevent
*/
struct Mevent * new_mreg_event(unsigned long curtime, unsigned int type, unsigned int chan, unsigned int param1, unsigned int param2);

/** 
 * Make a new system exclusive event
 * @param curtime Current time in delta-time
 * @param len Length of message
 * @param msg Sysex message
 * @return new system exclusive event
*/
struct Mevent * new_msysex_event(unsigned long curtime, int len, char * msg);

/** 
 * Make a new meta event
 * @param curtime Current time in delta-time
 * @param type Type of meta event, as defined by midi spec
 * @param len Length of event data
 * @param data Data of event; must be left-justified
*/
struct Mevent * new_mmeta_event(unsigned long curtime, unsigned char type, unsigned long len, char * data);

/**
 * Append current midi event to track
 * @param track Track
 * @param e Event
*/
void append_mevent(struct Mtrack * track, struct Mevent * e);

#endif // MIDI_H

void reverse_memcpy(char * restrict dest, char * restrict src, size_t n);