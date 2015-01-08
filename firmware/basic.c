#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include "lcd.h"
#include "led.h"
#include "basic.h"
#include "acia.h"
#include "keys.h"
#include "sid.h"
#include "readline.h"
#include "interrupt.h"
#include "variables.h"
#include "utils.h"
#include "basic.h"

#define syntax_error_malformed_string() syntax_error_msg("Malformed string argument!")
char argbuf[40];
char loadbuf[40];

typedef void (* command_function) ();

#define CMD_GOTO 0

const char const * keywords[] = {
  "goto",
  "run",
  "led",
  "print",
  "put",
  "list",
  "new",
  "free",
  "save",
  "load",
  "dir",
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

void cmd_goto(char *args);
void cmd_run(char *args);
void cmd_led(char *args);
void cmd_print(char *args);
void cmd_put(char *args);
void cmd_list(char *args);
void cmd_new(char *args);
void cmd_free(char *args);
void cmd_save(char *args);
void cmd_load(char *args);
void cmd_dir(char *args);
void cmd_sleep(char *args);
void cmd_cls(char *args);
void cmd_home(char *args);
void cmd_synth(char *args);
void cmd_let(char *args);
void cmd_clear(char *args);
void cmd_vars(char *args);
void cmd_input(char *args);

const command_function command_functions[] = {
  cmd_goto,
  cmd_run,
  cmd_led,
  cmd_print,
  cmd_put,
  cmd_list,
  cmd_new,
  cmd_free,
  cmd_save,
  cmd_load,
  cmd_dir,
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

/**
 * Skip any whitespace in the string pointed to by 's'.
 * Returns a pointer to the first non whitespace character.
 */
char * skip_whitespace(char *s) {
  while (*s == ' ') {
    ++s;
  }
  return s;
}

char * find_args(char *s) {
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
  char * args;
  reset_interrupted();
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
  char * args;
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
  char * command = s;
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
 * Parse a string argument ("...") at the beginning of the string pointed to by 's'.
 * If a string argument is found, a pointer to its first character is returned in 'value',
 * the terminating '"' character is replaced with '\0' and a pointer begind the string
 * argument is returned from parse_string().
 * You need to restore the '"' character, if you want to reuse 's'
 * If no string argument is found, NULL is returned and 's' is not modified.
 */
char *parse_string(char *s, char **value) {
  if (*s == '"') {
    char *right_mark = strchr(s + 1, '"');
    if (right_mark) {
      *value = s + 1;
      *right_mark = '\0';
      return right_mark + 1;
    }
  }
  return NULL;
}

#define parse_string_restore(s) *(s + strlen(s)) = '"';

/**
 * Parse an integer argument(+-0..9+) in the string pointed to by 's'.
 * If an integer argument is found, its value is returned in 'value' and a pointer
 * behind the integer argument is returned from parse_integer().
 * If no integer argument is found, NULL is returned.
 */
char *parse_integer(char *s, int *value) {
  if(*s == '+' || *s == '-' || isdigit(*s)) {
    char *next = s + 1;
    while (isdigit(*next)) {
      ++next;
    }
    sscanf(s, "%d", value);
    return next;
  }
  return NULL;
}

/**
 * Initialize the BASIC interpreter.
 */
void basic_init() {
  init_builtin_variables();
}

void cmd_led(char *args) {
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
void cmd_put(char *args) {
  if (args[0] == '"') {
    char *value;
    args = parse_string(args, &value);
    if (args) {
      lcd_puts(value);
      parse_string_restore(value);
    } else {
      syntax_error_malformed_string();
    }
  } else if (isalpha(*args)) {
    unsigned char type = VAR_TYPE_INTEGER;
    variable *v;
    unsigned int name = *args;
    ++args;
    if (isalnum(*args)) {
      name <<= 8;
      name |= *args;
      ++args;
    }
    if (*args == '$') {
      type = VAR_TYPE_STRING;
    }
    v = find_variable(name, type, NULL);
    if (v) {
      print_variable(v, VAR_PRINT_VALUE);
    } else {
      syntax_error_msg("Variable not found!");
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
void cmd_print(char *args) {
  cmd_put(args);
  if (! error) {
    lcd_put_newline();
  }
}

void cmd_list(char *args) {
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
      do {
        if (is_interrupted()) {
          lcd_puts("Interrupted.\n");
          return;
        }
        keys_update();
      } while (keys_get_code() == KEY_NONE);
    }
    line = line->next;
  }

  lcd_puts("Ready.\n");
}

void cmd_run(char *) {
  unsigned char command;
  error = 0;
  current_line = program;
  while (current_line) {
    if (is_interrupted()) {
      lcd_puts("Interrupted.\n");
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
  lcd_puts("Ready.\n");
}

void cmd_goto(char *args) {
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
void cmd_new(char *args) {
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

void cmd_free(char *) {
  sprintf(print_buffer, "%u bytes free.\n", _heapmemavail());
  lcd_puts(print_buffer);
}

/**
 * Save a program by sending it to the terminal program over the serial line.
 * SAVE "<filename>"
 */
void cmd_save(char *args) {
  char *filename;
  if (parse_string(args, &filename)) {
    program_line *line = program;
    lcd_puts("Saving...");
    acia_puts("*SAVE \"");
    acia_puts(filename);
    acia_puts("\"\n");
    parse_string_restore(filename);
    while (line) {
      sprintf(print_buffer, "%u %s %s\n", line->number, keywords[line->command], line->args);
      acia_puts(print_buffer);
      line = line->next;
      lcd_putc('.');
    }
    acia_puts("*EOF\n");
    lcd_puts("\nReady.\n");
  } else {
    syntax_error_msg("String expected!");
  }
}

/**
 * Load a program by reading it from the terminal program over the serial line.
 * LOAD "<filename>"
 */
void cmd_load(char *args) {
  char *filename;
  if (parse_string(args, &filename)) {
    cmd_new(0);
    lcd_puts("Loading...");
    acia_puts("*LOAD \"");
    acia_puts(filename);
    acia_puts("\"\n");
    parse_string_restore(filename);
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
      lcd_puts("\nReady.\n");
    }
  } else {
    syntax_error_msg("String expected!");
  }
}

/**
 * List all programs stored on the terminal host over the serial line.
 * DIR
 */
void cmd_dir(char *) {
  acia_puts("*DIR\n");
  for(;;) {
    acia_puts("*NEXT\n");
    acia_gets(loadbuf, 40);
    if (strncmp("*EOF", loadbuf, 4) == 0) {
      break;
    } else {
      lcd_puts(loadbuf);

      lcd_put_newline();
      do {
        if (is_interrupted()) {
          acia_puts("*BREAK\n");
          lcd_puts("Interrupted.\n");
          return;
        }
        keys_update();
      } while (keys_get_code() == KEY_NONE);
    }
  }
  lcd_puts("Ready.\n");
}

unsigned long delay;
unsigned long sleep_end_millis;

void cmd_sleep(char *args) {
  if (isdigit(args[0])) {
    sscanf(args, "%ul", &delay);
    sleep_end_millis = time_millis() + delay;
    while (time_millis() < sleep_end_millis) {
      if (is_interrupted()) {
        break;
      }
    }
  } else {
    syntax_error();
  }
}

void cmd_cls(char *) {
  lcd_clear();
}

void cmd_home(char *) {
  lcd_home();
}

void cmd_synth(char *) {
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
void cmd_let(char *args) {
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
        char *value;
        if (parse_string(args, &value)) {
          create_variable(name, type, value);
          parse_string_restore(value)
        } else {
          syntax_error_msg("String expected!");
        }
      } else {
        create_variable(name, type, args);
      }
    } else if (*args == '\0') {
      delete_variable(name, type);
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
void cmd_clear(char *) {
  clear_variables();
}

/**
 * List all variables.
 */
void cmd_vars(char *) {
  print_all_variables();
  lcd_puts("Ready.\n");
}

/**
 * Input a variable from the keyboard.
 */
void cmd_input(char *args) {
  if (isalpha(*args)) {
    unsigned int name = *args;
    ++args;
    if (isalnum(*args)) {
      name <<= 8;
      name |= *args;
    }
    if (*args == '$') {
      char *line = readline(INTERRUPTIBLE);
      create_variable(name, VAR_TYPE_STRING, line);
    } else {
      syntax_error();
    }
  }
}
