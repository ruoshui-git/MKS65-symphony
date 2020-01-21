#include <stdio.h>
#include <stdlib.h>
#include "midifile_old.h"


int mygetc()
{
    /* use standard input */
    return (getchar());
}

int my_error(char * msg)
{
    printf("Error: %s\n", msg);
}

int main()
{
    Mf_getc = mygetc;
    Mf_error = my_error;
    mfread();
    puts("Done");
    exit(0);
}