void parser_error(char * msg);
void writer_error(char * msg);
void sys_error(char * msg);
void sys_warning(char * msg);
/** 
 * If s is not NULL, then strdup; else returns NULL
*/
char *cstrdup(const char *s);