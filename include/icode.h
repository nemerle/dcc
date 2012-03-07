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
#include "state.h"			// State depends on INDEXBASE, but later need STATE
//enum condId;

struct LOCAL_ID;
struct BB;
struct Function;
struct STKFRAME;
struct CIcodeRec;
struct ICODE;
typedef std::list<ICODE>::iterator iICODE;
typedef std::list<ICODE>::reverse_iterator riICODE;

/* uint8_t and uint16_t registers */
static const char *const byteReg[9]  = {"al", "cl", "dl", "bl",
                                        "ah", "ch", "dh", "bh", "tmp" };
static const char *const wordReg[21] = {"ax", "cx", "dx", "bx", "sp", "bp",
                                        "si", "di", "es", "cs", "ss", "ds",
                                        "", "", "", "", "", "", "", "", "tmp"};

/* Def/use of flags - low 4 bits represent flags */
struct DU
{
    uint8_t   d;
    uint8_t   u;
};

/* Definition-use chain for level 1 (within a basic block) */
#define MAX_REGS_DEF	4		/* 2 regs def'd for long-reg vars */


struct COND_EXPR;
struct HlTypeSupport
{
    //hlIcode              opcode;    /* hlIcode opcode           */
    virtual bool        removeRegFromLong(eReg regi, LOCAL_ID *locId)=0;
    virtual std::string writeOut(Function *pProc, int *numLoc)=0;
protected:
    void performLongRemoval (eReg regi, LOCAL_ID *locId, COND_EXPR *tree);
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
    bool removeRegFromLong(eReg regi, LOCAL_ID *locId)
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
    bool removeRegFromLong(eReg regi, LOCAL_ID *locId)
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
    bool removeRegFromLong(eReg regi, LOCAL_ID *locId)
    {
        performLongRemoval(regi,locId,v);
        return true;
    }
    std::string writeOut(Function *pProc, int *numLoc);
};

struct HLTYPE
{
protected:
public:
    ExpType         exp;      /* for HLI_JCOND, HLI_RET, HLI_PUSH, HLI_POP*/
    hlIcode         opcode;    /* hlIcode opcode           */
    AssignType      asgn;
    CallType        call;
    HlTypeSupport *get()
    {
        switch(opcode)
        {
        case HLI_ASSIGN: return &asgn;
        case HLI_RET:
        case HLI_POP:
        case HLI_JCOND:
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
    HLTYPE(hlIcode op=HLI_INVALID) : opcode(op)
    {}
    HLTYPE & operator=(const HLTYPE &l)
    {
        exp=l.exp;
        opcode=l.opcode;
        asgn=l.asgn;
        call=l.call;
        return *this;
    }
public:
    std::string write1HlIcode(Function *pProc, int *numLoc);
    void setAsgn(COND_EXPR *lhs, COND_EXPR *rhs);
} ;
/* LOW_LEVEL icode operand record */
struct LLOperand //: public llvm::MCOperand
{
    eReg     seg;               /* CS, DS, ES, SS                       */
    int16_t    segValue;          /* Value of segment seg during analysis */
    eReg     segOver;           /* CS, DS, ES, SS if segment override   */
    eReg       regi;              /* 0 < regs < INDEXBASE <= index modes  */
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
struct LLInst : public llvm::ilist_node<LLInst>
{
protected:
    uint32_t     flg;            /* icode flags                  */
    llIcode      opcode;         /* llIcode instruction          */
public:
    int          codeIdx;    	/* Index into cCode.code            */
    uint8_t      numBytes;       /* Number of bytes this instr   */
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
    bool testFlags(uint32_t x) const { return (flg & x)!=0;}
    void  setFlags(uint32_t flag) {flg |= flag;}
    void  clrFlags(uint32_t flag)
    {
        if(opcode==iMOD)
        {
            assert(false);
        }
        flg &= ~flag;
    }

    uint32_t getFlag() const {return flg;}
    llIcode getOpcode() const { return opcode; }

    uint32_t  GetLlLabel() const { return label;}

    void SetImmediateOp(uint32_t dw) {src.SetImmediateOp(dw);}


    bool match(llIcode op)
    {
        return (opcode==op);
    }
    bool match(llIcode op,eReg dest)
    {
        return (opcode==op)&&dst.regi==dest;
    }
    bool match(llIcode op,eReg dest,uint32_t flgs)
    {
        return (opcode==op) and (dst.regi==dest) and testFlags(flgs);
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
    bool match(llIcode op,uint32_t flgs)
    {
        return (opcode==op) and testFlags(flgs);
    }
    void set(llIcode op,uint32_t flags)
    {
        opcode = op;
        flg =flags;
    }
    void setOpcode(llIcode op)
    {
        opcode = op;
    }
    void emitGotoLabel(int indLevel);
    void findJumpTargets(CIcodeRec &pc);
    void writeIntComment(std::ostringstream &s);
    void dis1Line(int loc_ip, int pass);
    std::ostringstream &strSrc(std::ostringstream &os,bool skip_comma=false);

    void flops(std::ostringstream &out);
    bool isJmpInst();
    HLTYPE toHighLevel(COND_EXPR *lhs, COND_EXPR *rhs, Function *func);
    HLTYPE createCall();
};

/* Icode definition: LOW_LEVEL and HIGH_LEVEL */
struct ICODE
{
protected:
    LLInst m_ll;
    HLTYPE m_hl;
public:
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
        uint8_t	regi[MAX_REGS_DEF+1];	/* registers defined by this inst   */
        Use     idx[MAX_REGS_DEF+1];
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
    LLInst *            ll() { return &m_ll;}
    const LLInst *      ll() const { return &m_ll;}
    HLTYPE *            hl() { return &m_hl;}
    const HLTYPE *      hl() const { return &m_hl;}
    void                hl(const HLTYPE &v) { m_hl=v;}
    int loc_ip; // used by CICodeRec to number ICODEs

    void setRegDU(eReg regi, operDu du_in);
    void invalidate();
    void newCallHl();
    void writeDU(int idx);
    condId idType(opLoc sd);
    // HLL setting functions
    // set this icode to be an assign
    void setAsgn(COND_EXPR *lhs, COND_EXPR *rhs)
    {
        type=HIGH_LEVEL;
        hl()->setAsgn(lhs,rhs);
    }
    void setUnary(hlIcode op, COND_EXPR *exp);
    void setJCond(COND_EXPR *cexp);

    void emitGotoLabel(int indLevel);
    void copyDU(const ICODE &duIcode, operDu _du, operDu duDu);
    bool valid() {return not invalid;}
public:
    bool removeDefRegi(eReg regi, int thisDefIdx, LOCAL_ID *locId);
    void checkHlCall();
    bool newStkArg(COND_EXPR *exp, llIcode opcode, Function *pproc)
    {
        return hl()->call.newStkArg(exp,opcode,pproc);
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
