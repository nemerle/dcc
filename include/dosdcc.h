/***************************************************************************
 * File     : dosdcc.h
 * Purpose  : include file for files decompiled by dcc.
 * Copyright (c) Cristina Cifuentes - QUT - 1992
 **************************************************************************/

/* Type definitions for intel 80x86 architecture */
typedef unsigned int    Word;       /* 16 bits */
typedef unsigned char   Byte;       /* 8 bits  */
typedef union {
    unsigned long   dW;
    Word            wL, wH;         /* 2 words */
} Dword;                            /* 32 bits */

/* Structure to access high and low bits of a Byte or Word variable */
typedef struct {
    /* low  byte */
    Word    lowBitWord  : 1;
    Word    filler1     : 6;
    Word    highBitByte : 1;
    /* high byte */ 
    Word    lowBitByte  : 1;
    Word    filler2     : 6;
    Word    highBitWord : 1;
} wordBits;

/* Low and high bits of a Byte or Word variable */
#define lowBit(a)       ((wordBits)(a).lowBitWord)
#define highBitByte(a)  ((wordBits)(a).highBitByte)
#define lowBitByte(a)   ((wordBits)(a).lowBitByte)
#define highBit(a)      (sizeof(a) == sizeof(Word) ? \
                        ((wordBits)(a).highBitWord):\
                        ((wordBits)(a).highBitByte))

/* Word register variables */
#define ax      regs.x.ax
#define bx      regs.x.bx
#define cx      regs.x.cx
#define dx      regs.x.dx

#define cs      regs.x.cs
#define es      regs.x.es
#define ds      regs.x.ds
#define ss      regs.x.ss

#define si      regs.x.si
#define di      regs.x.di
#define bp      regs.x.bp
#define sp      regs.x.sp

/* getting rid of all flags */
#define carry       regs.x.cflags
#define overF       regs.x.flags    /***** check *****/

/* Byte register variables */
#define ah      regs.h.ah
#define al      regs.h.al
#define bh      regs.h.bh
#define bl      regs.h.bl
#define ch      regs.h.ch
#define cl      regs.h.cl
#define dh      regs.h.dh
#define dl      regs.h.dl


/* High and low words of a Dword */
#define highWord(w)     (*((Word*)&(w) + 1))
#define lowWord(w)      ((Word)(w))

#define MAXByte     0xFF
#define MAXWord     0xFFFF
#define MAXSignByte 0x7F
#define MINSignByte 0x81
#define MAXSignWord 0x7FFF
#define MINSignWord 0x8001


