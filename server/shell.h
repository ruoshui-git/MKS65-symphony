#ifndef SHELL_H
#define SHELL_H
struct cmd
{
    char * name;
    int num_args;
    char * desc;
    int (*fn)();
};

#endif