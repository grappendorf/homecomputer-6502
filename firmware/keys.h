#ifndef _KEYS_H
#define _KEYS_H

extern void keys_init();
extern void keys_update();
extern char keys_getc();
extern unsigned char keys_get_code();
extern unsigned char keys_get_modifiers();
extern unsigned char keys_read_row(unsigned char row);

#define MODIFIER_SHIFT    1
#define MODIFIER_CTRL     2
#define MODIFIER_ALT      4

#define KEY_ESC           5
#define KEY_F1            103
#define KEY_F2            10
#define KEY_F3            15
#define KEY_F4            13
#define KEY_F5            77
#define KEY_F6            37
#define KEY_F7            87
#define KEY_F8            101
#define KEY_F9            74
#define KEY_F10           72
#define KEY_F11           96
#define KEY_F12           64
#define KEY_NUM           102
#define KEY_PRINT         48
#define KEY_SCROLL        51
#define KEY_BREAK         62
#define KEY_BACKSPACE     79
#define KEY_HOME          58
#define KEY_TAB           7
#define KEY_PAGE_UP       82
#define KEY_PAGE_DOWN     80
#define KEY_END           56
#define KEY_MENU          63
#define KEY_INSERT        66
#define KEY_DELETE        98
#define KEY_CURSOR_UP     61
#define KEY_CURSOR_LEFT   60
#define KEY_CURSOR_DOWN   100
#define KEY_CURSOR_RIGHT  68
#define KEY_HAT           2
#define KEY_1             0
#define KEY_2             97
#define KEY_3             8
#define KEY_4             16
#define KEY_5             18
#define KEY_6             26
#define KEY_7             24
#define KEY_8             32
#define KEY_9             99
#define KEY_0             40
#define KEY_A             1
#define KEY_B             20
#define KEY_C             14
#define KEY_D             9
#define KEY_E             11
#define KEY_F             17
#define KEY_G             21
#define KEY_H             29
#define KEY_I             35
#define KEY_J             25
#define KEY_K             33
#define KEY_L             81
#define KEY_M             30
#define KEY_N             28
#define KEY_O             83
#define KEY_P             43
#define KEY_Q             3
#define KEY_R             19
#define KEY_S             65
#define KEY_T             23
#define KEY_U             27
#define KEY_V             22
#define KEY_W             67
#define KEY_X             70
#define KEY_Y             6
#define KEY_Z             31
#define KEY_MINUS         42
#define KEY_EQUAL         34
#define KEY_LEFT_BRACKET  47
#define KEY_RIGHT_BRACKET 39
#define KEY_SEMICOLON     41
#define KEY_APOSTROPH     45
#define KEY_HASH          46
#define KEY_COMMA         38
#define KEY_DOT           86
#define KEY_SLASH         44
#define KEY_LESS          69
#define KEY_SPACE         76

#endif
