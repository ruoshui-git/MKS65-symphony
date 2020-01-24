
enum server_control_set
{
    /** Pause accepting clients and start (parsing and playing) */
    SERVER_START_PLAYER = 0,
    SERVER_SET_PLAYER = 1,
    SERVER_SEEK_PLAYER = 2,
};

enum player_control
{
    PLAYER_PAUSE = 0,
    PLAYER_RESUME = 1,
    /** Restart playing from beginning */
    PLAYER_RESTART = 2
};

struct server_ctl
{
    enum server_control_set control;
    int value;
};