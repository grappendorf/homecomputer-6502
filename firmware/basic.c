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
#include "debug.h"

void basic_init();
void interpret(char *s);
void execute(char *s);

char *parse_number_expression(char *s, int *value);
char *parse_number_term(char *s, int *value);
char *parse_string_expression(char *s, char **value);
char *parse_string(char *s, char *value);
char *parse_integer(char *s, int *value);
char *parse_variable(char *s, unsigned int *name, unsigned char *type);
unsigned char next_token(char *s);
char *consume_token(char *s, unsigned char token);
char * skip_whitespace(char *s);
char * find_args(char *s);
unsigned char find_keyword(char *s);

void syntax_error_msg(char *msg);
void syntax_error();
#define syntax_error_invalid_string() syntax_error_msg("Invalid string expression!")
#define syntax_error_invalid_number() syntax_error_msg("Invalid number expression!")

void delete_line(unsigned int line_number);
void create_line(unsigned int line_number, char *s);

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
void cmd_input(char *args);
void cmd_at(char *args);
void cmd_cursor(char *args);
void cmd_seed(char *args);

typedef void (* command_function) ();

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
  cmd_input,
  cmd_at,
  cmd_cursor,
  cmd_seed
};

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
  "input",
  "at",
  "cursor",
  "seed",
  0
};

#define CMD_UNKNOWN 0xFF
#define CMD_GOTO 0

char print_buffer[41];
char parsebuf[256];

typedef struct _program_line {
  unsigned int number;
  unsigned char command;
  char * args;
  struct _program_line * next;
} program_line;

program_line * program = NULL;

program_line * current_line;

unsigned char error = 0;

#define TOKEN_END         0
#define TOKEN_DIGITS      1
#define TOKEN_STRING      2
#define TOKEN_VAR_NUMBER  3
#define TOKEN_VAR_STRING  4
#define TOKEN_EQUAL       5
#define TOKEN_PLUS        6
#define TOKEN_MINUS       7
#define TOKEN_MUL         8
#define TOKEN_DIV         9
#define TOKEN_MOD         10
#define TOKEN_INVALID     0xFF;

/**
 * Initialize the BASIC interpreter.
 */
void basic_init() {
  init_builtin_variables();
}

/**
 * Interpret the input buffer 's'. This either executes the BASIC command in 's' or
 * creates a new program line containing the parsed command in 's'.
 */
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
 * Execute the BASIC command in 's'.
 */
void execute(char *s) {
  unsigned char command;
  char * args;
  reset_interrupted();
  current_line = 0;
  args = find_args(s);
  command = find_keyword(s);
  if (command != CMD_UNKNOWN) {
    command_functions[command](args);
  } else {
    lcd_puts("Unknown command!\n");
  }
}

/**
 * Parse a number expression 's' (that contains only number values) and return its
 * resulting value in 'value'.
 * Return a pointer behind the last character of the expression.
 * Return NULL if a syntax error occurred.
 */
char *parse_number_expression(char *s, int *value) {
  unsigned char token;
  int operand;
  if (s = parse_number_term(s, value)) {
    while ((token = next_token(s)) != TOKEN_END) {
      s = consume_token(s, token);
      switch (token) {
        case TOKEN_PLUS:
          if (s = parse_number_term(s, &operand)) {
            *value += operand;
          }
          break;
        case TOKEN_MINUS:
          if (s = parse_number_term(s, &operand)) {
            *value -= operand;
          }
          break;
        case TOKEN_MUL:
          if (s = parse_number_term(s, &operand)) {
            *value *= operand;
          }
          break;
        case TOKEN_DIV:
          if (s = parse_number_term(s, &operand)) {
            *value /= operand;
          }
          break;
        case TOKEN_MOD:
          if (s = parse_number_term(s, &operand)) {
            *value %= operand;
          }
          break;
        default:
          syntax_error();
          return NULL;
          break;
      }
    }
    return s;
  }
  return NULL;
}

char *parse_number_term(char *s, int *value) {
  unsigned char token;
  s = skip_whitespace(s);
  token = next_token(s);
  if (token == TOKEN_DIGITS || token == TOKEN_PLUS || token == TOKEN_MINUS) {
    if (s = parse_integer(s, value)) {
      return s;
    } else {
      syntax_error_invalid_number();
    }
  } else if (token == TOKEN_VAR_NUMBER) {
    unsigned int var_name;
    unsigned char var_type;
    variable *var;
    s = parse_variable(s, &var_name, &var_type);
    var = find_variable(var_name, VAR_TYPE_INTEGER, NULL);
    if (var) {
      *value = get_integer_variable_value(var);
      return s;
    } else {
      syntax_error_msg("Variable not found!");
    }
  } else {
    syntax_error();
  }

  return NULL;
}

/**
 * Parse the string expression 's' (that contains only string values) and return its
 * resulting value in 'value'.
 * Return a pointer behind the last character of the expression.
 * Return NULL if a syntax error occurred.
 */
char *parse_string_expression(char *s, char **value) {
  unsigned int var_name;
  variable *var;
  unsigned char var_type;
  s = skip_whitespace(s);
  if (next_token(s) == TOKEN_STRING) {
    if (s = parse_string(s, parsebuf)) {
      *value = parsebuf;
      return s;
    } else {
      syntax_error_invalid_string();
    }
  } else if (next_token(s) == TOKEN_VAR_STRING) {
    s = parse_variable(s, &var_name, &var_type);
    var = find_variable(var_name, VAR_TYPE_STRING, NULL);
    if (var) {
      *value = get_string_variable_value(var);
      return s;
    } else {
      syntax_error_msg("Variable not found!");
    }
  } else {
    syntax_error();
  }

  return NULL;
}

/**
 * Parse a string argument ("...") at the beginning of the string pointed to by 's'.
 * If a string argument is found, it is copied to the buffer 'value' and a pointer
 * behind the string argument is returned from parse_string().
 * If no string argument is found, NULL is returned and 'value' is not modified.
 */
char *parse_string(char *s, char *value) {
  char *right_mark;
  s = skip_whitespace(s);
  if (*s == '"') {
    ++s;
    right_mark = strchr(s, '"');
    if (right_mark) {
      int len = right_mark - s;
      strncpy(value, s, len);
      *(value + len) = '\0';
      return right_mark + 1;
    }
  }
  return NULL;
}

/**
 * Parse an integer argument(+-0..9+) in the string pointed to by 's'.
 * If an integer argument is found, its value is returned in 'value' and a pointer
 * behind the integer argument is returned from parse_integer().
 * If no integer argument is found, NULL is returned.
 */
char *parse_integer(char *s, int *value) {
  s = skip_whitespace(s);
  if(*s == '+' || *s == '-' || isdigit(*s)) {
    sscanf(s, "%d", value);
    ++s;
    while (isdigit(*s)) {
      ++s;
    }
    return s;
  }
  return NULL;
}

/**
 * Parse a variable in the string 's' and return its name in 'name', its type
 * in 'type' and a pointer behind the variable.
 * If no variable is found, NULL is returned;
 */
char *parse_variable(char *s, unsigned int *name, unsigned char *type) {
  s = skip_whitespace(s);
  if (isalpha(*s)) {
    *name = *s;
    ++s;
    if (isalnum(*s)) {
      *name <<= 8;
      *name |= *s;
    }
    while (isalnum(*s)) { ++s; }
    if (*s == '$') {
      ++s;
      *type = VAR_TYPE_STRING;
    } else {
      *type = VAR_TYPE_INTEGER;
    }
    return s;
  }
  return NULL;
}

/**
 * Consume the token 'token' in the string 's'.
 * Return a pointer behind the token.
 * If the token wasn't found, return NULL with a syntax error.
 * Currently only implemented for operator tokens.
 */
char *consume_token(char *s, unsigned char token) {
  s = skip_whitespace(s);
  if ((token == TOKEN_EQUAL && *s == '=') ||
      (token == TOKEN_PLUS && *s == '+') ||
      (token == TOKEN_MINUS && *s == '-') ||
      (token == TOKEN_MUL && *s == '*') ||
      (token == TOKEN_DIV && *s == '/') ||
      (token == TOKEN_MOD && *s == '%')) {
    return s + 1;
  }
  return NULL;
}

/**
 * Get the token id of the first token in 's'.
 */
unsigned char next_token(char *s) {
  s = skip_whitespace(s);
  if(isdigit(*s)){
    return TOKEN_DIGITS;
  } else if (*s == '"') {
    return TOKEN_STRING;
  } else if (isalpha(*s)) {
    while (isalnum(*s)) { ++s; }
    if (*s == '$') {
      return TOKEN_VAR_STRING;
    } else {
      return TOKEN_VAR_NUMBER;
    }
  } else if (*s == '=') {
    return TOKEN_EQUAL;
  } else if (*s == '+') {
    return TOKEN_PLUS;
  } else if (*s == '-') {
    return TOKEN_MINUS;
  } else if (*s == '*') {
    return TOKEN_MUL;
  } else if (*s == '/') {
    return TOKEN_DIV;
  } else if (*s == '%') {
    return TOKEN_MOD;
  } else if (*s == '\0' || *s == ';') {
    return TOKEN_END;
  }
  return TOKEN_INVALID;
}

/**
 * Skip any whitespace in the string pointed to by 's'.
 * Returns a pointer to the first non whitespace character.
 */
char *skip_whitespace(char *s) {
  while (*s == ' ') {
    ++s;
  }
  return s;
}

/**
 * Find the start of the arguments after the command in 's'.
 * Side-effect: terminate the command with a '\0'.
 */
char *find_args(char *s) {
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

/**
 * Find the keyword index of the command string 's'.
 * Returns CMD_UNKNOWN if no command was found.
 */
unsigned char find_keyword(char *s) {
  unsigned char index = 0;
  while (keywords[index]) {
    if (strcasecmp(keywords[index], s) == 0) {
      return index;
    }
    ++index;
  }
  return CMD_UNKNOWN;
}

/**
 * Print the error message 'msg' and set the error flag.
 */
void syntax_error_msg(char *msg) {
  error = 1;
  if (current_line) {
    sprintf(print_buffer, "%u: ", current_line->number);
    lcd_puts(print_buffer);
  }
  lcd_puts(msg);
  lcd_puts("\n");
}

/**
 * Print the standard error message and set the error flag.
 */
void syntax_error() {
  syntax_error_msg("Syntax error!");
}

/**
 * Selete the program line with number 'number'.
 */
void delete_line(unsigned int number) {
  program_line *prev_line = 0;
  program_line *line = program;
  while (line) {
    if (line->number == number) {
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

/**
 * Create a new program line with number 'number' and the command in 's'.
 */
void create_line(unsigned int number, char *s) {
  unsigned char command;
  char * args;
  program_line * new_line;
  args = find_args(s);
  command = find_keyword(s);
  if (command != CMD_UNKNOWN) {
    delete_line(number);
    new_line = malloc(sizeof(program_line));
    new_line->number = number;
    new_line->command = command;
    new_line->args = malloc(strlen(args) + 1);
    strcpy(new_line->args, args);
    if (program && number > program->number) {
      program_line *line = program;
      while (line->next && number > line->next->number) {
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

/**
 * Enable/Disable the LED.
 * LET ON|OFF
 */
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
 * PUT <expression>
 */
void cmd_put(char *args) {
  int number_value;
  char *string_value;
  unsigned char token;
  token = next_token(args);
  if (token == TOKEN_STRING || token == TOKEN_VAR_STRING) {
    if (parse_string_expression(args, &string_value)) {
      lcd_puts(string_value);
    }
  } else if (token == TOKEN_DIGITS || token == TOKEN_PLUS || token == TOKEN_MINUS ||
             token == TOKEN_VAR_NUMBER) {
    if (parse_number_expression(args, &number_value)) {
      sprintf(print_buffer, "%d", number_value);
      lcd_puts(print_buffer);
    }
  } else {
    syntax_error();
  }
  return;
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

/**
 * List the program.
 * LiST [<from>]
 */
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

/**
 * Run the program.
 * RUN
 */
void cmd_run(char *) {
  unsigned char command;
  error = 0;
  current_line = program;
  while (current_line) {
    if (is_interrupted()) {
      lcd_puts("Interrupted.\n");
      lcd_cursor_on();
      lcd_cursor_blink();
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

/**
 * Jump to another program line.
 * GOTO <line>
 */
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

/**
 * Print the number of free RAM bytes.
 * FREE
 */
void cmd_free(char *) {
  sprintf(print_buffer, "%u bytes free.\n", _heapmemavail());
  lcd_puts(print_buffer);
}

/**
 * Save a program by sending it to the terminal program over the serial line.
 * SAVE "<filename>"
 */
void cmd_save(char *args) {
  if (parse_string(args, parsebuf)) {
    program_line *line = program;
    lcd_puts("Saving...");
    acia_puts("*SAVE \"");
    acia_puts(parsebuf);
    acia_puts("\"\n");
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
  if (parse_string(args, parsebuf)) {
    cmd_new(0);
    lcd_puts("Loading...");
    acia_puts("*LOAD \"");
    acia_puts(parsebuf);
    acia_puts("\"\n");
    for(;;) {
      acia_puts("*NEXT\n");
      acia_gets(readline_buffer, 255);
      if (strncmp("*EOF", readline_buffer, 4) == 0) {
        break;
      } else if (strncmp("!NOTFOUND", readline_buffer, 9) == 0) {
        lcd_put_newline();
        syntax_error_msg("File not found!");
        break;
      } else {
        lcd_putc('.');
        interpret(readline_buffer);
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
    acia_gets(readline_buffer, 255);
    if (strncmp("*EOF", readline_buffer, 4) == 0) {
      break;
    } else {
      lcd_puts(readline_buffer);

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

/**
 * Pause the program for some specified amount of time.
 * SLEEP <milliseconds>
 */
void cmd_sleep(char *args) {
  static unsigned long delay;
  static unsigned long sleep_end_millis;
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

/**
 * Clear the screen.
 * CLS
 */
void cmd_cls(char *) {
  lcd_clear();
}

/**
 * Set the cursor to the home position (0,0).
 * HOME
 */
void cmd_home(char *) {
  lcd_home();
}

/**
 * Start the synthesizer program.
 * SYNTH
 */
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
 * List all variables if no arguments are given.
 */
void cmd_let(char *args) {
  unsigned char token;
  unsigned int var_name;
  unsigned char var_type;

  skip_whitespace(args);

  if (*args == '\0') {
    print_all_variables();
    lcd_puts("Ready.\n");
    return;
  }

  if (args = parse_variable(args, &var_name, &var_type)) {
    if (next_token(args) == TOKEN_EQUAL) {
      token = next_token(args);
      args = consume_token(args, token);
      switch (var_type) {
        case VAR_TYPE_INTEGER: {
          int value;
          if (parse_number_expression(args, &value)) {
            create_variable(var_name, var_type, &value);
          }
          break;
        }
        case VAR_TYPE_STRING: {
          char *value;
          if (parse_string_expression(args, &value)) {
            create_variable(var_name, var_type, value);
          }
          break;
        }
      }
    } else {
      if (*args == '\0') {
        delete_variable(var_name, var_type);
      }
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

/**
 * Set the cursor to a specific screen position.
 * AT <x>,<y>
 */
void cmd_at(char *args) {
  int x;
  int y;

  args = parse_integer(args, &x);
  if (! args || x < 0 || x > 39) {
    syntax_error();
    return;
  }

  args = skip_whitespace(args);
  if (*args != ',') {
    syntax_error();
    return;
  }
  ++args;

  args = skip_whitespace(args);
  args = parse_integer(args, &y);
  if (!args || y < 0 || y > 3) {
    syntax_error();
    return;
  }

  lcd_goto(x, y);
}

/**
 * Enable or disable the cursor.
 * CURSOR on|off
 */
void cmd_cursor(char *args) {
  if (strcmp("on", args) == 0) {
    lcd_cursor_on();
    lcd_cursor_blink();
  } else if (strcmp("off", args) == 0) {
    lcd_cursor_off();
  } else {
    syntax_error();
  }
}

/**
 * Seed the random number generator (e.g. SEED TI).
 * SEED <number>
 */
void cmd_seed(char *args) {
  int seed;
  if (parse_number_expression(args, &seed)) {
    seed_random_number_variable(seed);
  }
}


