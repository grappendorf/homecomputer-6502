#ifndef _INTERRUPT_H
#define _INTERRUPT_H

extern unsigned long interrupted;
#pragma zpsym("interrupted");
#define is_interrupted() interrupted

#define reset_interrupted() interrupted = 0

#endif
