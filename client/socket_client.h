#include <sys/socket.h>

/** 
 * @return sockfd on success, -1 on error
*/
int connect_to(char * server_addr);
int confirm_connection(int sockfd);
int recvall(int sockfd, void *buf, unsigned int *len);
void *get_in_addr(struct sockaddr *sa);