#define MIDI_SOUNDFONT "../concerto-in-d-minor-mids/timbres_of_heaven.sf2"

void player_setup();
void player_add_midi_file(char* path);
void player_clear_midi_files();
void player_play();
void player_pause();
void player_seek(int tick);
void player_setloop(int looping);
