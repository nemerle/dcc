/*
 * Symbol table prototypes
 * (C) Mike van Emmerik
*/
#pragma once
#include <string>
#include <stdint.h>
#include "Enums.h"
#include "types.h"
struct COND_EXPR;
struct TypeContainer;
/* * * * * * * * * * * * * * * * * */
/* Symbol table structs and protos */
/* * * * * * * * * * * * * * * * * */
struct Function;
struct SYMTABLE
{
    std::string pSymName;              /* Ptr to symbolic name or comment */
    uint32_t   symOff;                 /* Symbol image offset */
    Function *symProc;             /* Procedure pointer */
    SYMTABLE() : symOff(0),symProc(0) {}
    SYMTABLE(uint32_t _sym,Function *_proc) : symOff(_sym),symProc(_proc)
    {}
    bool operator == (const SYMTABLE &other) const
    {
        // does not yse pSymName, to ease finding by symOff/symProc combo
        // in map<SYMTABLE,X>
        return (symOff==other.symOff) && symProc==(other.symProc);
    }
};

enum tableType                     /* The table types */
{
    Label=0,                        /* The label table */
    Comment,                        /* The comment table */
    NUM_TABLE_TYPES                 /* Number of entries: must be last */
};

void    createSymTables(void);
void    destroySymTables(void);
boolT   readVal (std::ostringstream &symName, uint32_t   symOff, Function *symProc);
void    selectTable(tableType);     /* Select a particular table */

