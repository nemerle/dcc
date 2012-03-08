#pragma once
#include <vector>
#include <cstring>
#include "types.h"
#include "Enums.h"
struct COND_EXPR;
/* STACK FRAME */
struct STKSYM
{
    COND_EXPR	*actual;	/* Expression tree of actual parameter 		*/
    COND_EXPR 	*regs;		/* For register arguments only				*/
    int16_t       off;        /* Immediate off from BP (+:args, -:params) */
    uint8_t        regOff;     /* Offset is a register (e.g. SI, DI)       */
    int         size;       /* Size             						*/
    hlType      type;       /* Probable type    						*/
    eDuVal      duVal;      /* DEF, USE, VAL    						*/
    boolT       hasMacro;	/* This type needs a macro					*/
    char        macro[10];	/* Macro name								*/
    char        name[10];   /* Name for this symbol/argument            */
    bool        invalid;	/* Boolean: invalid entry in formal arg list*/
    STKSYM()
    {
        memset(this,0,sizeof(STKSYM));
    }
};

struct STKFRAME
{
    std::vector<STKSYM> sym;
    //STKSYM *    sym;        /* Symbols                      */
    int16_t       m_minOff;     /* Initial offset in stack frame*/
    int16_t       maxOff;     /* Maximum offset in stack frame*/
    int         cb;         /* Number of bytes in arguments */
    int         numArgs;    /* No. of arguments in the table*/
    void        adjustForArgType(int numArg_, hlType actType_);
    STKFRAME() : sym(0),m_minOff(0),maxOff(0),cb(0),numArgs(0)
    {

    }
    size_t getLocVar(int off);
};
