/*
 * Symbol table prototypes
 * (C) Mike van Emmerik
*/

/* * * * * * * * * * * * * * * * * */
/* Symbol table structs and protos */
/* * * * * * * * * * * * * * * * * */

typedef struct
{
    char    *pSymName;              /* Ptr to symbolic name or comment */
    dword   symOff;                 /* Symbol image offset */
    PPROC   symProc;                /* Procedure pointer */
    word    preHash;                /* Hash value before the modulo */
    word    postHash;               /* Hash value after the modulo */
    word    nextOvf;                /* Next entry this hash bucket, or -1 */
    word    prevOvf;                /* Back link in Ovf chain */
} SYMTABLE;

enum _tableType                     /* The table types */
{
    Label=0,                        /* The label table */
    Comment,                        /* The comment table */
    NUM_TABLE_TYPES                 /* Number of entries: must be last */
};              

typedef enum _tableType tableType;  /* For convenience */

void    createSymTables(void);
void    destroySymTables(void);
void    enterSym(char *symName, dword   symOff, PPROC   symProc, boolT bSymToo);
boolT   readSym (char *symName, dword *pSymOff, PPROC *pSymProc);
boolT   readVal (char *symName, dword   symOff, PPROC   symProc);
void    deleteSym(char *symName);
void    deleteVal(dword symOff, PPROC symProc, boolT bSymToo);
boolT   findVal(dword symOff, PPROC symProc, word *pIndex);
word    symHash(char *name, word *pre);
word    valHash(dword off, PPROC proc, word *pre);
void    selectTable(tableType);     /* Select a particular table */

char   *addStrTbl(char *pStr);      /* Add string to string table */

