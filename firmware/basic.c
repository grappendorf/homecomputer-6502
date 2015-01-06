#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include "utils.h"
#include "lcd.h"
#include "led.h"
#include "basic.h"
#include "acia.h"
#include "keys.h"
#include "sid.h"
#include "readline.h"

#define syntax_error_malformed_string() syntax_error_msg("Malformed string argument!")
char argbuf[40];
char loadbuf[40];

typedef void (* command_function) ();

#define CMD_GOTO 0

const char const * keywords[] = {
  "goto",
  "run",
  "time",
  "uptime",
  "led",
  "print",
  "put",
  "list",
  "new",
  "free",
  "save",
  "load",
  "sleep",
  "cls",
  "home",
  "synth",
  "let",
  "clear",
  "vars",
  "input",
  0
};

void cmd_goto(const char *args);
void cmd_run(const char *args);
void cmd_print_time(const char *args);
void cmd_print_uptime(const char *args);
void cmd_led(const char *args);
void cmd_print(const char *args);
void cmd_put(const char *args);
void cmd_list(const char *args);
void cmd_new(const char *args);
void cmd_free(const char *args);
void cmd_save(const char *args);
void cmd_load(const char *args);
void cmd_sleep(const char *args);
void cmd_cls(const char *args);
void cmd_home(const char *args);
void cmd_synth(const char *args);
void cmd_let(const char *args);
void cmd_clear(const char *args);
void cmd_vars(const char *args);
void cmd_input(const char *args);

const command_function command_functions[] = {
  cmd_goto,
  cmd_run,
  cmd_print_time,
  cmd_print_uptime,
  cmd_led,
  cmd_print,
  cmd_put,
  cmd_list,
  cmd_new,
  cmd_free,
  cmd_save,
  cmd_load,
  cmd_sleep,
  cmd_cls,
  cmd_home,
  cmd_synth,
  cmd_let,
  cmd_clear,
  cmd_vars,
  cmd_input
};

char print_buffer[41];

typedef struct _program_line {
  unsigned int number;
  unsigned char command;
  char * args;
  struct _program_line * next;
} program_line;

program_line * program = NULL;

program_line * current_line;

unsigned char error = 0;

#define VAR_TYPE_INTEGER 0
#define VAR_TYPE_STRING 1

#define VAR_PRINT_VALUE   0
#define VAR_PRINT_VERBOSE 1

typedef struct _variable {
  unsigned int name;
  unsigned char type;
  union {
    int integer;
    char *string;
  } value;
  struct _variable *next;
} variable;

variable *variables = NULL;

/**
 * Skip any whitespace in the argument string pointed to by args.
 * Returns a pointer to the first non whitespace character.
 */
const char * skip_whitespace(const char *args) {
  while (*args == ' ') {
    ++args;
  }
  return args;
}

const char * find_args(char *s) {
  char * args = s;
  while (*args && *args != ' ') {
    ++args;
  }
  if (*args == ' ') {
    *args = '\0';
    ++args;
  }
  return skip_whitespace(args);
}

unsigned char find_keyword(char *s) {
  unsigned char index = 0;
  while (keywords[index]) {
    if (strcasecmp(keywords[index], s) == 0) {
      return index;
    }
    ++index;
  }
  return 0xff;
}

void syntax_error_msg(char *msg) {
  error = 1;
  if (current_line) {
    sprintf(print_buffer, "%u: ", current_line->number);
    lcd_puts(print_buffer);
  }
  lcd_puts(msg);
  lcd_puts("\n");
}

void syntax_error() {
  syntax_error_msg("Syntax error!");
}

void execute(char *s) {
  unsigned char command;
  const char * args;
  current_line = 0;
  args = find_args(s);
  command = find_keyword(s);
  if (command != 0xff) {
    command_functions[command](args);
  } else {
    lcd_puts("Unknown command!\n");
  }
}

void delete_line(unsigned int line_number) {
  program_line *prev_line = 0;
  program_line *line = program;
  while (line) {
    if (line->number == line_number) {
      if (prev_line) {
        prev_line->next = line->next;
      } else {
        program = line->next;
      }
      free(line->args);
      free(line);
      break;
    }
    prev_line = line;
    line = line->next;
  }
}

void create_line(unsigned int line_number, char *s) {
  unsigned char command;
  const char * args;
  program_line * new_line;
  args = find_args(s);
  command = find_keyword(s);
  if (command != 0xff) {
    delete_line(line_number);
    new_line = malloc(sizeof(program_line));
    new_line->number = line_number;
    new_line->command = command;
    new_line->args = malloc(strlen(args) + 1);
    strcpy(new_line->args, args);
    if (program && line_number > program->number) {
      program_line *line = program;
      while (line->next && line_number > line->next->number) {
        line = line->next;
      }
      new_line->next = line->next;
      line->next = new_line;
    } else {
      new_line->next = program;
      program = new_line;
    }
  } else {
    lcd_puts("Unknown command!\n");
  }
}

void interpret(char *s) {
  const char * command = s;
  unsigned int line_number;

  error = 0;

  if (strlen(s) == 0) {
    return;
  }

  if (isdigit(s[0])) {
    sscanf(s, "%u", &line_number);
    command = strchr(s, ' ');
    command = skip_whitespace(command);
    if (command) {
      create_line(line_number, (char *) command);
    } else {
      delete_line(line_number);
    }
  } else {
    execute((char *) command);
  }
}

/**
 * Parse a string argument ("...") in the string pointed to by args.
 * If a string argument is found, a pointer to its first character is returned in
 * start, a pointer to its last character is returned in end and a pointer behind
 * the string argument is returned from parse_string().
 * If no string argument is found, NULL is returned.
 */
const char *parse_string(const char *args, const char **start, const char **end) {
  const char * right_mark;
  if (*args == '"') {
    right_mark = strrchr(args, '"');
    if (right_mark != args) {
      *start = args + 1;
      *end = right_mark - 1;
      return right_mark + 1;
    }
  }
  return NULL;
}

/**
 * Parse an integer argument(+-0..9+) in the string pointed to by args.
 * If an integer argument is found, its value is returned in value and a pointer
 * behind the integer argument is returned from parse_integer().
 * If no integer argument is found, NULL is returned.
 */
const char *parse_integer(const char *args, int *value) {
  if(*args == '+' || *args == '-' || isdigit(*args)) {
    const char * next_args = args + 1;
    while (isdigit(*next_args)) {
      ++next_args;
    }
    sscanf(args, "%d", value);
    return next_args;
  }
  return NULL;
}

/**
 * Find the variable with the given name.
 * Returns NULL if the variable wasn't found.
 * Returns a pointer to the previous variable in prev if prev != NULL.
 */
variable * find_variable(unsigned int name, variable **prev) {
  variable *v = variables;
  if (prev) {
    *prev = NULL;
  }
  while (v && v->name != name) {
    if (prev) {
      *prev = v;
    }
    v = v->next;
  }
  return v;
}

/**
 * Create a new variable with name name, type type and value value.
 * Override the variable if it is already defined.
 */
void create_variable(unsigned int name, unsigned char type, const char *value) {
  variable *new_v;
  variable *prev_v;
  variable *v = find_variable(name, &prev_v);
  if (v) {
    if (v->type == VAR_TYPE_STRING) {
      free(v->value.string);
    }
    new_v = v;
  } else {
    new_v = malloc(sizeof(variable));
    new_v->name = name;
    new_v->next = variables;
    variables = new_v;
  }
  new_v->type = type;
  switch (type) {
    case VAR_TYPE_INTEGER: {
      int integer;
      if (parse_integer(value, &integer)) {
        new_v->value.integer = integer;
      } else {
        syntax_error();
      }
      break;
    }
    case VAR_TYPE_STRING: {
      unsigned char len = strlen(value);
      new_v->value.string = malloc(len + 1);
      strcpy(new_v->value.string, value);
      break;
    }
  }
}

/**
 * Delete the variable with the name name.
 */
void delete_variable(unsigned int name) {
  variable *prev_v;
  variable *v = find_variable(name, &prev_v);
  if (v) {
    if (v->type == VAR_TYPE_STRING) {
      free(v->value.string);
    }
    if (v == variables) {
      variables = v->next;
    } else {
      prev_v->next = v->next;
    }
    free(v);
  }
}

/**
 * Print the value of the variable v.
 * If mode == VAR_PRINT_VERBOSE, a verbose representation is printed
 * (e.g. "..." for strings).
 */
void print_variable(variable *v, unsigned char mode) {
  switch (v->type) {
    case VAR_TYPE_INTEGER:
      sprintf(print_buffer, "%d", v->value.integer);
      lcd_puts(print_buffer);
      break;
    case VAR_TYPE_STRING:
      if (mode == VAR_PRINT_VERBOSE) {
        lcd_putc('"');
      }
      lcd_puts(v->value.string);
      if (mode == VAR_PRINT_VERBOSE) {
        lcd_putc('"');
      }
      break;
  }
}

/**
 * Print the current time (without newline).
 */
void cmd_print_time(const char *) {
  sprintf(print_buffer, "%02d:%02d:%02d", time_hours(), time_minutes(), time_seconds());
  lcd_puts(print_buffer);
}

/**
 * Print the current time in milliseconds (without newline).
 */
void cmd_print_uptime(const char *) {
  sprintf(print_buffer, "%ld", time_millis());
  lcd_puts(print_buffer);
}

void cmd_led(const char * args) {
  if (strcmp("on", args) == 0) {
    led_set(1);
  } else if (strcmp("off", args) == 0) {
    led_set(0);
  } else {
    syntax_error();
  }
}

/**
 * Print a string constant or a variable value (no newline).
 */
void cmd_put(const char * args) {
  if (args[0] == '"') {
    const char *start;
    const char *end;
    args = parse_string(args, &start, &end);
    if (args) {
      for (; start <= end; ++start) {
        lcd_putc(*start);
      }
    } else {
      syntax_error_malformed_string();
    }
  } else if (isalpha(*args)) {
    variable *v;
    unsigned int name = *args;
    ++args;
    if (isalnum(*args)) {
      name << 8;
      name |= *args;
    }
    v = find_variable(name, NULL);
    if (v) {
      print_variable(v, VAR_PRINT_VALUE);
    } else {
      syntax_error_msg("Variable to found!");
    }
  } else if (*args == 0) {
    // Print nothing
  } else {
    syntax_error();
  }
}

/**
 * Does a print and then a newline.
 */
void cmd_print(const char *args) {
  cmd_put(args);
  if (! error) {
    lcd_put_newline();
  }
}

void cmd_list(const char *args) {
  unsigned char range = 0;
  unsigned int from_number;
  unsigned int to_number;
  program_line *line;

  if (isdigit(args[0])) {
    sscanf(args, "%u", &from_number);
    to_number = from_number;
    range = 1;
  }

  line = program;
  while (line) {
    if (range == 0 || (line->number >= from_number && line->number <= to_number)) {
      sprintf(print_buffer, "%u %s %s\n", line->number, keywords[line->command], line->args);
      lcd_puts(print_buffer);
    }
    line = line->next;
  }
}

void cmd_run(const char *) {
  unsigned char command;
  error = 0;
  current_line = program;
  while (current_line) {
    keys_update();
    if (keys_get_code() == KEY_BREAK) {
      lcd_puts("Program interrupted.\n");
      break;
    }
    command = current_line->command;
    command_functions[command](current_line->args);
    if (error) {
      break;
    }
    if (command != CMD_GOTO) {
      current_line = current_line->next;
    }
  }
}

void cmd_goto(const char *args) {
  unsigned int line_number;
  program_line *line;
  if (isdigit(args[0])) {
    sscanf(args, "%u", &line_number);
    line = program;
    while (line) {
      if (line->number == line_number) {
        current_line = line;
        return;
      }
      line = line->next;
    }
    syntax_error_msg("Line not found!");
  } else {
    syntax_error();
  }
}

/**
 * Clear the program and the variables.
 */
void cmd_new(const char *args) {
  program_line * line = program;
  cmd_clear(args);
  while (line) {
    program_line * next = line->next;
    free(line->args);
    free(line);
    line = next;
  }
  program = 0;
}

void cmd_free(const char *) {
  sprintf(print_buffer, "%u bytes free.\n", _heapmemavail());
  lcd_puts(print_buffer);
}

void cmd_save(const char * args) {
  program_line *line;
  const char *start;
  const char *end;
  if (parse_string(args, &start, &end)) {
    lcd_puts("Saving...");
    acia_puts("*SAVE \"");
    while (start <= end) {
      acia_putc(*start);
      ++start;
    }
    acia_puts("\"\n");
    line = program;
    while (line) {
      sprintf(print_buffer, "%u %s %s\n", line->number, keywords[line->command], line->args);
      acia_puts(print_buffer);
      line = line->next;
      lcd_putc('.');
    }
    acia_puts("*EOF\n");
    lcd_puts("\nOk.\n");
  } else {
    syntax_error_msg("String expected!");
  }
}

void cmd_load(const char * args) {
  const char *start;
  const char *end;
  if (parse_string(args, &start, &end)) {
    cmd_new(0);
    lcd_puts("Loading...");
    acia_puts("*LOAD \"");
    while (start <= end) {
      acia_putc(*start);
      ++start;
    }
    acia_puts("\"\n");
    for(;;) {
      acia_puts("*NEXT\n");
      acia_gets(loadbuf, 40);
      if (strncmp("*EOF", loadbuf, 4) == 0) {
        break;
      } else if (strncmp("!NOTFOUND", loadbuf, 9) == 0) {
        lcd_put_newline();
        syntax_error_msg("File not found!");
        break;
      } else {
        lcd_putc('.');
        interpret(loadbuf);
      }
    }
    if (! error) {
      lcd_puts("\nOk.\n");
    }
  } else {
    syntax_error_msg("String expected!");
  }
}

unsigned long delay;
unsigned long sleep_end_millis;

void cmd_sleep(const char *args) {
  if (isdigit(args[0])) {
    sscanf(args, "%ul", &delay);
    sleep_end_millis = time_millis() + delay;
    while (time_millis() < sleep_end_millis)
      ;
  } else {
    syntax_error();
  }
}

void cmd_cls(const char *) {
  lcd_clear();
}

void cmd_home(const char *) {
  lcd_home();
}

void cmd_synth(const char *) {
  lcd_clear();
  lcd_puts("ESC to quit\n");
  lcd_puts("Play sounds with these keys:\n");
  lcd_puts(" W E   T Z U   O P\n");
  lcd_puts("A S D F G H J K L");
  sid_synth();
  lcd_clear();
}

/**
 * Assign a value to a variable or delete the variable if no assignment is given.
 */
void cmd_let(const char *args) {
  unsigned char type = VAR_TYPE_INTEGER;
  unsigned int name;
  if (isalpha(*args)) {
    name = *args;
    ++args;
    if (isalnum(*args)) {
      name <<= 8;
      name |= *args;
      ++args;
    }
    if (*args == '$') {
      type = VAR_TYPE_STRING;
      ++args;
    }
    args = skip_whitespace(args);
    if (*args == '=') {
      args = skip_whitespace(args + 1);
      if (type == VAR_TYPE_STRING) {
        const char *start;
        const char *end;
        if (parse_string(args, &start, &end)) {
          *((char *) end + 1) = '\0';
          create_variable(name, type, start);
        } else {
          syntax_error_msg("String expected!");
        }
      } else {
        create_variable(name, type, args);
      }
    } else if (*args == '\0') {
      delete_variable(name);
    } else {
      syntax_error();
    }
  } else {
    syntax_error();
  }
}

/**
 * Delete all variables.
 */
void cmd_clear(const char *) {
  variable *v = variables;
  variable *delete_v;
  while (v) {
    delete_v = v;
    v = v->next;
    if (delete_v->type == VAR_TYPE_STRING) {
      free(delete_v->value.string);
    }
    free(delete_v);
  }
  variables = NULL;
}

/**
 * List all variables.
 */
void cmd_vars(const char *) {
  variable *v = variables;
  while (v) {
    if (v->name > 256) {
      lcd_putc(v->name >> 8);
    }
    lcd_putc(v->name & 0xff);
    if (v->type == VAR_TYPE_STRING) {
      lcd_putc('$');
    }
    lcd_puts(" = ");
    print_variable(v, VAR_PRINT_VERBOSE);
    lcd_put_newline();
    v = v->next;
  }
}

/**
 * Input a variable from the keyboard.
 */
void cmd_input(const char *args) {
  if (isalpha(*args)) {
    unsigned int name = *args;
    ++args;
    if (isalnum(*args)) {
      name <<= 8;
      name |= *args;
    }
    if (*args == '$') {
      char *line = readline();
      create_variable(name, VAR_TYPE_STRING, line);
    } else {
      syntax_error();
    }
  }
}
