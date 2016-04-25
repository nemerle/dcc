/*
 * File:    ast.h
 * Purpose: definition of the abstract syntax tree ADT.
 * Date:    September 1993
 * (C) Cristina Cifuentes
 */
#pragma once

#include "Enums.h"
#include "msvc_fixes.h"

#include <boost/range/iterator_range.hpp>
#include <stdint.h>
#include <cstring>
#include <list>

static const int operandSize=20;
/* The following definitions and types define the Conditional Expression
 * attributed syntax tree, as defined by the following EBNF:
    CondExp     ::= CondTerm AND CondTerm | CondTerm
    CondTerm    ::= (CondFactor op CondFactor)
    CondFactor  ::= Identifier | ! CondFactor
    Identifier  ::= globalVar | register | localVar | parameter | constant
    op          ::= <= | < | = | != | > | >=
 */
/* High-level BOOLEAN conditions for iJB..iJNS icodes */
static const condOp condOpJCond[12] = {LESS, LESS_EQUAL, GREATER_EQUAL, GREATER,
                                       EQUAL, NOT_EQUAL, LESS, GREATER_EQUAL,
                                       LESS_EQUAL, GREATER, GREATER_EQUAL, LESS};
struct AstIdent;
struct Function;
struct STKFRAME;
struct LOCAL_ID;
struct ICODE;
struct LLInst;
struct LLOperand;
struct ID;
typedef std::list<ICODE>::iterator iICODE;
typedef boost::iterator_range<iICODE> rICODE;
#include "IdentType.h"

/* Expression data type */
struct Expr
{
public:
    condNodeType            m_type;     /* Conditional Expression Node Type */
public:
    static bool         insertSubTreeLongReg(Expr *exp, Expr *&tree, int longIdx);
    static bool         insertSubTreeReg(Expr *&tree, Expr *_expr, eReg regi, const LOCAL_ID *locsym);
    static bool         insertSubTreeReg(AstIdent *&tree, Expr *_expr, eReg regi, const LOCAL_ID *locsym);
public:

    virtual Expr *clone() const=0;  //!< Makes a deep copy of the given expression
    Expr(condNodeType t=UNKNOWN_OP) : m_type(t)
    {

    }
    /** Recursively deallocates the abstract syntax tree rooted at *exp */
    virtual ~Expr() {}
public:
    virtual QString walkCondExpr (Function * pProc, int* numLoc) const=0;
    virtual Expr *inverse() const=0; // return new COND_EXPR that is invarse of this
    virtual bool xClear(rICODE range_to_check, iICODE lastBBinst, const LOCAL_ID &locId)=0;
    virtual Expr *insertSubTreeReg(Expr *_expr, eReg regi, const LOCAL_ID *locsym)=0;
    virtual Expr *insertSubTreeLongReg(Expr *_expr, int longIdx)=0;
    virtual hlType expType(Function *pproc) const=0;
    virtual int hlTypeSize(Function *pproc) const=0;
    virtual Expr * performLongRemoval(eReg regi, LOCAL_ID *locId) { return this; }
};
struct UnaryOperator : public Expr
{
    UnaryOperator(condNodeType t=UNKNOWN_OP) : Expr(t),unaryExp(nullptr) {}
    Expr *unaryExp;
    virtual Expr *inverse() const
    {
        if (m_type == NEGATION) //TODO: memleak here
        {
            return unaryExp->clone();
        }
        return this->clone();
    }
    virtual Expr *clone() const
    {
        UnaryOperator *newExp = new UnaryOperator(*this);
        newExp->unaryExp = unaryExp->clone();
        return newExp;
    }
    virtual bool xClear(rICODE range_to_check, iICODE lastBBinst, const LOCAL_ID &locs);
    static UnaryOperator *Create(condNodeType t, Expr *sub_expr)
    {
        UnaryOperator *newExp = new UnaryOperator();
        newExp->m_type = t;
        newExp->unaryExp = sub_expr;
        return (newExp);
    }
    ~UnaryOperator()
    {
        delete unaryExp;
        unaryExp=nullptr;
    }
public:
    int hlTypeSize(Function *pproc) const;
    virtual QString walkCondExpr(Function *pProc, int *numLoc) const;
    virtual Expr *insertSubTreeReg(Expr *_expr, eReg regi, const LOCAL_ID *locsym);
    virtual hlType expType(Function *pproc) const;
    virtual Expr *insertSubTreeLongReg(Expr *_expr, int longIdx);
private:
    QString wrapUnary(Function *pProc, int *numLoc, QChar op) const;
};

struct BinaryOperator : public Expr
{
    condOp      m_op;
    Expr *m_lhs;
    Expr *m_rhs;
    BinaryOperator(condOp o) : Expr(BOOLEAN_OP)
    {
        m_op = o;
        m_lhs=m_rhs=nullptr;
    }
    BinaryOperator(condOp o,Expr *l,Expr *r) : Expr(BOOLEAN_OP)
    {
        m_op = o;
        m_lhs=l;
        m_rhs=r;
    }
    ~BinaryOperator()
    {
        assert(m_lhs!=m_rhs or m_lhs==nullptr);
        delete m_lhs;
        delete m_rhs;
        m_lhs=m_rhs=nullptr;
    }
    static BinaryOperator *Create(condOp o,Expr *l,Expr *r)
    {
        BinaryOperator *res = new BinaryOperator(o);
        res->m_lhs = l;
        res->m_rhs = r;
        return res;
    }
    static BinaryOperator *LogicAnd(Expr *l,Expr *r)
    {
        return Create(DBL_AND,l,r);
    }
    static BinaryOperator *createSHL(Expr *l,Expr *r)
    {
        return Create(SHL,l,r);
    }
    static BinaryOperator *And(Expr *l,Expr *r)
    {
        return Create(AND,l,r);
    }
    static BinaryOperator *Or(Expr *l,Expr *r)
    {
        return Create(OR,l,r);
    }
    static BinaryOperator *LogicOr(Expr *l,Expr *r)
    {
        return Create(DBL_OR,l,r);
    }
    static BinaryOperator *CreateAdd(Expr *l,Expr *r) {
        return Create(ADD,l,r);

    }
    void changeBoolOp(condOp newOp);
    virtual Expr *inverse() const;
    virtual Expr *clone() const;
    virtual bool xClear(rICODE range_to_check, iICODE lastBBinst, const LOCAL_ID &locs);
    virtual Expr *insertSubTreeReg(Expr *_expr, eReg regi, const LOCAL_ID *locsym);
    virtual Expr *insertSubTreeLongReg(Expr *_expr, int longIdx);
    const Expr *lhs() const
    {
        return const_cast<const Expr *>(const_cast<BinaryOperator *>(this)->lhs());
    }
    const Expr *rhs() const
    {
        return const_cast<const Expr *>(const_cast<BinaryOperator *>(this)->rhs());
    }

    Expr *lhs()
    {
        assert(m_type==BOOLEAN_OP);
        return m_lhs;
    }
    Expr *rhs()
    {
        assert(m_type==BOOLEAN_OP);
        return m_rhs;
    }
    condOp op() const { return m_op;}
    /* Changes the boolean conditional operator at the root of this expression */
    void op(condOp o) { m_op=o;}
    QString walkCondExpr(Function * pProc, int* numLoc) const;
public:
    hlType expType(Function *pproc) const;
    int hlTypeSize(Function *pproc) const;
};
struct AstIdent : public UnaryOperator
{
    AstIdent() : UnaryOperator(IDENTIFIER)
    {
    }
    IDENTTYPE   ident;              /* for IDENTIFIER                   */
    static AstIdent *  Loc(int off, LOCAL_ID *localId);
    static AstIdent *  LongIdx(int idx);
    static AstIdent *  String(uint32_t idx);
    static AstIdent *  Other(eReg seg, eReg regi, int16_t off);
    static AstIdent *  Param(int off, const STKFRAME *argSymtab);
    static AstIdent *  Long(LOCAL_ID *localId, opLoc sd, iICODE pIcode, hlFirst f, iICODE ix, operDu du, LLInst &atOffset);
    static AstIdent *  idID(const ID *retVal, LOCAL_ID *locsym, iICODE ix_);
    static Expr * id(const LLInst &ll_insn, opLoc sd, Function *pProc, iICODE ix_, ICODE &duIcode, operDu du);

    virtual Expr *clone() const
    {
        return new AstIdent(*this);
    }
    virtual int hlTypeSize(Function *pproc) const;
    virtual hlType expType(Function *pproc) const;
    virtual Expr * performLongRemoval(eReg regi, LOCAL_ID *locId);
    virtual QString walkCondExpr(Function *pProc, int *numLoc) const;
    virtual Expr *insertSubTreeReg(Expr *_expr, eReg regi, const LOCAL_ID *locsym);
    virtual Expr *insertSubTreeLongReg(Expr *_expr, int longIdx);
    virtual bool xClear(rICODE range_to_check, iICODE lastBBinst, const LOCAL_ID &locId);
protected:
    eReg otherLongRegi (eReg regi, int idx, LOCAL_ID *locTbl);

};
struct GlobalVariable : public AstIdent
{
    bool valid;
    int globIdx;
    virtual Expr *clone() const
    {
        return new GlobalVariable(*this);
    }
    GlobalVariable(int16_t segValue, int16_t off);
    QString walkCondExpr(Function *pProc, int *numLoc) const;
    int hlTypeSize(Function *pproc) const;
    hlType expType(Function *pproc) const;
};
struct GlobalVariableIdx : public AstIdent
{
    bool valid;
    int idxGlbIdx;	/* idx into localId, GLOB_VAR_IDX   */

    virtual Expr *clone() const
    {
        return new GlobalVariableIdx(*this);
    }
    GlobalVariableIdx(int16_t segValue, int16_t off, uint8_t regi, const LOCAL_ID *locSym);
    QString walkCondExpr(Function *pProc, int *numLoc) const;
    int hlTypeSize(Function *pproc) const;
    hlType expType(Function *pproc) const;
};
struct Constant : public AstIdent
{
    struct _kte
    {			/* for CONSTANT only					*/
        uint32_t   kte;   	/*   value of the constant			*/
        uint8_t    size;       /*   #bytes size constant	 		*/
    } kte;

    Constant(uint32_t _kte, uint8_t size)
    {
        ident.idType = CONSTANT;
        kte.kte = _kte;
        kte.size = size;
    }
    virtual Expr *clone() const
    {
        return new Constant(*this);
    }
    QString walkCondExpr(Function *pProc, int *numLoc) const;
    int hlTypeSize(Function *pproc) const;
    hlType expType(Function *pproc) const;
};
struct FuncNode : public AstIdent
{
    struct _call {			/* for FUNCTION only				*/
        Function     *proc;
        STKFRAME *args;
    } call;

    FuncNode(Function *pproc, STKFRAME *args)
    {
        call.proc = pproc;
        call.args = args;
    }
    virtual Expr *clone() const
    {
        return new FuncNode(*this);
    }
    QString walkCondExpr(Function *pProc, int *numLoc) const;
    int hlTypeSize(Function *pproc) const;
    hlType expType(Function *pproc) const;
};
struct RegisterNode : public AstIdent
{
    const LOCAL_ID *m_syms;
    regType     regiType;  /* for REGISTER only                */
    int         regiIdx;   /* index into localId, REGISTER		*/

    virtual Expr *insertSubTreeReg(Expr *_expr, eReg regi, const LOCAL_ID *locsym);

    RegisterNode(int idx, regType reg_type,const LOCAL_ID *syms)
    {
        m_syms= syms;
        ident.type(REGISTER);
        regiType = reg_type;
        regiIdx = idx;
    }
    RegisterNode(const LLOperand &, LOCAL_ID *locsym);

    //RegisterNode(eReg regi, uint32_t icodeFlg, LOCAL_ID *locsym);
    virtual Expr *clone() const
    {
        return new RegisterNode(*this);
    }
    QString walkCondExpr(Function *pProc, int *numLoc) const;
    int hlTypeSize(Function *) const;
    hlType expType(Function *pproc) const;
    bool xClear(rICODE range_to_check, iICODE lastBBinst, const LOCAL_ID &locId);
};
