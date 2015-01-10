#ifndef _LCD_H
#define _LCD_H

extern void lcd_init();
extern void __fastcall__ lcd_command(unsigned char cmd);
extern void __fastcall__ lcd_write(char c);
extern void __fastcall__ lcd_putc(char c);
extern void __fastcall__ lcd_puts(const char * s);
extern void lcd_put_newline();
extern void __fastcall__ lcd_goto(unsigned char x, unsigned char y);
#define lcd_home() lcd_goto(0, 0);
extern void lcd_clear();
extern void lcd_cursor_on();
extern void lcd_cursor_blink();
extern void lcd_cursor_off();
extern unsigned char lcd_get_x();
extern unsigned char lcd_get_y();

#endif
