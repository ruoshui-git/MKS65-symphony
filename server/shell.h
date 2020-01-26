#ifndef SHELL_H
#define SHELL_H
struct cmd
{
    char * name;
    char * type;
    char * desc;
    void (*fn)();
};

#endif