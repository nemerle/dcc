/*****************************************************************************
 *          I-code related definitions
 * (C) Cristina Cifuentes
 ****************************************************************************/
#pragma once
#include <vector>
#include <list>
#include <bitset>
#include <llvm/MC/MCInst.h>
#include <llvm/MC/MCAsmInfo.h>
#include "Enums.h"
//enum condId;
struct LOCAL_ID;
/* LOW_LEVEL icode flags */
enum eLLFlags
{

    B           =0x0000001,    /* Byte operands (value implicitly used) */
    I           =0x0000002,    /* Immed. source */
    NOT_HLL     =0x0000004,    /* Not HLL inst. */
    FLOAT_OP    =0x0000008,    /* ESC or WAIT   */
    SEG_IMMED   =0x0000010,    /* Number is relocated segment value */
    IMPURE      =0x0000020,    /* Instruction modifies code */
    WORD_OFF    =0x0000040,    /* Inst has word offset ie.could be address */
    TERMINATES  =0x0000080,    /* Instruction terminates program */
    CASE        =0x0000100,    /* Label as case part of switch */
    SWITCH      =0x0000200,    /* Treat indirect JMP as switch stmt */
    TARGET      =0x0000400,    /* Jump target */
    SYNTHETIC   =0x0000800,    /* Synthetic jump instruction */
    NO_LABEL    =0x0001000,    /* Immed. jump cannot be linked to a label */
    NO_CODE     =0x0002000,    /* Hole in Icode array */
    SYM_USE     =0x0004000,    /* Instruction uses a symbol */
    SYM_DEF     =0x0008000,    /* Instruction defines a symbol */

    NO_SRC      =0x0010000,    /* Opcode takes no source */
    NO_OPS      =0x0020000,    /* Opcode takes no operands */
    IM_OPS      =0x0040000,    /* Opcode takes implicit operands */
    SRC_B       =0x0080000,    /* Source operand is byte (dest is word) */
#define NO_SRC_B    0xF7FFFF    /* Masks off SRC_B */
    HLL_LABEL   =0x0100000,    /* Icode has a high level language label */
    IM_DST      =0x0200000,	/* Implicit DST for opcode (SIGNEX) */
    IM_SRC      =0x0400000,	/* Implicit SRC for opcode (dx:ax)	*/
    IM_TMP_DST  =0x0800000,	/* Implicit rTMP DST for opcode (DIV/IDIV) */

    JMP_ICODE   =0x1000000,    /* Jmp dest immed.op converted to icode index */
    JX_LOOP     =0x2000000,	/* Cond jump is part of loop conditional exp */
    REST_STK    =0x4000000	/* Stack needs to be restored after CALL */
};

/*  Parser flags  */
#define TO_REG      0x000100    /* rm is source  */
#define S_EXT       0x000200    /* sign extend   */
#define OP386       0x000400    /* 386 op-code   */
#define NSP         0x000800    /* NOT_HLL if SP is src or dst */
#define ICODEMASK   0xFF00FF    /* Masks off parser flags */

/* LOW_LEVEL icode, DU flag bits */
enum eDuFlags
{
    Cf=1,
    Sf=2,
    Zf=4,
    Df=8
};

/* Machine registers */
enum eReg
{
rAX =        1,  /* These are numbered relative to real 8086 */
rCX =        2,
rDX =        3,
rBX =        4,
rSP =        5,
rBP =        6,
rSI =        7,
rDI =        8,

rES =        9,
rCS =       10,
rSS =       11,
rDS =       12,

rAL =       13,
rCL =       14,
rDL =       15,
rBL =       16,
rAH =       17,
rCH =       18,
rDH =       19,
rBH =       20,

rTMP=       21			/* temp register for DIV/IDIV/MOD	*/
#define INDEXBASE   22          /* Indexed modes go from INDEXBASE to
    * INDEXBASE+7  */
};
/* Byte and Word registers */
static const char *const byteReg[9]  = {"al", "cl", "dl", "bl",
                                        "ah", "ch", "dh", "bh", "tmp" };
static const char *const wordReg[21] = {"ax", "cx", "dx", "bx", "sp", "bp",
                                        "si", "di", "es", "cs", "ss", "ds",
                                        "", "", "", "", "", "", "", "", "tmp"};

#include "state.h"			// State depends on INDEXBASE, but later need STATE

/* Types of icodes */
enum icodeType
{
    NOT_SCANNED = 0,    /* not even scanned yet */
    LOW_LEVEL,          /* low-level icode  */
    HIGH_LEVEL          /* high-level icode */
};


/* LOW_LEVEL icode opcodes */
enum llIcode
{
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
    iMOD		/* 110 */
};
struct BB;
struct Function;
struct STKFRAME;
/* HIGH_LEVEL icodes opcodes */
typedef enum {
    HLI_ASSIGN,         /* :=               		*/
    HLI_CALL,			/* Call procedure			*/
    HLI_JCOND,          /* Conditional jump 		*/
    HLI_RET,			/* Return from procedure	*/
    /* pseudo high-level icodes */
    HLI_POP,			/* Pop expression			*/
    HLI_PUSH			/* Push expression			*/
} hlIcode;

/* Def/use of flags - low 4 bits represent flags */
struct DU
{
    byte   d;
    byte   u;
};

/* Def/Use of registers and stack variables */
struct DU_ICODE
{
    std::bitset<32> def;        // For Registers: position in bitset is reg index
    //dword	def;		// For Registers: position in dword is reg index
    //dword	def;		// For Registers: position in dword is reg index
    std::bitset<32>	use;	// For Registers: position in dword is reg index
    std::bitset<32> lastDefRegi;// Bit set if last def of this register in BB
};


/* Definition-use chain for level 1 (within a basic block) */
#define MAX_REGS_DEF	2		/* 2 regs def'd for long-reg vars */
#define MAX_USES		5


struct COND_EXPR;
struct HLTYPE
{
    hlIcode              opcode;    /* hlIcode opcode           */
    union {                         /* different operands       */
        struct {                    /* for HLI_ASSIGN			*/
            COND_EXPR    *lhs;
            COND_EXPR    *rhs;
        }                asgn;
        COND_EXPR        *exp;      /* for HLI_JCOND, HLI_RET, HLI_PUSH, HLI_POP*/
        struct {					/* for HLI_CALL				*/
            Function     *proc;
            STKFRAME *args;	/* actual arguments			*/
        } call;
    } oper;      /* operand                  */
} ;
/* LOW_LEVEL icode operand record */
struct LLOperand //: public llvm::MCOperand
{
    byte     seg;               /* CS, DS, ES, SS                       */
    int16    segValue;          /* Value of segment seg during analysis */
    byte     segOver;           /* CS, DS, ES, SS if segment override   */
    byte     regi;              /* 0 < regs < INDEXBASE <= index modes  */
    int16    off;               /* memory address offset                */
    dword   opz;             /*   idx of immed src op        */
    //union {/* Source operand if (flg & I)  */
    struct {				/* Call & # actual arg bytes	*/
        Function *proc;     /*   pointer to target proc (for CALL(F))*/
        int     cb;		/*   # actual arg bytes			*/
    } proc;
    dword op() const {return opz;}
    void SetImmediateOp(dword dw) {opz=dw;}

};
struct LLInst : public llvm::MCInst
{
    llIcode     opcode;         /* llIcode instruction          */
    byte        numBytes;       /* Number of bytes this instr   */
    flags32     flg;            /* icode flags                  */
    dword       label;          /* offset in image (20-bit adr) */
    LLOperand    dst;            /* destination operand          */
    LLOperand    src;            /* source operand               */
    DU          flagDU;         /* def/use of flags				*/
    struct {                    /* Case table if op==JMP && !I  */
        Int     numEntries;     /*   # entries in case table    */
        dword  *entries;        /*   array of offsets           */

    }           caseTbl;
    Int         hllLabNum;      /* label # for hll codegen      */
    bool conditionalJump()
    {
        return (opcode >= iJB) && (opcode < iJCXZ);
    }
    bool anyFlagSet(uint32_t x) const { return (flg & x)!=0;}
    bool match(llIcode op)
    {
        return (opcode==op);
    }
    bool match(llIcode op,eReg dest)
    {
        return (opcode==op)&&dst.regi==dest;
    }
    bool match(llIcode op,eReg dest,eReg src_reg)
    {
        return (opcode==op)&&(dst.regi==dest)&&(src.regi==src_reg);
    }
    bool match(eReg dest,eReg src_reg)
    {
        return (dst.regi==dest)&&(src.regi==src_reg);
    }
    bool match(eReg dest)
    {
        return (dst.regi==dest);
    }
    void set(llIcode op,uint32_t flags)
    {
        opcode = op;
        flg =flags;
    }
};

/* Icode definition: LOW_LEVEL and HIGH_LEVEL */
struct ICODE
{
    struct DU1
    {
        struct DefUse
        {
            int Reg; // used register
            int DefLoc;
            std::vector<std::list<ICODE>::iterator > useLoc; // use locations [MAX_USES]

        };
        struct Use
        {
            int Reg; // used register
            std::vector<std::list<ICODE>::iterator> uses; // use locations [MAX_USES]
            void removeUser(std::list<ICODE>::iterator us)
            {
                // ic is no no longer an user
                auto iter=std::find(uses.begin(),uses.end(),us);
                if(iter==uses.end())
                    return;
                uses.erase(iter);
                assert("Same user more then once!" && uses.end()==std::find(uses.begin(),uses.end(),us));
            }

        };
        Int     numRegsDef;             /* # registers defined by this inst */
        byte	regi[MAX_REGS_DEF];	/* registers defined by this inst   */
        Use     idx[MAX_REGS_DEF];
        //Int     idx[MAX_REGS_DEF][MAX_USES];	/* inst that uses this def  */
        bool    used(int regIdx)
                {
                    return not idx[regIdx].uses.empty();
                }
        int     numUses(int regIdx)
                {
                    return idx[regIdx].uses.size();
                }
        void recordUse(int regIdx,std::list<ICODE>::iterator location)
                {
                    idx[regIdx].uses.push_back(location);
                }
        void remove(int regIdx,int use_idx)
                {
                    idx[regIdx].uses.erase(idx[regIdx].uses.begin()+use_idx);
                }
        void remove(int regIdx,std::list<ICODE>::iterator ic)
                {
                    Use &u(idx[regIdx]);
                    u.removeUser(ic);
                }
    };
    icodeType           type;           /* Icode type                       */
    bool                invalid;        /* Has no HIGH_LEVEL equivalent     */
    BB			*inBB;      	/* BB to which this icode belongs   */
    DU_ICODE		du;             /* Def/use regs/vars                */
    DU1			du1;        	/* du chain 1                       */
    Int			codeIdx;    	/* Index into cCode.code            */
    struct IC {         /* Different types of icodes    */
        LLInst ll;
        HLTYPE hl;  	/* For HIGH_LEVEL icodes    */
    };
    IC ic;/* intermediate code        */
    int loc_ip; // used by CICodeRec to number ICODEs

    void  ClrLlFlag(dword flag) {ic.ll.flg &= ~flag;}
    void  SetLlFlag(dword flag) {ic.ll.flg |= flag;}
    dword GetLlFlag() {return ic.ll.flg;}
    bool isLlFlag(dword flg) {return (ic.ll.flg&flg)!=0;}
    llIcode GetLlOpcode() const { return ic.ll.opcode; }
    dword  GetLlLabel() const { return ic.ll.label;}
    void SetImmediateOp(dword dw) {ic.ll.src.SetImmediateOp(dw);}

    void writeIntComment(std::ostringstream &s);
    void setRegDU(byte regi, operDu du_in);
    void invalidate();
    void newCallHl();
    void writeDU(Int idx);
    condId idType(opLoc sd);
    // HLL setting functions
    void setAsgn(COND_EXPR *lhs, COND_EXPR *rhs); // set this icode to be an assign
    void setUnary(hlIcode op, COND_EXPR *exp);
    void setJCond(COND_EXPR *cexp);
    void emitGotoLabel(Int indLevel);
    void copyDU(const ICODE &duIcode, operDu _du, operDu duDu);
    bool valid() {return not invalid;}
public:
    bool removeDefRegi(byte regi, Int thisDefIdx, LOCAL_ID *locId);
    void checkHlCall();
};

// This is the icode array object.
class CIcodeRec : public std::list<ICODE>
{
public:
    CIcodeRec();	// Constructor

    ICODE *	addIcode(ICODE *pIcode);
    void	SetInBB(int start, int end, BB* pnewBB);
    bool	labelSrch(dword target, dword &pIndex);
    iterator    labelSrch(dword target);
    ICODE *	GetIcode(int ip);
};
typedef CIcodeRec::iterator iICODE;
typedef CIcodeRec::reverse_iterator riICODE;
