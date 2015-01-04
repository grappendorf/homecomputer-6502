#ifndef _UTILS_H
#define _UTILS_H

extern void __fastcall__ delay_ms(unsigned char delay);

extern unsigned long millis;
#pragma zpsym("millis");
#define time_millis() millis

extern unsigned char jiffies;
#pragma zpsym("jiffies");
#define time_jiffies() ((int) jiffies)

extern unsigned char seconds;
#pragma zpsym("seconds");
#define time_seconds() ((int) seconds)

extern unsigned char minutes;
#pragma zpsym("minutes");
#define time_minutes() ((int) minutes)

extern unsigned char hours;
#pragma zpsym("hours");
#define time_hours() ((int)hours)

#endif
