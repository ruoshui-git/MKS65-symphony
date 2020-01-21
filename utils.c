#include <stdio.h>

void parser_error(char * msg)
{
    printf("Parser error: %s\n", msg);
}

void sys_error(char * msg)
{
    printf("System error: %s\n", msg);
}

void writer_error(char * msg)
{
    printf("Writer error: %s\n", msg);
}