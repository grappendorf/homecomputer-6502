#ifndef _VARIABLES_H
#define _VARIABLES_H

#define VAR_TYPE_INTEGER          0
#define VAR_TYPE_STRING           1
#define VAR_FLAG_BUILTIN          0x80
#define VAR_TYPE_MASK             0x0f

#define VAR_PRINT_VALUE   0
#define VAR_PRINT_VERBOSE 1

typedef struct _variable {
  unsigned int name;
  unsigned char type;
  union {
    int integer;
    char *string;
    int *(* builtin_integer) ();
    char *(* builtin_string) ();
  } value;
  struct _variable *next;
} variable;

extern void init_builtin_variables();
extern variable * find_variable(unsigned int name, unsigned char type, variable **prev);
extern void create_variable(unsigned int name, unsigned char type, void *value);
extern void delete_variable(unsigned int name, unsigned char type);
extern void print_variable(variable *v, unsigned char mode);
extern void clear_variables();
extern void print_all_variables();

#endif
