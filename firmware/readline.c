#include "lcd.h"
#include "keys.h"
#include "readline.h"
#include "interrupt.h"

char readline_buffer[256];
char *buffer_pos;
unsigned char last_key = KEY_NONE;
char last_char = 0;

char * readline(unsigned char interruptible) {
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

        if ((last_key) == KEY_BACKSPACE) {
          if (buffer_pos != readline_buffer) {
            --buffer_pos;
            lcd_goto(lcd_get_x() - 1, lcd_get_y());
            lcd_putc(' ');
            lcd_goto(lcd_get_x() - 1, lcd_get_y());
          }
        } else if (last_char == '\n') {
          lcd_putc(last_char);
          *buffer_pos = '\0';
          break;
        } else if (last_char != 0 && buffer_pos - readline_buffer < 39) {
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
