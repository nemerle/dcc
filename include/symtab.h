/*
 * Symbol table prototypes
 * (C) Mike van Emmerik
*/
#pragma once
/* * * * * * * * * * * * * * * * * */
/* Symbol table structs and protos */
/* * * * * * * * * * * * * * * * * */
struct Function;
struct SYMTABLE
{
    std::string pSymName;              /* Ptr to symbolic name or comment */
    dword   symOff;                 /* Symbol image offset */
    Function *symProc;             /* Procedure pointer */
    word    preHash;                /* Hash value before the modulo */
    word    postHash;               /* Hash value after the modulo */
    word    nextOvf;                /* Next entry this hash bucket, or -1 */
    word    prevOvf;                /* Back link in Ovf chain */
    SYMTABLE() : symOff(0),symProc(0) {}
    SYMTABLE(dword _sym,Function *_proc) : symOff(_sym),symProc(_proc)
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
void    enterSym(char *symName, dword   symOff, Function *symProc, boolT bSymToo);
boolT   readSym (char *symName, dword *pSymOff, Function **pSymProc);
boolT   readVal (char *symName, dword   symOff, Function *symProc);
void    deleteSym(char *symName);
void    deleteVal(dword symOff, Function * symProc, boolT bSymToo);
std::string findVal(dword symOff, Function * symProc, word *pIndex);
word    symHash(char *name, word *pre);
word    valHash(dword off, Function * proc, word *pre);
void    selectTable(tableType);     /* Select a particular table */

char   *addStrTbl(char *pStr);      /* Add string to string table */

