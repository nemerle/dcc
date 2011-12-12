/****************************************************************************
 *          dcc project general header
 * (C) Cristina Cifuentes, Mike van Emmerik
 ****************************************************************************/
#pragma once
#include <llvm/ADT/ilist.h>
#include "types.h"
#include "ast.h"
#include "icode.h"
#include "locident.h"
#include "error.h"
#include "graph.h"
#include "bundle.h"
#include "Procedure.h"
#include "BasicBlock.h"

typedef llvm::iplist<Function> FunctionListType;
typedef FunctionListType lFunction;
typedef lFunction::iterator ilFunction;


/* SYMBOL TABLE */
struct SYM {
    char        name[10];   /* New name for this variable   */
    dword       label;      /* physical address (20 bit)    */
    Int         size;       /* maximum size                 */
    flags32     flg;        /* SEG_IMMED, IMPURE, WORD_OFF  */
    hlType      type;       /* probable type                */
    eDuVal      duVal;      /* DEF, USE, VAL                */
};

struct SYMTAB
{
    Int         csym;       /* No. of symbols in table  */
    Int         alloc;      /* Allocation               */
    SYM *        sym;        /* Symbols                  */
};

/* CALL GRAPH NODE */
struct CALL_GRAPH
{
        ilFunction proc;               /* Pointer to procedure in pProcList	*/
        std::vector<CALL_GRAPH *> outEdges; /* array of out edges                   */
public:
        void write();
        CALL_GRAPH() : outEdges(0)
        {
        }
public:
        void writeNodeCallGraph(Int indIdx);
        boolT insertCallGraph(ilFunction caller, ilFunction callee);
        boolT insertCallGraph(Function *caller, ilFunction callee);
        void insertArc(ilFunction newProc);
};
#define NUM_PROCS_DELTA		5		/* delta # procs a proc invokes		 	*/
//extern std::list<Function> pProcList;
extern FunctionListType pProcList;
extern CALL_GRAPH * callGraph;	/* Pointer to the head of the call graph     */
extern bundle cCode;			/* Output C procedure's declaration and code */

/* Procedure FLAGS */
enum PROC_FLAGS
{
    PROC_BADINST=0x000100,/* Proc contains invalid or 386 instruction */
    PROC_IJMP   =0x000200,/* Proc incomplete due to indirect jmp	 	*/
    PROC_ICALL  =0x000400, /* Proc incomplete due to indirect call		*/
    PROC_HLL=0x001000, /* Proc is likely to be from a HLL			*/
    CALL_PASCAL=0x002000, /* Proc uses Pascal calling convention		*/
    CALL_C=0x004000, /* Proc uses C calling convention			*/
    CALL_UNKNOWN=0x008000, /* Proc uses unknown calling convention		*/
    PROC_NEAR=0x010000, /* Proc exits with near return				*/
    PROC_FAR=0x020000, /* Proc exits with far return				*/
    GRAPH_IRRED=0x100000, /* Proc generates an irreducible graph		*/
    SI_REGVAR=0x200000, /* SI is used as a stack variable 			*/
    DI_REGVAR=0x400000, /* DI is used as a stack variable 			*/
    PROC_IS_FUNC=0x800000,	/* Proc is a function 						*/
    REG_ARGS=0x1000000, /* Proc has registers as arguments			*/
    PROC_VARARG=0x2000000,	/* Proc has variable arguments				*/
    PROC_OUTPUT=0x4000000, /* C for this proc has been output 			*/
    PROC_RUNTIME=0x8000000, /* Proc is part of the runtime support		*/
    PROC_ISLIB=0x10000000, /* Proc is a library function				*/
    PROC_ASM=0x20000000, /* Proc is an intrinsic assembler routine   */
    PROC_IS_HLL=0x40000000 /* Proc has HLL prolog code					*/
};
#define CALL_MASK    0xFFFF9FFF /* Masks off CALL_C and CALL_PASCAL		 	*/




/**** Global variables ****/

extern char *asm1_name, *asm2_name; /* Assembler output filenames 		*/

typedef struct {            /* Command line option flags */
    unsigned verbose        : 1;
    unsigned VeryVerbose    : 1;
    unsigned asm1           : 1;    /* Early disassembly listing */
    unsigned asm2           : 1;    /* Disassembly listing after restruct */
    unsigned Map            : 1;
    unsigned Stats          : 1;
    unsigned Interact       : 1;    /* Interactive mode */
    unsigned Calls          : 1;    /* Follow register indirect calls */
    char	filename[80];			/* The input filename */
} OPTION;

extern OPTION option;       /* Command line options             */
extern SYMTAB symtab;       /* Global symbol table              */

struct PROG /* Loaded program image parameters  */
{
    int16       initCS;
    int16       initIP;     /* These are initial load values    */
    int16       initSS;     /* Probably not of great interest   */
    int16       initSP;
    boolT       fCOM;       /* Flag set if COM program (else EXE)*/
    Int         cReloc;     /* No. of relocation table entries  */
    dword      *relocTable; /* Ptr. to relocation table         */
    byte       *map;        /* Memory bitmap ptr                */
    Int         cProcs;     /* Number of procedures so far      */
    Int         offMain;    /* The offset  of the main() proc   */
    word        segMain;    /* The segment of the main() proc   */
    boolT       bSigs;		/* True if signatures loaded		*/
    Int         cbImage;    /* Length of image in bytes         */
    byte       *Image;      /* Allocated by loader to hold entire
                             * program image                    */
};

extern PROG prog;   		/* Loaded program image parameters  */
extern char condExp[200];	/* Conditional expression buffer 	*/
extern char callBuf[100];	/* Function call buffer				*/
extern dword duReg[30];		/* def/use bits for registers		*/
extern dword maskDuReg[30];	/* masks off du bits for regs		*/

/* Registers used by icode instructions */
static const char *allRegs[21] = {"ax", "cx", "dx", "bx", "sp", "bp",
                                    "si", "di", "es", "cs", "ss", "ds",
                                    "al", "cl", "dl", "bl", "ah", "ch", "dh", "bh",
                                    "tmp"};

/* Memory map states */
#define BM_UNKNOWN  0   /* Unscanned memory     */
#define BM_DATA     1   /* Data                 */
#define BM_CODE     2   /* Code                 */
#define BM_IMPURE   3   /* Used as Data and Code*/

/* Intermediate instructions statistics */
struct STATS
{
        Int		numBBbef;		/* number of basic blocks initially 	       */
        Int		numBBaft;		/* number of basic blocks at the end 	       */
        Int		nOrder;			/* n-th order								   */
        Int		numLLIcode;		/* number of low-level Icode instructions      */
        Int		numHLIcode; 	/* number of high-level Icode instructions     */
        Int		totalLL;		/* total number of low-level Icode insts       */
        Int		totalHL;		/* total number of high-level Icod insts       */
};

extern STATS stats; /* Icode statistics */


/**** Global function prototypes ****/

void    FrontEnd(char *filename, CALL_GRAPH * *);            /* frontend.c   */
void   *allocMem(Int cb);                                   /* frontend.c   */
void   *reallocVar(void *p, Int newsize);                   /* frontend.c   */
void    udm(void);                                          /* udm.c        */
void    freeCFG(BB * cfg);                                  /* graph.c      */
BB *    newBB(BB *, Int, Int, byte, Int, Function *);      /* graph.c      */
void    BackEnd(char *filename, CALL_GRAPH *);              /* backend.c    */
char   *cChar(byte c);                                      /* backend.c    */
Int     scan(dword ip, ICODE * p);                          /* scanner.c    */
void    parse (CALL_GRAPH * *);                             /* parser.c     */
boolT   labelSrch(ICODE * pIc, Int n, dword tg, Int *pIdx); /* parser.c     */
Int     strSize (byte *, char);                             /* parser.c     */
void    disassem(Int pass, Function * pProc);              /* disassem.c   */
void    interactDis(Function * initProc, Int initIC);      /* disassem.c   */
boolT   JmpInst(llIcode opcode);                            /* idioms.c     */
queue::iterator  appendQueue(queue &Q, BB *node);                  /* reducible.c  */

void    SetupLibCheck(void);                                /* chklib.c     */
void    CleanupLibCheck(void);                              /* chklib.c     */
boolT   LibCheck(Function &p);                            /* chklib.c     */

/* Exported functions from procs.c */
boolT	insertCallGraph (CALL_GRAPH *, ilFunction, ilFunction);
boolT 	newStkArg (ICODE *, COND_EXPR *, llIcode, Function *);
void	allocStkArgs (ICODE *, Int);
void	placeStkArg (ICODE *, COND_EXPR *, Int);
void	adjustActArgType (COND_EXPR *, hlType, Function *);

/* Exported functions from ast.c */
void	  removeRegFromLong (byte, LOCAL_ID *, COND_EXPR *);
std::string walkCondExpr (const COND_EXPR *exp, Function * pProc, Int *);
Int       hlTypeSize (const COND_EXPR *, Function *);
hlType	  expType (const COND_EXPR *, Function *);
boolT	  insertSubTreeReg (COND_EXPR *, COND_EXPR **, byte, LOCAL_ID *);
boolT	  insertSubTreeLongReg (COND_EXPR *, COND_EXPR **, Int);
//COND_EXPR *concatExps (SEQ_COND_EXPR *, COND_EXPR *, condNodeType);

void	  initExpStk();
void	  pushExpStk (COND_EXPR *);
COND_EXPR *popExpStk();
Int       numElemExpStk();
boolT	  emptyExpStk();

/* Exported functions from hlicode.c */
std::string writeCall (Function *, STKFRAME *, Function *, Int *);
char 	*write1HlIcode (HLTYPE, Function *, Int *);
char 	*writeJcond (HLTYPE, Function *, Int *);
char 	*writeJcondInv (HLTYPE, Function *, Int *);
Int     power2 (Int);
void	inverseCondOp (COND_EXPR **);

/* Exported funcions from locident.c */
boolT checkLongEq (LONG_STKID_TYPE, ICODE *, Int, Int, Function *, COND_EXPR **,COND_EXPR **, Int);
boolT checkLongRegEq (LONGID_TYPE, ICODE *, Int, Int, Function *, COND_EXPR **,COND_EXPR **, Int);
byte otherLongRegi (byte, Int, LOCAL_ID *);
void insertIdx (IDX_ARRAY *, Int);


