/*****************************************************************************
 *          I-code related definitions
 * (C) Cristina Cifuentes
 ****************************************************************************/
#pragma once
#include "msvc_fixes.h"
#include "BinaryImage.h"
#include "libdis.h"
#include "Enums.h"
#include "state.h"	    	// State depends on INDEXBASE, but later need STATE
#include "CallConvention.h"

#include <boost/range/iterator_range.hpp>
#include <QtCore/QString>

#include <memory>
#include <vector>
#include <list>
#include <bitset>
#include <set>
#include <algorithm>
#include <initializer_list>

//enum condId;

struct LOCAL_ID;
struct BB;
class Function;
struct STKFRAME;
class CIcodeRec;
struct ICODE;
struct bundle;
typedef std::list<ICODE>::iterator iICODE;
typedef std::list<ICODE>::reverse_iterator riICODE;
typedef boost::iterator_range<iICODE> rCODE;

struct LivenessSet
{
    std::set<eReg> registers;
public:
    LivenessSet(const std::initializer_list<eReg> &init) : registers(init) {}
    LivenessSet() {}
    LivenessSet(const LivenessSet &other) : registers(other.registers)
    {
    }
    void reset()
    {
        registers.clear();
    }

    //    LivenessSet(LivenessSet &&other) : LivenessSet()
    //    {
    //        swap(*this,other);
    //    }
    LivenessSet &operator=(LivenessSet other)
    {
        swap(*this,other);
        return *this;
    }
    friend void swap(LivenessSet& first, LivenessSet& second) // nothrow
    {
        std::swap(first.registers, second.registers);
    }
    LivenessSet &operator|=(const LivenessSet &other)
    {
        registers.insert(other.registers.begin(),other.registers.end());
        return *this;
    }
    LivenessSet &operator&=(const LivenessSet &other)
    {
        std::set<eReg> res;
        std::set_intersection(registers.begin(),registers.end(),
                              other.registers.begin(),other.registers.end(),
                              std::inserter(res, res.end()));
        registers = res;
        return *this;
    }
    LivenessSet &operator-=(const LivenessSet &other)
    {
        std::set<eReg> res;
        std::set_difference(registers.begin(),registers.end(),
                            other.registers.begin(),other.registers.end(),
                            std::inserter(res, res.end()));
        registers = res;
        return *this;
    }
    LivenessSet operator-(const LivenessSet &other) const
    {
        return LivenessSet(*this) -= other;
    }
    LivenessSet operator+(const LivenessSet &other) const
    {
        return LivenessSet(*this) |= other;
    }
    LivenessSet operator &(const LivenessSet &other) const
    {
        return LivenessSet(*this) &= other;
    }
    bool any() const
    {
        return not registers.empty();
    }
    bool operator==(const LivenessSet &other) const
    {
        return registers==other.registers;
    }
    bool operator!=(const LivenessSet &other) const { return not(*this==other);}

    LivenessSet &setReg(int r);
    LivenessSet &addReg(int r);
    bool testReg(int r) const
    {
        return registers.find(eReg(r))!=registers.end();
    }
    bool testRegAndSubregs(int r) const;
    LivenessSet &clrReg(int r);
private:
    void postProcessCompositeRegs();
};

/* uint8_t and uint16_t registers */

/* Def/use of flags - low 4 bits represent flags */
struct DU
{
    uint8_t   d;
    uint8_t   u;
};

/* Definition-use chain for level 1 (within a basic block) */
#define MAX_REGS_DEF	4	    /* 2 regs def'd for long-reg vars */


struct Expr;
struct AstIdent;
struct UnaryOperator;
struct HlTypeSupport
{
    //hlIcode              opcode;    /* hlIcode opcode           */
    virtual bool    removeRegFromLong(eReg regi, LOCAL_ID *locId)=0;
    virtual QString writeOut(Function *pProc, int *numLoc) const=0;
protected:
    Expr * performLongRemoval (eReg regi, LOCAL_ID *locId, Expr *tree);
};

struct CallType : public HlTypeSupport
{
    //for HLI_CALL
    Function *      proc;
    STKFRAME *      args;   // actual arguments
    void allocStkArgs (int num);
    bool newStkArg(Expr *exp, llIcode opcode, Function *pproc);
    void placeStkArg(Expr *exp, int pos);
    virtual Expr * toAst();
public:
    bool removeRegFromLong(eReg /*regi*/, LOCAL_ID * /*locId*/)
    {
        printf("CallType : removeRegFromLong not supproted\n");
        return false;
    }
    QString writeOut(Function *pProc, int *numLoc) const;
};
struct AssignType : public HlTypeSupport
{
    /* for HLI_ASSIGN */
protected:
public:
    Expr    *m_lhs;
    Expr    *m_rhs;
    AssignType() {}
    Expr *lhs() const {return m_lhs;}
    void lhs(Expr *l);
    bool removeRegFromLong(eReg regi, LOCAL_ID *locId);
    QString writeOut(Function *pProc, int *numLoc) const;
};
struct ExpType : public HlTypeSupport
{
    /* for HLI_JCOND, HLI_RET, HLI_PUSH, HLI_POP*/
    Expr    *v;
    ExpType() : v(0) {}
    bool removeRegFromLong(eReg regi, LOCAL_ID *locId)
    {
        v=performLongRemoval(regi,locId,v);
        return true;
    }
    QString writeOut(Function *pProc, int *numLoc) const;
};

struct HLTYPE
{
protected:
public:
    ExpType         exp;      /* for HLI_JCOND, HLI_RET, HLI_PUSH, HLI_POP*/
    hlIcode         opcode;    /* hlIcode opcode           */
    AssignType      asgn;
    CallType        call;
    HlTypeSupport *get();
    const HlTypeSupport *get() const
    {
        return const_cast<const HlTypeSupport *>(const_cast<HLTYPE*>(this)->get());
    }

    void expr(Expr *e)
    {
        assert(e);
        exp.v=e;
    }
    Expr *getMyExpr()
    {
        if(opcode==HLI_CALL)
            return call.toAst();
        return expr();
    }
    void replaceExpr(Expr *e);
    Expr * expr() { return exp.v;}
    const Expr * expr() const  { return exp.v;}
    void set(hlIcode i,Expr *e)
    {
        if(i!=HLI_RET)
            assert(e);
        assert(exp.v==0);
        opcode=i;
        exp.v=e;
    }
    void set(Expr *l,Expr *r);
    void setCall(Function *proc);
    HLTYPE(hlIcode op=HLI_INVALID) : opcode(op)
    {}
    //    HLTYPE() // help valgrind find uninitialized HLTYPES
    //    {}
    HLTYPE & operator=(const HLTYPE &l)
    {
        exp     = l.exp;
        opcode  = l.opcode;
        asgn    = l.asgn;
        call    = l.call;
        return *this;
    }
public:
    QString write1HlIcode(Function *pProc, int *numLoc) const;
    void setAsgn(Expr *lhs, Expr *rhs);
} ;
/* LOW_LEVEL icode operand record */
struct LLOperand
{
    eReg        seg;               /* CS, DS, ES, SS                       */
    eReg        segOver;           /* CS, DS, ES, SS if segment override   */
    int16_t     segValue;          /* Value of segment seg during analysis */
    eReg        regi;              /* 0 < regs < INDEXBASE <= index modes  */
    int16_t     off;               /* memory address offset                */
    uint32_t    opz;             /*   idx of immed src op        */
    bool        immed;
    bool        is_offset; // set by jumps
    bool        is_compound;
    size_t      width;
    /* Source operand if (flg & I)  */
    struct {	    	    /* Call & # actual arg bytes	*/
        Function *proc;     /*   pointer to target proc (for CALL(F))*/
        int     cb;	    /*   # actual arg bytes	    	*/
    } proc;
    LLOperand() : seg(rUNDEF),segOver(rUNDEF),segValue(0),regi(rUNDEF),off(0),
        opz(0),immed(0),is_offset(false),is_compound(0),width(0)
    {
        proc.proc=0;
        proc.cb=0;
    }
    LLOperand(eReg r,size_t w) : LLOperand()
    {
        regi=r;
        width=w;
    }
    bool operator==(const LLOperand &with) const
    {
        return (seg==with.seg) and
                (segOver==with.segOver) and
                (segValue==with.segValue) and
                (regi == with.regi) and
                (off == with.off) and
                (opz==with.opz) and
                (proc.proc==with.proc.proc);
    }
    int64_t getImm2() const {return opz;}
    void SetImmediateOp(uint32_t dw)
    {
        opz=dw;
    }
    eReg getReg2() const {return regi;}
    bool isReg() const;
    static LLOperand CreateImm2(int64_t Val,uint8_t wdth=2)
    {
        LLOperand Op;
        Op.immed=true;
        Op.opz = Val;
        Op.width = wdth;
        return Op;
    }
    static LLOperand CreateReg2(unsigned Val)
    {
        LLOperand Op;
        Op.regi = (eReg)Val;
        return Op;
    }
    bool isSet() const
    {
        return not (*this == LLOperand());
    }
    void addProcInformation(int param_count, CConv::CC_Type call_conv);
    bool isImmediate() const { return immed;}
    void setImmediate(bool x) { immed=x;}
    bool compound() const {return is_compound;} // dx:ax pair
    size_t byteWidth() const { assert(width<=4); return width;}
};
struct LLInst
{
protected:
    uint32_t        m_opcode;       // Low level opcode identifier
    uint32_t            flg;            /* icode flags                  */
    LLOperand           m_src;          /* source operand               */
public:
    int                 codeIdx;    	/* Index into cCode.code        */
    uint8_t             numBytes;       /* Number of bytes this instr   */
    uint32_t            label;          /* offset in image (20-bit adr) */
    LLOperand           m_dst;          /* destination operand          */
    DU                  flagDU;         /* def/use of flags	    	    */
    int                 caseEntry;
    std::vector<uint32_t> caseTbl2;
    int                 hllLabNum;      /* label # for hll codegen      */

    uint32_t    getOpcode() const { return m_opcode;}
    void        setOpcode(uint32_t op) { m_opcode=op; }
    bool                conditionalJump()
                        {
                            return (getOpcode() >= iJB) and (getOpcode() < iJCXZ);
                        }
    bool                testFlags(uint32_t x) const { return (flg & x)!=0;}
    void                setFlags(uint32_t flag) {flg |= flag;}
    void                clrFlags(uint32_t flag);
    uint32_t            getFlag() const {return flg;}
    uint32_t            GetLlLabel() const { return label;}

    void                SetImmediateOp(uint32_t dw) {m_src.SetImmediateOp(dw);}

    bool                match(llIcode op)
                        {
                            return (getOpcode()==op);
                        }
    bool                matchWithRegDst(llIcode op)
                        {
                            return match(op) and m_dst.isReg();
                        }
    bool                match(llIcode op,eReg dest)
                        {
                            return match(op) and match(dest);
                        }
    bool                match(llIcode op,eReg dest,uint32_t flgs)
                        {
                            return match(op) and match(dest) and testFlags(flgs);
                        }
    bool                match(llIcode op,eReg dest,eReg src_reg)
                        {
                            return match(op) and match(dest) and (m_src.regi==src_reg);
                        }
    bool                match(eReg dest,eReg src_reg)
                        {
                            return match(dest) and (m_src.regi==src_reg);
                        }
    bool                matchAny(std::initializer_list<llIcode> ops) {
                            for(llIcode op : ops) {
                                if(match(op))
                                    return true;
                            }
                            return false;
                        }
    bool                match(eReg dest)
                        {
                            return (m_dst.regi==dest);
                        }
    bool                match(llIcode op,uint32_t flgs)
                        {
                            return match(op) and testFlags(flgs);
                        }
    void                set(llIcode op,uint32_t flags)
                        {
                            setOpcode(op);
                            flg =flags;
                        }
    void                set(llIcode op,uint32_t flags,eReg dst_reg)
                        {
                            setOpcode(op);
                            m_dst = LLOperand::CreateReg2(dst_reg);
                            flg =flags;
                        }
    void                set(llIcode op,uint32_t flags,eReg dst_reg,const LLOperand &src_op)
                        {
                            setOpcode(op);
                            m_dst = LLOperand::CreateReg2(dst_reg);
                            m_src = src_op;
                            flg =flags;
                        }
    void                emitGotoLabel(int indLevel);
    void                findJumpTargets(CIcodeRec &_pc);
    void                writeIntComment(QTextStream & s);
    void                dis1Line(int loc_ip, int pass);

    void                flops(QTextStream & out);
    bool                isJmpInst();
    HLTYPE              createCall();
                        LLInst(ICODE *container) : flg(0),codeIdx(0),numBytes(0),m_link(container)
                        {
                            setOpcode(0);
                        }
    const LLOperand &   dst() const { return m_dst; }
    LLOperand &         dst()       { return m_dst; }
    const LLOperand &   src() const { return m_src; }
    LLOperand &         src()       { return m_src; }
    void                replaceSrc(const LLOperand &with) { m_src = with; }
    void                replaceSrc(eReg r) { m_src = LLOperand::CreateReg2(r); }
    void                replaceSrc(int64_t r) { m_src = LLOperand::CreateImm2(r); }
    void                replaceDst(const LLOperand &with) { m_dst = with; }
    bool                srcIsImmed() const { return (flg & I)!=0; }
    condId              idType(opLoc sd) const;
    const LLOperand *   get(opLoc sd) const { return (sd == SRC) ? &src() : &m_dst; }
    LLOperand *         get(opLoc sd) { return (sd == SRC) ? &src() : &m_dst; }

    ICODE *             m_link;
};
struct ADDRESS {

};
struct BinaryArea {
    ADDRESS start;
    ADDRESS fin;
};
#include <boost/icl/interval_set.hpp>
#include <boost/icl/interval_map.hpp>

/* Icode definition: LOW_LEVEL and HIGH_LEVEL */
struct ICODE
{
    // use llvm names at least
    typedef BB MachineBasicBlock;
protected:
    LLInst *m_ll;
    HLTYPE m_hl;
    MachineBasicBlock * Parent;      	/* BB to which this icode belongs   */
    bool                invalid;        /* Has no HIGH_LEVEL equivalent     */
public:
    x86_insn_t insn;
    template<int FLAG>
    struct FlagFilter
    {
        bool operator()(ICODE *ic) {return ic->ll()->testFlags(FLAG);}
        bool operator()(ICODE &ic) {return ic.ll()->testFlags(FLAG);}
    };
    template<int TYPE>
    struct TypeFilter
    {
        bool operator()(ICODE *ic) {return ic->type==TYPE;}
        bool operator()(ICODE &ic) {return ic.type==TYPE;}
    };
    template<int TYPE>
    struct TypeAndValidFilter
    {
        bool operator()(ICODE *ic) {return (ic->type==TYPE) and (ic->valid());}
        bool operator()(ICODE &ic) {return (ic.type==TYPE) and ic.valid();}
    };
    static TypeFilter<HIGH_LEVEL_ICODE> select_high_level;
    static TypeAndValidFilter<HIGH_LEVEL_ICODE> select_valid_high_level;
    /* Def/Use of registers and stack variables */
    struct DU_ICODE
    {
        DU_ICODE()
        {
            def.reset();
            use.reset();
            lastDefRegi.reset();
        }
        LivenessSet def;        // For Registers: position in bitset is reg index
        LivenessSet use;	// For Registers: position in uint32_t is reg index
        LivenessSet lastDefRegi;// Bit set if last def of this register in BB
        void addDefinedAndUsed(eReg r)
        {
            def.addReg(r);
            use.addReg(r);
        }
    };
    struct DU1
    {
    protected:
        int     numRegsDef;             /* # registers defined by this inst */

    public:
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
                assert("Same user more then once!" and uses.end()==std::find(uses.begin(),uses.end(),us));
            }

        };
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
        int getNumRegsDef() const {return numRegsDef;}
        void clearAllDefs() {numRegsDef=0;}
        DU1 &addDef(eReg r) {numRegsDef++; return *this;}
        DU1 &setDef(eReg r) {numRegsDef=1; return *this;}
        void removeDef(eReg r) {numRegsDef--;}
        DU1() : numRegsDef(0)
        {
        }
    };
    icodeType           type;           /* Icode type                       */
    DU_ICODE            du;             /* Def/use regs/vars                */
    DU1                 du1;            /* du chain 1                       */
    int                 loc_ip; // used by CICodeRec to number ICODEs

    LLInst *            ll() { return m_ll;}
    const LLInst *      ll() const { return m_ll;}

    HLTYPE *            hlU() {
        //        assert(type==HIGH_LEVEL);
        //        assert(m_hl.opcode!=HLI_INVALID);
        return &m_hl;
    }
    const HLTYPE *      hl() const {
        //        assert(type==HIGH_LEVEL);
        //        assert(m_hl.opcode!=HLI_INVALID);
        return &m_hl;
    }
    void                hl(const HLTYPE &v) { m_hl=v;}

    void setRegDU(eReg regi, operDu du_in);
    void invalidate();
    void newCallHl();
    void writeDU();
    condId idType(opLoc sd);
    // HLL setting functions
    // set this icode to be an assign
    void setAsgn(Expr *lhs, Expr *rhs)
    {
        type=HIGH_LEVEL_ICODE;
        hlU()->setAsgn(lhs,rhs);
    }
    void setUnary(hlIcode op, Expr *_exp);
    void setJCond(Expr *cexp);

    void emitGotoLabel(int indLevel);
    void copyDU(const ICODE &duIcode, operDu _du, operDu duDu);
    bool valid() {return not invalid;}
    void setParent(MachineBasicBlock *P) { Parent = P; }
public:
    bool removeDefRegi(eReg regi, int thisDefIdx, LOCAL_ID *locId);
    void checkHlCall();
    bool newStkArg(Expr *exp, llIcode opcode, Function *pproc)
    {
        return hlU()->call.newStkArg(exp,opcode,pproc);
    }
    ICODE() :Parent(0),invalid(false),type(NOT_SCANNED_ICODE),loc_ip(0)
    {
         m_ll = new LLInst(this);
    }
    ~ICODE() {
        delete m_ll;
    }
    ICODE(const ICODE &v) {
        m_ll = new LLInst(*v.m_ll);
        m_hl = v.m_hl;
        Parent = v.Parent;
        insn = v.insn;
        type = v.type;
        du = v.du;
        du1 = v.du1;
        loc_ip = v.loc_ip;

    }
    ICODE & operator=(const ICODE &v) {
        delete m_ll;
        m_ll = v.m_ll;
        m_hl = v.m_hl;
        Parent = v.Parent;
        insn = v.insn;
        type = v.type;
        du = v.du;
        du1 = v.du1;
        loc_ip = v.loc_ip;
        return *this;
    }
    ICODE & operator=(ICODE &&v) {
        std::swap(m_ll,v.m_ll);
        std::swap(m_hl,v.m_hl);
        std::swap(Parent , v.Parent);
        std::swap(insn , v.insn);
        std::swap(type , v.type);
        std::swap(du , v.du);
        std::swap(du1 , v.du1);
        std::swap(loc_ip , v.loc_ip);
        return *this;
    }
public:
    const MachineBasicBlock* getParent() const { return Parent; }
    MachineBasicBlock* getParent() { return Parent; }
    //unsigned getNumOperands() const { return (unsigned)Operands.size(); }

};
/** Map n low level instructions to m high level instructions
*/
//struct MappingLLtoML
//{
//    typedef boost::iterator_range<iICODE> rSourceRange;
//    typedef boost::iterator_range<InstListType::iterator> rTargetRange;
//    rSourceRange m_low_level;
//    rTargetRange m_middle_level;
//};
// This is the icode array object.
class CIcodeRec : public std::list<ICODE>
{
public:
                CIcodeRec();	// Constructor

    ICODE *     addIcode(const ICODE * pIcode);
    void        SetInBB(rCODE &rang, BB* pnewBB);
    bool        labelSrch(uint32_t target, uint32_t &pIndex);
    iterator    labelSrch(uint32_t target);
    ICODE *     GetIcode(size_t ip);
    bool        alreadyDecoded(uint32_t target);
};
