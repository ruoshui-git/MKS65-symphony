
#ifndef SHELL_H
#define SHELL_H
struct cmd
{
    char * name;
    int num_args;
    char * desc;
    int (*fn)();
};

#endif // SHELL_H

/** 
 * Setup listening server
 * @return socket descriptor
*/
int setup_server(void);

/** Clean up shell and exit */
void shell_exit();