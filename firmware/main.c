#include <stdio.h>
#include <stdlib.h>
#include "utils.h"
#include "acia.h"
#include "led.h"
#include "lcd.h"
#include "keys.h"
#include "basic.h"
#include "debug.h"

char buffer[41];
char *buffer_pos;
unsigned char last_key = 0xff;
char last_char = 0;

int main() {

  acia_init();
  lcd_init();
  led_init();
  keys_init();

  acia_puts("6502 HomeComputer ready.\n");
  lcd_puts("6502 HomeComputer ready!\n");
  sprintf(buffer, "%u bytes free.\n", _heapmemavail());
  lcd_puts(buffer);
  lcd_cursor_on();
  lcd_cursor_blink();

  buffer_pos = buffer;

  for (;;) {

    keys_update();
    if (keys_get_code() != 0xff) {
      if (last_key == 0xff) {
        last_key = keys_get_code();
        last_char = keys_getc();
        if ((last_key) == KEY_BACKSPACE) {
          if (buffer_pos != buffer) {
            --buffer_pos;
            lcd_goto(lcd_get_x() - 1, lcd_get_y());
            lcd_putc(' ');
            lcd_goto(lcd_get_x() - 1, lcd_get_y());
          }
        } else if (last_char != 0 && buffer_pos - buffer < 39) {
          lcd_putc(last_char);
          if (last_char == '\n') {
            *buffer_pos = '\0';
            interpret(buffer);
            buffer_pos = buffer;
          } else {
            *buffer_pos = last_char;
            ++buffer_pos;
          }
        }
      }
    } else {
      last_key = 0xff;
    }
  }

  return 0;
}