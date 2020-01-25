
/** 
 * Setup listening server
 * @return socket descriptor
*/
int setup_server(void);

/** 
  * Run server on @param sockfd, accepting commands from @param control_fd
*/
pthread_t run_server(int sockfd, int control_fd);