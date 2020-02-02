
// Handler declarations
int handle_load(char *filepath);
int handle_play();
int handle_pause();
int handle_resume();
int handle_restart();
int handle_loop();
int handle_noloop();
int handle_seek(int value);
int handle_status();

/** 
 * @return 2 to signal quitting
*/
int handle_quit();
int handle_reconnect();
int handle_help();

void setup_connections();
void clear_connections();

// 
int handle_socket(int sockfd);



/** Write out the midi files to the tmp folder */
void create_midi_files();


/** Make tmp dir if it doesn't exist */
void make_tmp_dir();