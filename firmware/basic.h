#ifndef _BASIC_H
#define _BASIC_H

extern void basic_init();
extern void interpret(char * s);
extern void syntax_error();
extern void syntax_error_msg(char *msg);
extern char *parse_integer(char *s, int *value);
extern char print_buffer[];

#endif
