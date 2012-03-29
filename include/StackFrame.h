#pragma once
#include <vector>
#include <cstring>
#include "types.h"
#include "Enums.h"
#include "symtab.h"

struct STKFRAME : public SymbolTableCommon<STKSYM>
{
    //std::vector<STKSYM> sym;
    //STKSYM *    sym;        /* Symbols                      */
    int16_t       m_minOff;     /* Initial offset in stack frame*/
    int16_t       maxOff;     /* Maximum offset in stack frame*/
    int         cb;         /* Number of bytes in arguments */
    int         numArgs;    /* No. of arguments in the table*/
    void        adjustForArgType(size_t numArg_, hlType actType_);
    STKFRAME() : m_minOff(0),maxOff(0),cb(0),numArgs(0)
    {

    }
    size_t getLocVar(int off);
public:
    void updateFrameOff(int16_t off, int size, uint16_t duFlag);
};
