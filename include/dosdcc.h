/*

=**************************************************************************
 * File     : dosdcc.h
 * Purpose  : include file for files decompiled by dcc.
 * Copyright (c) Cristina Cifuentes - QUT - 1992
 *************************************************************************
*/

/* Type definitions for intel 80x86 architecture */
typedef unsigned int    uint16_t;       /* 16 bits */
typedef unsigned char   uint8_t;       /* 8 bits  */
typedef union {
    unsigned long   dW;
    uint16_t            wL, wH;         /* 2 words */
} Dword;                            /* 32 bits */

/* Structure to access high and low bits of a uint8_t or uint16_t variable */
typedef struct {
    /* low  uint8_t */
    uint16_t    lowBitWord  : 1;
    uint16_t    filler1     : 6;
    uint16_t    highBitByte : 1;
    /* high uint8_t */
    uint16_t    lowBitByte  : 1;
    uint16_t    filler2     : 6;
    uint16_t    highBitWord : 1;
} wordBits;

/* Low and high bits of a uint8_t or uint16_t variable */
#define lowBit(a)       ((wordBits)(a).lowBitWord)
#define highBitByte(a)  ((wordBits)(a).highBitByte)
#define lowBitByte(a)   ((wordBits)(a).lowBitByte)
#define highBit(a)      (sizeof(a) == sizeof(uint16_t) ? \
                        ((wordBits)(a).highBitWord):\
                        ((wordBits)(a).highBitByte))

/* uint16_t register variables */
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

/* uint8_t register variables */
#define ah      regs.h.ah
#define al      regs.h.al
#define bh      regs.h.bh
#define bl      regs.h.bl
#define ch      regs.h.ch
#define cl      regs.h.cl
#define dh      regs.h.dh
#define dl      regs.h.dl


/* High and low words of a Dword */
#define highWord(w)     (*((uint16_t*)&(w) + 1))
#define lowWord(w)      ((uint16_t)(w))

#define MAXByte     0xFF
#define MAXWord     0xFFFF
#define MAXSignByte 0x7F
#define MINSignByte 0x81
#define MAXSignWord 0x7FFF
#define MINSignWord 0x8001


