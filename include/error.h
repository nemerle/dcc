/*****************************************************************************
 * Error codes
 * (C) Cristina Cifuentes
 ****************************************************************************/
#pragma once

/* These definitions refer to errorMessage in error.c */
enum eErrorId
{
    NO_ERR              =0,
    USAGE,
    INVALID_ARG,
    INVALID_OPCODE,
    INVALID_386OP,
    FUNNY_SEGOVR,
    FUNNY_REP,
    CANNOT_OPEN,
    CANNOT_READ,
    MALLOC_FAILED,
    NEWEXE_FORMAT,

    NO_BB,
    INVALID_SYNTHETIC_BB,
    INVALID_INT_BB,
    IP_OUT_OF_RANGE,
    DEF_NOT_FOUND,
    JX_NOT_DEF,
    NOT_DEF_USE,
    REPEAT_FAIL,
    WHILE_FAIL
};

//lint -function(exit,fatalError)
void fatalError(eErrorId errId, ...);
void reportError(eErrorId errId, ...);

