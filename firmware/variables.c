#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "lcd.h"
#include "basic.h"
#include "utils.h"
#include "interrupt.h"
#include "keys.h"
#include "variables.h"

// Pointer to the list of variables
variable *variables = NULL;

/**
 * Find the variable with the given name.
 * Returns NULL if the variable wasn't found.
 * Returns a pointer to the previous variable in prev if prev != NULL.
 */
variable * find_variable(unsigned int name, unsigned char type, variable **prev) {
  variable *v = variables;
  if (prev) {
    *prev = NULL;
  }
  while (v && (v->name != name ||
         (v->type & VAR_TYPE_MASK) != (type & VAR_TYPE_MASK))) {
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
void create_variable(unsigned int name, unsigned char type, void *value) {
  variable *new_v;
  variable *prev_v;
  variable *v = find_variable(name, type, &prev_v);

  if (v && v->type & VAR_FLAG_BUILTIN) {
    syntax_error_msg("Cannot overwrite builtin!");
    return;
  }

  if (v) {
    if ((v->type & VAR_TYPE_MASK) == VAR_TYPE_STRING) {
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
  switch (type & VAR_TYPE_MASK) {
    case VAR_TYPE_INTEGER: {
      if (type & VAR_FLAG_BUILTIN) {
        new_v->value.builtin_string = value;
      } else {
        new_v->value.integer = *((int *)value);
      }
      break;
    }
    case VAR_TYPE_STRING: {
      if (type & VAR_FLAG_BUILTIN) {
        new_v->value.builtin_string = value;
      } else {
        unsigned char len = strlen(value);
        new_v->value.string = malloc(len + 1);
        strcpy(new_v->value.string, value);
      }
      break;
    }
  }
}

/**
 * Delete the variable with the name 'name' and the type 'type'.
 */
void delete_variable(unsigned int name, unsigned char type) {
  variable *prev_v;
  variable *v = find_variable(name, type, &prev_v);

  if (v && v->type & VAR_FLAG_BUILTIN) {
    syntax_error_msg("Cannot delete builtin!");
    return;
  }

  if (v) {
    if ((v->type & VAR_TYPE_MASK) == VAR_TYPE_STRING) {
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
  switch (v->type & VAR_TYPE_MASK) {
    case VAR_TYPE_INTEGER:
      if (v->type & VAR_FLAG_BUILTIN) {
        sprintf(print_buffer, "%d", v->value.builtin_integer());
      } else {
        sprintf(print_buffer, "%d", v->value.integer);
      }
      lcd_puts(print_buffer);
      break;
    case VAR_TYPE_STRING:
      if (mode == VAR_PRINT_VERBOSE) {
        lcd_putc('"');
      }
      if (v->type & VAR_FLAG_BUILTIN) {
        lcd_puts(v->value.builtin_string());
      } else {
        lcd_puts(v->value.string);
      }
      if (mode == VAR_PRINT_VERBOSE) {
        lcd_putc('"');
      }
      break;
  }
}

/**
 * Return the value of the builtin ti$ variable ("00:00:00").
 */
char *builtin_var_time_string() {
  static char builtin_var_time_buffer[9]; // "00:00:00"
  sprintf(builtin_var_time_buffer, "%02d:%02d:%02d", time_hours(), time_minutes(), time_seconds());
  return builtin_var_time_buffer;
}

/**
 * Return the value of the builtin ti variable (time in milliseconds).
 */
int builtin_var_time_integer() {
  return time_millis();
}

/**
 * Return the value of the builtin rn variable (random value).
 */
int builtin_var_random_integer() {
  return rand();
}

/**
 * Initialize all builtin varaibles.
 */
void init_builtin_variables() {
  create_variable(('t' << 8) | 'i', VAR_FLAG_BUILTIN | VAR_TYPE_INTEGER, builtin_var_time_integer);
  create_variable(('t' << 8) | 'i', VAR_FLAG_BUILTIN | VAR_TYPE_STRING, builtin_var_time_string);
  create_variable(('r' << 8) | 'n', VAR_FLAG_BUILTIN | VAR_TYPE_INTEGER, builtin_var_random_integer);
}

/**
 * Delete all variables except the builtin ones.
 */
void clear_variables() {
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
  init_builtin_variables();
}


/**
 * List all variables.
 */
void print_all_variables() {
  unsigned char first = 1;
  variable *v = variables;
  while (v) {
    if (first) {
      first = 0;
    } else {
      do {
        if (is_interrupted()) {
          lcd_puts("Interrupted.\n");
          return;
        }
        keys_update();
      } while (keys_get_code() == KEY_NONE);
    }

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
 * Return the value of a string variable.
 */
char * get_string_variable_value(variable *var) {
  if (var->type & VAR_FLAG_BUILTIN) {
    return var->value.builtin_string();
  } else {
    return var->value.string;
  }
}

/**
 * Return the value of a integer variable.
 */
int get_integer_variable_value(variable *var) {
  if (var->type & VAR_FLAG_BUILTIN) {
    return var->value.builtin_integer();
  } else {
    return var->value.integer;
  }
}
