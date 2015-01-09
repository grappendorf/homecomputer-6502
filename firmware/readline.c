#include "lcd.h"
#include "keys.h"
#include "readline.h"
#include "interrupt.h"

#define MAX_CHARS 79
char readline_buffer[MAX_CHARS + 1];
char *buffer_pos;

void delete_character();

char * readline(unsigned char interruptible) {
  unsigned char last_key = KEY_NONE;
  char last_char;
  unsigned char last_modifiers;
  reset_interrupted();
  buffer_pos = readline_buffer;

  for (;;) {
    if (interruptible && is_interrupted()) {
      lcd_put_newline();
      *buffer_pos = '\0';
      return readline_buffer;
    }

    keys_update();

    if (keys_get_code() != KEY_NONE) {
      if (last_key == KEY_NONE) {
        last_key = keys_get_code();
        last_char = keys_getc();
        last_modifiers = keys_get_modifiers();

        if ((last_key) == KEY_BACKSPACE) {
          if (buffer_pos != readline_buffer) {
            delete_character();
          }
        } else if (last_char == '\n') {
          lcd_putc(last_char);
          *buffer_pos = '\0';
          break;
        } else if (last_modifiers ==  MODIFIER_CTRL && last_key == KEY_U) {
           while (buffer_pos != readline_buffer) {
              delete_character();
            }
        } else if (last_char != 0 && buffer_pos - readline_buffer < MAX_CHARS) {
          lcd_putc(last_char);
          *buffer_pos = last_char;
          ++buffer_pos;
        }
      }
    } else {
      last_key = KEY_NONE;
    }
  }

  return readline_buffer;
}

void delete_character() {
  --buffer_pos;
  if (lcd_get_x() == 0) {
    lcd_goto(39, lcd_get_y() - 1);
    lcd_putc(' ');
    lcd_goto(39, lcd_get_y() - 1);
  } else {
    lcd_goto(lcd_get_x() - 1, lcd_get_y());
    lcd_putc(' ');
    lcd_goto(lcd_get_x() - 1, lcd_get_y());
  }
}