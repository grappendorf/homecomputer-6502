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
  "list",
  "new",
  "free",
  "save",
  "load",
  "sleep",
  "cls",
  "home",
  "synth",
  0
};

void cmd_goto(const char *args);
void cmd_run(const char *args);
void cmd_print_time(const char *args);
void cmd_print_uptime(const char *args);
void cmd_led(const char *args);
void cmd_print(const char *args);
void cmd_list(const char *args);
void cmd_new(const char *args);
void cmd_free(const char *args);
void cmd_save(const char *args);
void cmd_load(const char *args);
void cmd_sleep(const char *args);
void cmd_cls(const char *args);
void cmd_home(const char *args);
void cmd_synth(const char *args);

const command_function command_functions[] = {
  cmd_goto,
  cmd_run,
  cmd_print_time,
  cmd_print_uptime,
  cmd_led,
  cmd_print,
  cmd_list,
  cmd_new,
  cmd_free,
  cmd_save,
  cmd_load,
  cmd_sleep,
  cmd_cls,
  cmd_home,
  cmd_synth
};

char print_buffer[41];

typedef struct _program_line {
  unsigned int number;
  unsigned char command;
  char * args;
  struct _program_line * next;
} program_line;

program_line * program = 0;

program_line * current_line;

unsigned char error = 0;

unsigned char * find_args(char *s) {
  char * args = s;
  while (*args && *args != ' ') {
    ++args;
  }
  if (*args == ' ') {
    *args = '\0';
    ++args;
  }
  return args;
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
  syntax_error_msg("Syntax error!\n");
}

void execute(char *s) {
  unsigned char command;
  char * args;
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

void parse(char *s) {
  char * command = s;
  unsigned int line_number;

  if (strlen(s) == 0) {
    return;
  }

  if (isdigit(s[0])) {
    sscanf(s, "%u", &line_number);
    command = strchr(s, ' ');
    if (command) {
      ++command;
      create_line(line_number, command);
    } else {
      delete_line(line_number);
    }
  } else {
    execute(command);
  }
}

void cmd_print_time(const char *) {
  sprintf(print_buffer, "%02d:%02d:%02d\n", time_hours(), time_minutes(), time_seconds());
  lcd_puts(print_buffer);
}

void cmd_print_uptime(const char * args) {
  sprintf(print_buffer, "%ld %s\n", time_millis(), args);
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

void cmd_print(const char * args) {
  if (args[0] == '"' && args[strlen(args) - 1] == '"') {
    const char * p = args + 1;
    for (; *p != '"'; ++p) {
      lcd_putc(*p);
    }
    lcd_put_newline();
  } else if (*args == 0) {
    // Print nothing
  } else {
    syntax_error();
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

void cmd_new(const char *) {
  program_line * line = program;
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
  acia_puts("*SAVE ");
  acia_puts(args);
  acia_puts("\n");
  line = program;
  while (line) {
    sprintf(print_buffer, "%u %s %s\n", line->number, keywords[line->command], line->args);
    acia_puts(print_buffer);
    line = line->next;
  }
  acia_puts("*EOF\n");
}

void cmd_load(const char * args) {
  cmd_new(0);
  lcd_puts("Loading...");
  acia_puts("*LOAD ");
  acia_puts(args);
  acia_puts("\n");
  for(;;) {
    acia_puts("*NEXT\n");
    acia_gets(loadbuf, 40);
    if (strncmp("*EOF", loadbuf, 4) == 0) {
      break;
    } else {
      lcd_putc('.');
      parse(loadbuf);
    }
  }
  lcd_puts("\nOk.\n");
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