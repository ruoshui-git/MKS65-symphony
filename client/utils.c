#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "utils.h"

void parser_error(char *msg)
{
    fprintf(stderr, "Parser error: %s\n", msg);
}

void sys_error(char *msg)
{
    fprintf(stderr, "System error: %s\n", msg);
}

void writer_error(char *msg)
{
    fprintf(stderr, "Writer error: %s\n", msg);
}

void sys_warning(char *msg)
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

char **parse_line(char *line, int *len_ptr)
{
    // malloc the size of the number of words, separated by ' '
    *len_ptr = count_char(line, ' ') + 1;
    char **args = malloc(sizeof(char *) * (*len_ptr));

    if (args == NULL)
    {
        perror("malloc");
        exit(1);
    }
    int i = 0;
    char *sep = " ";
    args[i++] = strtok(line, sep);
    char *token = strtok(NULL, sep);
    while (token != NULL)
    {
        args[i++] = token;
        token = strtok(NULL, sep);
    }
    return args;
}

int count_char(char *str, char c)
{
    int i, sum, len;
    for (i = 0, sum = 0, len = strlen(str); i < len; i++)
    {
        if (str[i] == c)
        {
            sum++;
        }
    }
    return sum;
}

int max(int a, int b)
{
    return a > b ? a : b;
}