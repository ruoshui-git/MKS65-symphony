// represents a midi file
struct Mfile
{
    int format;
    struct Mheader * header;
    int ntracks;
    struct Mtrack * tracks [];
};

struct Mheader
{
    int type; // 0 = delta-time / quarter note; 1 = delta-time / SMTPE frame
    
    int ppqn;
    int frames_per_sec;
    int resolution;
};

struct Mtrack
{
    struct Mevent * first;
    struct Mevent * last;
    int len;
};

struct Mevent
{

};

struct Mfile * make_mfile(void);
struct Mheader * make_mheader(int division);