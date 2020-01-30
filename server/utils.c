#include <stdio.h>
#include <string.h>

void parser_error(char * msg)
{
    fprintf(stderr, "Parser error: %s\n", msg);
}

void sys_error(char * msg)
{
    fprintf(stderr, "System error: %s\n", msg);
}

void writer_error(char * msg)
{
    fprintf(stderr, "Writer error: %s\n", msg);
}

void sys_warning(char * msg)
{
    fprintf(stderr, "Warning: %s\n", msg);
}

/** 
 * If s is not NULL, then strdup; else returns NULL
*/
char *cstrdup(const char *s)
{
    return s ? strdup(s) : NULL;
}