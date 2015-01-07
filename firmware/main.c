#include <stdio.h>
#include <stdlib.h>
#include "acia.h"
#include "keys.h"
#include "led.h"
#include "lcd.h"
#include "basic.h"
#include "readline.h"

int main() {

  acia_init();
  lcd_init();
  led_init();
  keys_init();

  acia_puts("6502 HomeComputer ready.\n");
  lcd_puts("6502 HomeComputer ready!\n");
  cmd_free(NULL);
  lcd_cursor_on();
  lcd_cursor_blink();

  for (;;) {
    char * line = readline(NON_INTERRUPTIBLE);
    interpret(line);
  }

  return 0;
}