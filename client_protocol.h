#ifndef CLIENT_PROTOCOL_H
#define CLIENT_PROTOCOL_H

enum client_protocol
{
    CLIENT_PLAY = 0,
    CLIENT_PAUSE = 1,
    CLIENT_RESUME = 2,
    CLIENT_SEEK = 3, // followed by another 4 byte with a tick value
    CLIENT_LOOP = 4,
    CLIENT_NOLOOP = 5,
    CLIENT_NFILE = 6, // new file
    CLIENT_QUIT = 7,
    // Response code
    CLIENT_READY = 10
};

#endif