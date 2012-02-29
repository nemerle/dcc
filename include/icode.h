/*****************************************************************************
 *          I-code related definitions
 * (C) Cristina Cifuentes
 ****************************************************************************/
#pragma once
#include <vector>
#include <list>
#include <bitset>
#include <llvm/ADT/ilist.h>
#include <llvm/ADT/ilist_node.h>
#include <llvm/MC/MCInst.h>
#include <llvm/MC/MCAsmInfo.h>
#include "Enums.h"
//enum condId;
struct LOCAL_ID;

/* LOW_LEVEL icode, DU flag bits */
enum eDuFlags
{
    Cf=1,
    Sf=2,
    Zf=4,
    Df=8
};

/* uint8_t and uint16_t registers */
static const char *const byteReg[9]  = {"al", "cl", "dl", "bl",
                                        "ah", "ch", "dh", "bh", "tmp" };
static const char *const wordReg[21] = {"ax", "cx", "dx", "bx", "sp", "bp",
                                        "si", "di", "es", "cs", "ss", "ds",
                                        "", "", "", "", "", "", "", "", "tmp"};

#include "state.h"			// State depends on INDEXBASE, but later need STATE

struct BB;
struct Function;
struct STKFRAME;

/* Def/use of flags - low 4 bits represent flags */
struct DU
{
    uint8_t   d;
    uint8_t   u;
};

/* Definition-use chain for level 1 (within a basic block) */
#define MAX_REGS_DEF	2		/* 2 regs def'd for long-reg vars */
#define MAX_USES		5


struct COND_EXPR;
struct HlTypeSupport
{
    //hlIcode              opcode;    /* hlIcode opcode           */
    virtual bool        removeRegFromLong(uint8_t regi, LOCAL_ID *locId)=0;
    virtual std::string writeOut(Function *pProc, int *numLoc)=0;
protected:
    void performLongRemoval (uint8_t regi, LOCAL_ID *locId, COND_EXPR *tree);
};

struct CallType : public HlTypeSupport
{
    //for HLI_CALL
    Function *      proc;
    STKFRAME *      args;   // actual arguments
    void allocStkArgs (int num);
    bool newStkArg(COND_EXPR *exp, llIcode opcode, Function *pproc);
    void placeStkArg(COND_EXPR *exp, int pos);
public:
    bool removeRegFromLong(uint8_t regi, LOCAL_ID *locId)
    {
        printf("CallType : removeRegFromLong not supproted");
        return false;
    }
    std::string writeOut(Function *pProc, int *numLoc);
};
struct AssignType : public HlTypeSupport
{
    /* for HLI_ASSIGN */
    COND_EXPR    *lhs;
    COND_EXPR    *rhs;
    bool removeRegFromLong(uint8_t regi, LOCAL_ID *locId)
    {
        performLongRemoval(regi,locId,lhs);
        return true;
    }
    std::string writeOut(Function *pProc, int *numLoc);
};
struct ExpType : public HlTypeSupport
{
    /* for HLI_JCOND, HLI_RET, HLI_PUSH, HLI_POP*/
    COND_EXPR    *v;
    bool removeRegFromLong(uint8_t regi, LOCAL_ID *locId)
    {
        performLongRemoval(regi,locId,v);
        return true;
    }
    std::string writeOut(Function *pProc, int *numLoc);
};

struct HLTYPE
{
    hlIcode              opcode;    /* hlIcode opcode           */
    ExpType         exp;      /* for HLI_JCOND, HLI_RET, HLI_PUSH, HLI_POP*/
    AssignType      asgn;
    CallType        call;
    HlTypeSupport *get()
    {
        switch(opcode)
        {
        case HLI_ASSIGN: return &asgn;
        case HLI_RET:
        case HLI_POP:
        case HLI_PUSH:   return &exp;
        case HLI_CALL:   return &call;
        default:
            return 0;
        }
    }

    void expr(COND_EXPR *e)
    {
        assert(e);
        exp.v=e;
    }
    COND_EXPR * expr() { return exp.v;}
    void set(hlIcode i,COND_EXPR *e)
    {
        if(i!=HLI_RET)
            assert(e);
        assert(exp.v==0);
        opcode=i;
        exp.v=e;
    }
    void set(COND_EXPR *l,COND_EXPR *r)
    {
        assert(l);
        assert(r);
        opcode = HLI_ASSIGN;
        assert((asgn.lhs==0) and (asgn.rhs==0)); //prevent memory leaks
        asgn.lhs=l;
        asgn.rhs=r;
    }
public:
    std::string write1HlIcode(Function *pProc, int *numLoc);
} ;
/* LOW_LEVEL icode operand record */
struct LLOperand //: public llvm::MCOperand
{
    uint8_t     seg;               /* CS, DS, ES, SS                       */
    int16_t    segValue;          /* Value of segment seg during analysis */
    uint8_t     segOver;           /* CS, DS, ES, SS if segment override   */
    uint8_t     regi;              /* 0 < regs < INDEXBASE <= index modes  */
    int16_t    off;               /* memory address offset                */
    uint32_t   opz;             /*   idx of immed src op        */
    //union {/* Source operand if (flg & I)  */
    struct {				/* Call & # actual arg bytes	*/
        Function *proc;     /*   pointer to target proc (for CALL(F))*/
        int     cb;		/*   # actual arg bytes			*/
    } proc;
    uint32_t op() const {return opz;}
    void SetImmediateOp(uint32_t dw) {opz=dw;}

};
struct LLInst : public llvm::MCInst
{
    llIcode      opcode;         /* llIcode instruction          */
    uint8_t      numBytes;       /* Number of bytes this instr   */
    uint32_t     flg;            /* icode flags                  */
    uint32_t     label;          /* offset in image (20-bit adr) */
    LLOperand    dst;            /* destination operand          */
    LLOperand    src;            /* source operand               */
    DU           flagDU;         /* def/use of flags				*/
    struct {                    /* Case table if op==JMP && !I  */
        int     numEntries;     /*   # entries in case table    */
        uint32_t  *entries;        /*   array of offsets           */

    }           caseTbl;
    int         hllLabNum;      /* label # for hll codegen      */
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
    /* Def/Use of registers and stack variables */
    struct DU_ICODE
    {
        DU_ICODE()
        {
            def.reset();
            use.reset();
            lastDefRegi.reset();
        }
        std::bitset<32> def;        // For Registers: position in bitset is reg index
        std::bitset<32> use;	// For Registers: position in uint32_t is reg index
        std::bitset<32> lastDefRegi;// Bit set if last def of this register in BB
    };
    struct DU1
    {
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
        int     numRegsDef;             /* # registers defined by this inst */
        uint8_t	regi[3];	/* registers defined by this inst   */
        Use     idx[3];
        //int     idx[MAX_REGS_DEF][MAX_USES];	/* inst that uses this def  */
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
    int			codeIdx;    	/* Index into cCode.code            */
    struct IC {         /* Different types of icodes    */
        LLInst ll;
        HLTYPE hl;  	/* For HIGH_LEVEL icodes    */
    };
    IC ic;/* intermediate code        */
    int loc_ip; // used by CICodeRec to number ICODEs

    void  ClrLlFlag(uint32_t flag) {ic.ll.flg &= ~flag;}
    void  SetLlFlag(uint32_t flag) {ic.ll.flg |= flag;}
    uint32_t GetLlFlag() {return ic.ll.flg;}
    bool isLlFlag(uint32_t flg) {return (ic.ll.flg&flg)!=0;}
    llIcode GetLlOpcode() const { return ic.ll.opcode; }
    uint32_t  GetLlLabel() const { return ic.ll.label;}
    void SetImmediateOp(uint32_t dw) {ic.ll.src.SetImmediateOp(dw);}

    void writeIntComment(std::ostringstream &s);
    void setRegDU(uint8_t regi, operDu du_in);
    void invalidate();
    void newCallHl();
    void writeDU(int idx);
    condId idType(opLoc sd);
    // HLL setting functions
    void setAsgn(COND_EXPR *lhs, COND_EXPR *rhs); // set this icode to be an assign
    void setUnary(hlIcode op, COND_EXPR *exp);
    void setJCond(COND_EXPR *cexp);
    void emitGotoLabel(int indLevel);
    void copyDU(const ICODE &duIcode, operDu _du, operDu duDu);
    bool valid() {return not invalid;}
public:
    bool removeDefRegi(uint8_t regi, int thisDefIdx, LOCAL_ID *locId);
    void checkHlCall();
    bool newStkArg(COND_EXPR *exp, llIcode opcode, Function *pproc)
    {
        return ic.hl.call.newStkArg(exp,opcode,pproc);
    }
};

// This is the icode array object.
class CIcodeRec : public std::list<ICODE>
{
public:
    CIcodeRec();	// Constructor

    ICODE *	addIcode(ICODE *pIcode);
    void	SetInBB(int start, int end, BB* pnewBB);
    bool	labelSrch(uint32_t target, uint32_t &pIndex);
    iterator    labelSrch(uint32_t target);
    ICODE *	GetIcode(int ip);
};
typedef CIcodeRec::iterator iICODE;
typedef CIcodeRec::reverse_iterator riICODE;
