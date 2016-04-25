/*
 ***************************************************************************
 *          dcc project general header
 * (C) Cristina Cifuentes, Mike van Emmerik
 ***************************************************************************
*/
#pragma once
#include "Enums.h"
#include "msvc_fixes.h"

#include <cassert>
#include <stdint.h>
#include <stdlib.h>

/**** Common definitions and macros ****/
#define MAX 0x7FFFFFFF

/* Type definitions used in the program */

#define SYNTHESIZED_MIN 0x100000    /* Synthesized labs use bits 21..32 */

/* These are for C library signature detection */
#define SYMLEN  16                  /* Length of proc symbols, incl null */
#define PATLEN  23                  /* Length of proc patterns  */
#define WILD    0xF4                /* The wild byte            */

/* MACROS */

// Macro reads a LH word from the image regardless of host convention
// Returns a 16 bit quantity, e.g. C000 is read into an Int as C000
//#define LH(p)  ((int16)((byte *)(p))[0] + ((int16)((byte *)(p))[1] << 8))
#define LH(p) ((uint16_t)((uint8_t *)(p))[0]  + ((uint16_t)((uint8_t *)(p))[1] << 8))


/* Macro reads a LH word from the image regardless of host convention */
/* Returns a signed quantity, e.g. C000 is read into an Int as FFFFC000 */
#define LH_SIGNED(p) (((uint8_t *)(p))[0] + (((char *)(p))[1] << 8))

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
    uint8_t def :1; //!< Variable was first defined than used
    uint8_t use :1; //!< Variable was first used than defined
    uint8_t val :1;      /* Variable has an initial value.  2 cases:
                             1. When variable is used first (ie. global)
                             2. When a value is moved into the variable
                                for the first time.
                        */
    void setFlags(uint16_t x)
    {
        def = x&DEF;
        use = x&USE;
        val = x&VAL;
    }
    bool isUSE_VAL() {return use and val;}  //Use and Val
};
static constexpr const char * hlTypes[13] = {
    "",
    "char",
    "unsigned char",
    "int",
    "unsigned int",
    "long",
    "unsigned long",
    "record",
    "int *",
    "char *",
    "",
    "float",
    "double"
};

struct TypeContainer
{
    hlType m_type;
    size_t m_size;
    TypeContainer(hlType t,size_t sz) : m_type(t),m_size(sz)
    {
    }
    static size_t typeSize(hlType t)
    {
        switch(t)
        {
            case TYPE_WORD_SIGN: case TYPE_WORD_UNSIGN:
            return 2;
            case TYPE_BYTE_SIGN: case TYPE_BYTE_UNSIGN:
            return 1;
            case TYPE_LONG_SIGN:  case TYPE_LONG_UNSIGN:
                return 4;
            case TYPE_FLOAT:
                return 4;
            default:
                return ~0;
        }
        return 0;
    }
    static hlType defaultTypeForSize(size_t x)
    {
        /* Type of the symbol according to the number of bytes it uses */
        static hlType cbType[] = {TYPE_UNKNOWN, TYPE_BYTE_UNSIGN, TYPE_WORD_SIGN,
                                  TYPE_UNKNOWN, TYPE_LONG_SIGN};

        assert(x < sizeof(cbType)/sizeof(hlType));
        return cbType[x];
    }
    static constexpr const char *typeName(hlType t)
    {
        return hlTypes[t];
    }
};
