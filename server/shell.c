#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>

// #include <readline/readline.h>
// #include <readline/history.h>

#include <fluidsynth.h>

#include "server.h"
#include "midi.h"
#include "utils.h"
#include "shell.h"
#include "midi_reader.h"




/** 
 * Write struct server_ctl @param server_control and @param value to @param server_fd
 * @return -1 on error, 0 on success
*/
int write_to_server(enum server_control_set server_control, int value);

// free stuff
static void cleanup();





