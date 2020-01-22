#include <stdio.h>
#include <string.h>

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

void sys_warning(char * msg)
{
    printf("Warning: %s\n");
}

/** 
 * If s is not NULL, then strdup; else returns NULL
*/
char *cstrdup(const char *s)
{
    return s ? strdup(s) : NULL;
}