#ifndef _BASIC_H
#define _BASIC_H

extern void basic_init();
extern void interpret(char * s);
extern void syntax_error();
extern void syntax_error_msg_with_arg(const char *msg, const char *msg_arg);
#define syntax_error_msg(s) syntax_error_msg_with_arg(s, NULL)
extern char print_buffer[];

#endif
