/****************************************************************************
 *          dcc project general header
 * (C) Cristina Cifuentes, Mike van Emmerik
 ****************************************************************************/
#pragma once
//TODO: Remove boolT

#include <utility>
#include <algorithm>
#include <bitset>
#include <QtCore/QString>

#include "Enums.h"
#include "types.h"
#include "ast.h"
#include "icode.h"
#include "locident.h"
#include "error.h"
#include "graph.h"
#include "bundle.h"
#include "Procedure.h"
#include "BasicBlock.h"
class Project;
/* CALL GRAPH NODE */
extern bundle cCode;			/* Output C procedure's declaration and code */

/**** Global variables ****/

extern QString asm1_name, asm2_name; /* Assembler output filenames 		*/

/** Command line option flags */
struct OPTION
{
    bool verbose;
    bool VeryVerbose;
    bool asm1;          /* Early disassembly listing */
    bool asm2;          /* Disassembly listing after restruct */
    bool Map;
    bool Stats;
    bool Interact;      /* Interactive mode */
    bool Calls;         /* Follow register indirect calls */
    QString	filename;			/* The input filename */
    uint32_t CustomEntryPoint;
};

extern OPTION option;       /* Command line options             */

#include "BinaryImage.h"

/* Memory map states */
enum eAreaType
{
    BM_UNKNOWN = 0,   /* Unscanned memory     */
    BM_DATA =    1,   /* Data                 */
    BM_CODE =    2,   /* Code                 */
    BM_IMPURE =  3   /* Used as Data and Code*/
};

/* Intermediate instructions statistics */
struct STATS
{
        int		numBBbef;       /* number of basic blocks initially 	       */
        int		numBBaft;       /* number of basic blocks at the end 	       */
        int		nOrder;         /* n-th order								   */
        int		numLLIcode;     /* number of low-level Icode instructions      */
        int		numHLIcode; 	/* number of high-level Icode instructions     */
        int		totalLL;        /* total number of low-level Icode insts       */
        int		totalHL;        /* total number of high-level Icod insts       */
};

extern STATS stats; /* Icode statistics */


/**** Global function prototypes ****/

void    udm(void);                                          /* udm.c        */
void    freeCFG(BB * cfg);                                  /* graph.c      */
BB *    newBB(BB *, int, int, uint8_t, int, Function *);    /* graph.c      */
void    BackEnd(CALL_GRAPH *);              /* backend.c    */
extern char   *cChar(uint8_t c);                            /* backend.c    */
eErrorId scan(uint32_t ip, ICODE &p);                       /* scanner.c    */
void    parse (CALL_GRAPH * *);                             /* parser.c     */

extern int     strSize (const uint8_t *, char);             /* parser.c     */
//void    disassem(int pass, Function * pProc);             /* disassem.c   */
void    interactDis(Function *, int initIC);       /* disassem.c   */
bool    JmpInst(llIcode opcode);                            /* idioms.c     */
queue::iterator  appendQueue(queue &Q, BB *node);           /* reducible.c  */

bool    SetupLibCheck(void);                                /* chklib.c     */
void    CleanupLibCheck(void);                              /* chklib.c     */
bool    LibCheck(Function &p);                              /* chklib.c     */


/* Exported functions from hlicode.c */
QString writeJcond(const HLTYPE &, Function *, int *);
QString writeJcondInv(HLTYPE, Function *, int *);


/* Exported funcions from locident.c */
bool checkLongEq(LONG_STKID_TYPE, iICODE, int, Function *, Assignment &asgn, LLInst &atOffset);
bool checkLongRegEq(LONGID_TYPE, iICODE, int, Function *, Assignment &asgn, LLInst &);

extern const char *indentStr(int level);
