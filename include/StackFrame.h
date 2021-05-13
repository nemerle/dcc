#pragma once
#include "types.h"
#include "Enums.h"
#include "symtab.h"

struct STKFRAME : public SymbolTableCommon<STKSYM>
{
    //std::vector<STKSYM> sym;
    //STKSYM *    sym;        /* Symbols                      */
    int16_t       m_minOff=0;     /* Initial offset in stack frame*/
    int16_t       maxOff=0;     /* Maximum offset in stack frame*/
    int         cb=0;         /* Number of bytes in arguments */
    int         numArgs=0;    /* No. of arguments in the table*/

    void   adjustForArgType(size_t numArg_, hlType actType_);
    size_t getLocVar(int off);
public:
    void updateFrameOff(int16_t off, int size, uint16_t duFlag);
};
