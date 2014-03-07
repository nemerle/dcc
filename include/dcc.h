/****************************************************************************
 *          dcc project general header
 * (C) Cristina Cifuentes, Mike van Emmerik
 ****************************************************************************/
#pragma once
//TODO: Remove boolT

#include <llvm/ADT/ilist.h>
#include <utility>
#include <algorithm>
#include <bitset>

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

extern std::string asm1_name, asm2_name; /* Assembler output filenames 		*/

typedef struct {            /* Command line option flags */
    unsigned verbose        : 1;
    unsigned VeryVerbose    : 1;
    unsigned asm1           : 1;    /* Early disassembly listing */
    unsigned asm2           : 1;    /* Disassembly listing after restruct */
    unsigned Map            : 1;
    unsigned Stats          : 1;
    unsigned Interact       : 1;    /* Interactive mode */
    unsigned Calls          : 1;    /* Follow register indirect calls */
    std::string	filename;			/* The input filename */
} OPTION;

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
class DccFrontend
{
    void    LoadImage(Project &proj);
    void    parse(Project &proj);
    std::string m_fname;
public:
    DccFrontend(const std::string &fname) : m_fname(fname)
    {
    }
    bool FrontEnd();            /* frontend.c   */
};

void    udm(void);                                          /* udm.c        */
void    freeCFG(BB * cfg);                                  /* graph.c      */
BB *    newBB(BB *, int, int, uint8_t, int, Function *);    /* graph.c      */
void    BackEnd(const std::string &filename, CALL_GRAPH *);              /* backend.c    */
extern char   *cChar(uint8_t c);                            /* backend.c    */
eErrorId scan(uint32_t ip, ICODE &p);                       /* scanner.c    */
void    parse (CALL_GRAPH * *);                             /* parser.c     */

extern int     strSize (const uint8_t *, char);             /* parser.c     */
//void    disassem(int pass, Function * pProc);             /* disassem.c   */
void    interactDis(Function *, int initIC);       /* disassem.c   */
bool    JmpInst(llIcode opcode);                            /* idioms.c     */
queue::iterator  appendQueue(queue &Q, BB *node);           /* reducible.c  */

void    SetupLibCheck(void);                                /* chklib.c     */
void    CleanupLibCheck(void);                              /* chklib.c     */
bool    LibCheck(Function &p);                              /* chklib.c     */


/* Exported functions from hlicode.c */
const char *writeJcond(const HLTYPE &, Function *, int *);
const char  *writeJcondInv (HLTYPE, Function *, int *);


/* Exported funcions from locident.c */
bool checkLongEq(LONG_STKID_TYPE, iICODE, int, Function *, Assignment &asgn, LLInst &atOffset);
bool checkLongRegEq(LONGID_TYPE, iICODE, int, Function *, Assignment &asgn, LLInst &);
eReg otherLongRegi(eReg, int, LOCAL_ID *);


extern const char *indentStr(int level);
