void parser_error(char * msg);
void writer_error(char * msg);
void sys_error(char * msg);
void sys_warning(char * msg);
/** 
 * If s is not NULL, then strdup; else returns NULL
*/
char *cstrdup(const char *s);


/** 
 * Count number of c in str
*/
int count_char(char *str, char c);

/** 
 * Parse a line and return an argv representing words
 * @param line String to be parsed
 * @param len_ptr Will be set to number of words
 * @return A malloc'd array of words
*/
char **parse_line(char *line, int *len_ptr);

/** @return max of a and b */
int max(int a, int b);
