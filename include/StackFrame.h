#pragma once
#include "types.h"
#include "ast.h"
#include "icode.h"
#include "locident.h"
#include "error.h"
#include "graph.h"
#include "bundle.h"

/* STACK FRAME */
struct STKSYM
{
    COND_EXPR	*actual;	/* Expression tree of actual parameter 		*/
    COND_EXPR 	*regs;		/* For register arguments only				*/
    int16       off;        /* Immediate off from BP (+:args, -:params) */
    byte        regOff;     /* Offset is a register (e.g. SI, DI)       */
    Int         size;       /* Size             						*/
    hlType      type;       /* Probable type    						*/
    eDuVal      duVal;      /* DEF, USE, VAL    						*/
    boolT       hasMacro;	/* This type needs a macro					*/
    char        macro[10];	/* Macro name								*/
    char        name[10];   /* Name for this symbol/argument            */
    boolT       invalid;	/* Boolean: invalid entry in formal arg list*/
    STKSYM()
    {
        memset(this,0,sizeof(STKSYM));
    }
};

struct STKFRAME
{
    std::vector<STKSYM> sym;
    //STKSYM *    sym;        /* Symbols                      */
    int16       minOff;     /* Initial offset in stack frame*/
    int16       maxOff;     /* Maximum offset in stack frame*/
    Int         cb;         /* Number of bytes in arguments */
    Int         numArgs;    /* No. of arguments in the table*/
    void        adjustForArgType(Int numArg_, hlType actType_);
    STKFRAME() : sym(0),minOff(0),maxOff(0),cb(0),numArgs(0)
    {

    }
    Int getLocVar(Int off);
};
