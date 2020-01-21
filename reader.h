#include "midi.h"
struct Mfile * Mfile_from_file(char * filepath);
int filegetc();
int loadfile(const char *file);
void unloadfile(void);
void handle_header(int format, int ntracks, int division);
void handle_track_start();
void handle_track_end();
void handle_noteon(int chan, int pitch, int vel);
void handle_noteoff(int chan, int pitch, int vel);
void handle_pressure(int chan, int pitch, int pressure);
void handle_control_change(int chan, int control, int value);
void handle_pitchbend(int chan, int msb, int lsb);
void handle_program(int chan, int program);
void handle_chan_pressure(int chan, int pressure);
void handle_sysex(int len, char *msg);
void handle_metamisc(int type, int len, char *msg);
void handle_metaspecial(int len, char *msg);
void handle_metatext(int type, int len, char *msg);
void handle_metaeot();

void handle_metaseq(int num);
void handle_keysig(int sf, int mi);
void handle_tempo(long tempo);
void handle_timesig(int nn, int dd, int cc, int bb);
void handle_smpte(int hr, int mn, int se, int fr, int ff);

void my_handle_metaseq(char * data);
void my_handle_keysig(char * data);
void my_handle_tempo(char * data);
void my_handle_timesig(char * data);
void my_handle_smpte(char * data);

void handle_arbitrary(int len, char * msg);
void port_spec(long channum);
void handle_error(char *s);
void bindfcns(void);





