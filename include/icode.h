/*****************************************************************************
 *          I-code related definitions
 * (C) Cristina Cifuentes
 ****************************************************************************/


/* LOW_LEVEL icode flags */
#define B           0x000001    /* Byte operands (value implicitly used) */
#define I           0x000002    /* Immed. source */
#define NOT_HLL     0x000004    /* Not HLL inst. */
#define FLOAT_OP    0x000008    /* ESC or WAIT   */

#define SEG_IMMED   0x000010    /* Number is relocated segment value */
#define IMPURE      0x000020    /* Instruction modifies code */
#define WORD_OFF    0x000040    /* Inst has word offset ie.could be address */
#define TERMINATES  0x000080    /* Instruction terminates program */

#define CASE        0x000100    /* Label as case part of switch */
#define SWITCH      0x000200    /* Treat indirect JMP as switch stmt */
#define TARGET      0x000400    /* Jump target */
#define SYNTHETIC   0x000800    /* Synthetic jump instruction */
#define NO_LABEL    0x001000    /* Immed. jump cannot be linked to a label */
#define NO_CODE     0x002000    /* Hole in Icode array */
#define SYM_USE     0x004000    /* Instruction uses a symbol */
#define SYM_DEF     0x008000    /* Instruction defines a symbol */

#define NO_SRC      0x010000    /* Opcode takes no source */
#define NO_OPS      0x020000    /* Opcode takes no operands */
#define IM_OPS      0x040000    /* Opcode takes implicit operands */
#define SRC_B       0x080000    /* Source operand is byte (dest is word) */
#define NO_SRC_B    0xF7FFFF    /* Masks off SRC_B */
#define HLL_LABEL   0x100000    /* Icode has a high level language label */
#define IM_DST		0x200000	/* Implicit DST for opcode (SIGNEX) */
#define IM_SRC		0x400000	/* Implicit SRC for opcode (dx:ax)	*/
#define IM_TMP_DST	0x800000	/* Implicit rTMP DST for opcode (DIV/IDIV) */

#define JMP_ICODE  0x1000000    /* Jmp dest immed.op converted to icode index */
#define JX_LOOP	   0x2000000	/* Cond jump is part of loop conditional exp */
#define REST_STK   0x4000000	/* Stack needs to be restored after CALL */

/*  Parser flags  */
#define TO_REG      0x000100    /* rm is source  */
#define S           0x000200    /* sign extend   */
#define OP386       0x000400    /* 386 op-code   */
#define NSP         0x000800    /* NOT_HLL if SP is src or dst */
#define ICODEMASK   0xFF00FF    /* Masks off parser flags */

/* LOW_LEVEL icode, DU flag bits */
#define Cf           1  
#define Sf           2
#define Zf           4
#define Df           8

/* Machine registers */
#define rAX          1  /* These are numbered relative to real 8086 */
#define rCX          2
#define rDX          3
#define rBX          4
#define rSP          5
#define rBP          6
#define rSI          7
#define rDI          8

#define rES          9
#define rCS         10
#define rSS         11
#define rDS         12

#define rAL         13
#define rCL         14
#define rDL         15
#define rBL         16
#define rAH         17
#define rCH         18
#define rDH         19
#define rBH         20

#define rTMP		21			/* temp register for DIV/IDIV/MOD	*/
#define INDEXBASE   22          /* Indexed modes go from INDEXBASE to
                                 * INDEXBASE+7  */

/* Byte and Word registers */
static char *byteReg[9]  = {"al", "cl", "dl", "bl", 
							"ah", "ch", "dh", "bh", "tmp" };
static char *wordReg[21] = {"ax", "cx", "dx", "bx", "sp", "bp", 
                			"si", "di", "es", "cs", "ss", "ds",
							"", "", "", "", "", "", "", "", "tmp"};

#include "state.h"			// State depends on INDEXBASE, but later need STATE

/* Types of icodes */
typedef enum {
    NOT_SCANNED = 0,    /* not even scanned yet */
    LOW_LEVEL,          /* low-level icode  */
    HIGH_LEVEL          /* high-level icode */
} icodeType;


/* LOW_LEVEL icode opcodes */
typedef enum {
	iCBW,		/* 0 */
    iAAA,
    iAAD,       
    iAAM,
    iAAS,
    iADC,
    iADD,
    iAND,
    iBOUND,
    iCALL,
    iCALLF,		/* 10 */
    iCLC,
    iCLD,
    iCLI,
    iCMC,
    iCMP,
    iCMPS,
    iREPNE_CMPS,
    iREPE_CMPS,
    iDAA,
    iDAS,		/* 20 */
    iDEC,
    iDIV,
    iENTER,
    iESC,
    iHLT,
    iIDIV,
    iIMUL,
    iIN,
    iINC,
    iINS,		/* 30 */
    iREP_INS,
    iINT,
    iIRET,
    iJB,
    iJBE,
    iJAE,
    iJA,
    iJE,                
    iJNE,
    iJL,		/* 40 */
    iJGE,
    iJLE,
    iJG,
    iJS,                
    iJNS,
    iJO,
    iJNO,
    iJP,    
    iJNP,
    iJCXZ,		/* 50 */
    iJMP,
    iJMPF,
    iLAHF,
    iLDS,
    iLEA,
    iLEAVE,
    iLES,
    iLOCK,
    iLODS,
    iREP_LODS,	/* 60 */
    iLOOP,
    iLOOPE,
    iLOOPNE,            
    iMOV,		/* 64 */
    iMOVS,
    iREP_MOVS,
    iMUL,		/* 67 */
    iNEG,
    iNOT,
    iOR,   		/* 70 */ 
    iOUT,
    iOUTS,
    iREP_OUTS,
    iPOP,
    iPOPA,
    iPOPF,
    iPUSH,
    iPUSHA,
    iPUSHF,
    iRCL,		/* 80 */
    iRCR,
    iROL,
    iROR,
    iRET,		/* 84 */
    iRETF,          
    iSAHF,
    iSAR,
    iSHL,
    iSHR,
    iSBB,		/* 90 */
    iSCAS,
    iREPNE_SCAS,
    iREPE_SCAS,
    iSIGNEX,
    iSTC,
    iSTD,
    iSTI,
    iSTOS,
    iREP_STOS,
    iSUB,		/* 100 */
    iTEST,
    iWAIT,          
    iXCHG,          
    iXLAT,          
    iXOR,
    iINTO,          
    iNOP,
    iREPNE,         
    iREPE,
	iMOD,		/* 110 */
} llIcode;


/* HIGH_LEVEL icodes opcodes */
typedef enum {
    HLI_ASSIGN,         /* :=               		*/
	HLI_CALL,			/* Call procedure			*/
    HLI_JCOND,          /* Conditional jump 		*/
	HLI_RET,			/* Return from procedure	*/
	/* pseudo high-level icodes */
	HLI_POP,			/* Pop expression			*/
	HLI_PUSH,			/* Push expression			*/
} hlIcode;


/* Operand is defined, used or both flag */
typedef enum {
	DEF,			/* Operand is defined						*/
	USE,			/* Operand is used							*/
	USE_DEF,		/* Operand is used and defined				*/
	NONE,			/* No operation is required on this operand	*/
} operDu;

// I can't believe these are necessary!
#define E_DEF (operDu)DEF
#define E_USE (operDu)USE
#define E_NONE (operDu)NONE
#define E_USE_DEF (operDu)USE_DEF

/* Def/use of flags - low 4 bits represent flags */
typedef struct {
    byte   d;
    byte   u;
} DU;
typedef DU *PDU;

/* Def/Use of registers and stack variables */
typedef struct {
	dword	def;		/* For Registers: position in dword is reg index*/
	dword	lastDefRegi;/* Bit set if last def of this register in BB   */
	dword	use;		/* For Registers: position in dword is reg index*/
}DU_ICODE;


/* Definition-use chain for level 1 (within a basic block) */
#define MAX_REGS_DEF	2		/* 2 regs def'd for long-reg vars */
#define MAX_USES		5

typedef struct {
	Int		numRegsDef;			/* # registers defined by this inst		*/
	byte	regi[MAX_REGS_DEF];	/* registers defined by this inst		*/
	Int		idx[MAX_REGS_DEF][MAX_USES];	/* inst that uses this def  */
} DU1;


/* LOW_LEVEL icode operand record */
typedef struct {
    byte     seg;               /* CS, DS, ES, SS                       */
    int16    segValue;          /* Value of segment seg during analysis */
    byte     segOver;           /* CS, DS, ES, SS if segment override   */
    byte     regi;              /* 0 < regs < INDEXBASE <= index modes  */
    int16    off;               /* memory address offset                */
} ICODEMEM;
typedef ICODEMEM *PMEM;

/* LOW_LEVEL operand location: source or destination */
typedef enum {
	SRC,						/* Source operand 		*/
	DST,						/* Destination operand	*/
	LHS_OP,						/* Left-hand side operand (for HIGH_LEVEL) */
} opLoc;

typedef struct
{
    hlIcode              opcode;    /* hlIcode opcode           */
    union {                         /* different operands       */
        struct {                    /* for HLI_ASSIGN			*/
            COND_EXPR    *lhs;
            COND_EXPR    *rhs;
        }                asgn;       
        COND_EXPR        *exp;      /* for HLI_JCOND, HLI_RET, HLI_PUSH, HLI_POP*/
		struct {					/* for HLI_CALL				*/
			struct _proc     *proc;
			struct _STKFRAME *args;	/* actual arguments			*/
		}				 call;
    }                    oper;      /* operand                  */
} HLTYPE;

typedef struct
{
    llIcode     opcode;         /* llIcode instruction          */
    byte        numBytes;       /* Number of bytes this instr   */
	flags32     flg;            /* icode flags                  */
    dword       label;          /* offset in image (20-bit adr) */
    ICODEMEM    dst;            /* destination operand          */
    ICODEMEM    src;            /* source operand               */
    union {                     /* Source operand if (flg & I)  */
        dword   op;             /*   idx of immed src op        */
		struct {				/* Call & # actual arg bytes	*/
          struct _proc *proc;   /*   ^ target proc (for CALL(F))*/
		  Int		   cb;		/*   # actual arg bytes			*/
		}		proc;
    }           immed;
    DU          flagDU;         /* def/use of flags				*/
    struct {                    /* Case table if op==JMP && !I  */
        Int     numEntries;     /*   # entries in case table    */
        dword  *entries;        /*   array of offsets           */
    }           caseTbl;
    Int         hllLabNum;      /* label # for hll codegen      */
} LLTYPE;


/* Icode definition: LOW_LEVEL and HIGH_LEVEL */
typedef struct {
    icodeType           type;           /* Icode type                   */
    boolT               invalid;        /* Has no HIGH_LEVEL equivalent */
	struct _BB			*inBB;			/* BB to which this icode belongs */
	DU_ICODE			du;				/* Def/use regs/vars			*/
	DU1					du1;			/* du chain 1					*/
	Int					codeIdx;		/* Index into cCode.code		*/
    struct {                            /* Different types of icodes    */
		LLTYPE ll;
        HLTYPE hl;						/* For HIGH_LEVEL icodes    */
    }                   		 ic;    /* intermediate code        */
} ICODE;
typedef ICODE* PICODE;

// This is the icode array object.
// The bulk of this could well be done with a class library
class CIcodeRec
{
public:
			CIcodeRec();	// Constructor
			~CIcodeRec();	// Destructor

	PICODE	addIcode(PICODE pIcode);
	PICODE	GetFirstIcode();
//	PICODE	GetNextIcode(PICODE pCurIcode);
	boolT	IsValid(PICODE pCurIcode);
	int		GetNumIcodes();
	void	SetInBB(int start, int end, struct _BB* pnewBB);
	void	SetImmediateOp(int ip, dword dw);
	void	SetLlFlag(int ip, dword flag);
	void	ClearLlFlag(int ip, dword flag);
	dword	GetLlFlag(int ip);
	void	SetLlInvalid(int ip, boolT fInv);
	dword	GetLlLabel(int ip);
	llIcode	GetLlOpcode(int ip);
	boolT	labelSrch(dword target, Int *pIndex);
	PICODE	GetIcode(int ip);

protected:
	Int		numIcode;		/* # icodes in use 		*/
	Int		alloc;			/* # icodes allocated 	*/
	PICODE	icode;			/* Array of icodes		*/

};
