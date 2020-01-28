
#ifndef SERVER_H
#define SERVER_H
enum server_control_set
{
    /** Pause accepting clients and start (parsing and playing) */
    SERVER_START_PLAYER = 0,
    // Requires value to be one of enum player_control
    SERVER_SET_PLAYER = 1,
    // Requires vavlue to be a possible value for seeking
    SERVER_SEEK_PLAYER = 2,
    SERVER_RECONNECT = 3,
    SERVER_QUIT = 4,
    SERVER_PRINT_STATUS = 5
};

enum player_control
{
    PLAYER_PAUSE = 0,
    PLAYER_RESUME = 1,
    /** Restart playing from beginning */
    PLAYER_RESTART = 2,
    PLAYER_LOOP = 3,
    PLAYER_NOLOOP = 4,
};

struct server_ctl
{
    enum server_control_set control;
    int value;
};

#endif

/** 
 * Setup listening server
 * @return socket descriptor
*/
int setup_server(void);

/** 
  * Run server on @param sockfd, accepting commands from @param control_fd
*/
pthread_t run_server(int sockfd, int control_fd);