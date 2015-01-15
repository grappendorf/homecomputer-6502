#include <stdio.h>
#include "debug.h"
#include "keys.h"
#include "lcd.h"
#include "led.h"
#include "utils.h"

char outbuf[41];

/**
 * Print the current  key code.
 */
void debug_keys() {
  for (;;) {
    keys_update();
    sprintf(outbuf, "%03d %02x %c", (unsigned int) keys_get_code(), (int) keys_get_modifiers(), keys_getc());
    lcd_home();
    lcd_puts(outbuf);
  }
}

/**
 * Print the state of the keyboard matrix.
 */
void debug_key_matrix() {
  unsigned char row;
  unsigned char columns;
  for (;;) {
    for (row = 0; row <= 13; ++row) {
      lcd_goto((row / 4) * 10, row % 4);
      columns = keys_read_row(row);
      lcd_putc(columns & 0x80 ? '1' : '0');
      lcd_putc(columns & 0x40 ? '1' : '0');
      lcd_putc(columns & 0x20 ? '1' : '0');
      lcd_putc(columns & 0x10 ? '1' : '0');
      lcd_putc(columns & 0x08 ? '1' : '0');
      lcd_putc(columns & 0x04 ? '1' : '0');
      lcd_putc(columns & 0x02 ? '1' : '0');
      lcd_putc(columns & 0x01 ? '1' : '0');
    }
  }
}

unsigned char led = 0;
unsigned long next_blink_millis = 0;

/**
 * Blink the led.
 */
void debug_blink() {
  if (time_millis() > next_blink_millis) {
    next_blink_millis = time_millis() + 500;
    led_set(led);
    led = !led;
  }
}

