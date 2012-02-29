/****************************************************************************
 *          dcc project general header
 * (C) Cristina Cifuentes, Mike van Emmerik
 ****************************************************************************/
#pragma once
#include <stdint.h>
/**** Common definitions and macros ****/
typedef unsigned int uint32_t;  /* 32 bits  */
#define MAX 0x7FFFFFFF

/* Type definitions used in the program */
typedef unsigned char byte; /* 8 bits   */
typedef unsigned short word;/* 16 bits  */
typedef short int16;        /* 16 bits  */
typedef unsigned char boolT; /* 8 bits   */

#if defined(__MSDOS__) | defined(WIN32)
#define unlink _unlink		// Compiler is picky about non Ansi names
#endif


#define TRUE    1
#define FALSE   0

#define SYNTHESIZED_MIN 0x100000    /* Synthesized labs use bits 21..32 */

/* These are for C library signature detection */
#define SYMLEN  16                  /* Length of proc symbols, incl null */
#define PATLEN  23                  /* Length of proc patterns  */
#define WILD    0xF4                /* The wild byte            */

/****** MACROS *******/

/* Macro reads a LH word from the image regardless of host convention */
/* Returns a 16 bit quantity, e.g. C000 is read into an Int as C000 */
//#define LH(p)  ((int16)((byte *)(p))[0] + ((int16)((byte *)(p))[1] << 8))
#define LH(p)    ((uint16_t)((uint8_t *)(p))[0]  + ((uint16_t)((uint8_t *)(p))[1] << 8))

/* Macro reads a LH word from the image regardless of host convention */
/* Returns a signed quantity, e.g. C000 is read into an Int as FFFFC000 */
#define LH_SIGNED(p) (((byte *)(p))[0] + (((char *)(p))[1] << 8))

/* Macro tests bit b for type t in prog.map */
#define BITMAP(b, t)  (prog.map[(b) >> 2] & ((t) << (((b) & 3) << 1)))

/* Macro to convert a segment, offset definition into a 20 bit address */
#define opAdr(seg,off)  ((seg << 4) + off)

/* duVal FLAGS */
struct eDuVal
{
    eDuVal()
    {
        def=use=val=0;
    }
    enum flgs
    {
        DEF=1,
        USE=2,
        VAL=4
    };
    int def :1; /* Variable was first defined than used          */
    int use :1;      /* Variable was first used than defined          */
    int val :1;      /* Variable has an initial value.  2 cases:
                    * 1. When variable is used first (ie. global)
                    * 2. When a value is moved into the variable
                    *    for the first time.                        */
    void setFlags(uint16_t x)
    {
        def = x&DEF;
        use = x&USE;
        val = x&VAL;
    }
    bool isUSE_VAL() {return use&&val;}            /* Use and Val                                   */
};
