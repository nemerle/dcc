/*
 * File:    ast.h
 * Purpose: definition of the abstract syntax tree ADT.
 * Date:    September 1993
 * (C) Cristina Cifuentes
 */
#pragma once
#include <cstring>
#include <list>
#include <boost/range/iterator_range.hpp>
#include "Enums.h"
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
struct ID;
typedef std::list<ICODE>::iterator iICODE;
typedef boost::iterator_range<iICODE> rICODE;
#include "IdentType.h"

/* Expression data type */
struct COND_EXPR
{
public:
    condNodeType            m_type;     /* Conditional Expression Node Type */
public:
    static bool         insertSubTreeLongReg(COND_EXPR *exp, COND_EXPR *&tree, int longIdx);
    static bool         insertSubTreeReg(COND_EXPR *&tree, COND_EXPR *_expr, eReg regi, const LOCAL_ID *locsym);
    static bool         insertSubTreeReg(AstIdent *&tree, COND_EXPR *_expr, eReg regi, const LOCAL_ID *locsym);
public:
    virtual COND_EXPR *clone() const;
    void release();
    COND_EXPR(condNodeType t=UNKNOWN_OP) : m_type(t)
    {

    }
    virtual ~COND_EXPR();
public:
    virtual std::string walkCondExpr (Function * pProc, int* numLoc) const=0;
    virtual COND_EXPR *inverse() const=0; // return new COND_EXPR that is invarse of this
    virtual bool xClear(rICODE range_to_check, iICODE lastBBinst, const LOCAL_ID &locId)=0;
    virtual COND_EXPR *insertSubTreeReg(COND_EXPR *_expr, eReg regi, const LOCAL_ID *locsym)=0;
    virtual COND_EXPR *insertSubTreeLongReg(COND_EXPR *_expr, int longIdx)=0;
    virtual hlType expType(Function *pproc) const;
    virtual int hlTypeSize(Function *pproc) const=0;
    virtual void performLongRemoval(eReg regi, LOCAL_ID *locId) {}
};
struct UnaryOperator : public COND_EXPR
{
    UnaryOperator(condNodeType t=UNKNOWN_OP) : COND_EXPR(t),unaryExp(nullptr) {}
    COND_EXPR *unaryExp;
    virtual COND_EXPR *inverse() const
    {
        if (m_type == NEGATION) //TODO: memleak here
        {
            return unaryExp->clone();
        }
        return this->clone();
    }
    virtual COND_EXPR *clone() const
    {
        UnaryOperator *newExp = new UnaryOperator(*this);
        newExp->unaryExp = unaryExp->clone();
        return newExp;
    }
    virtual bool xClear(rICODE range_to_check, iICODE lastBBinst, const LOCAL_ID &locs);
    static UnaryOperator *Create(condNodeType t, COND_EXPR *sub_expr)
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
    virtual std::string walkCondExpr(Function *pProc, int *numLoc) const;
    virtual COND_EXPR *insertSubTreeReg(COND_EXPR *_expr, eReg regi, const LOCAL_ID *locsym);
    virtual hlType expType(Function *pproc) const;
    virtual COND_EXPR *insertSubTreeLongReg(COND_EXPR *_expr, int longIdx);
};

struct BinaryOperator : public COND_EXPR
{
    condOp      m_op;
    COND_EXPR *m_lhs;
    COND_EXPR *m_rhs;
    BinaryOperator(condOp o) : COND_EXPR(BOOLEAN_OP)
    {
        m_op = o;
        m_lhs=m_rhs=nullptr;
    }
    BinaryOperator(condOp o,COND_EXPR *l,COND_EXPR *r) : COND_EXPR(BOOLEAN_OP)
    {
        m_op = o;
        m_lhs=l;
        m_rhs=r;
    }
    ~BinaryOperator()
    {
        assert(m_lhs!=m_rhs || m_lhs==nullptr);
        delete m_lhs;
        delete m_rhs;
    }
    static BinaryOperator *Create(condOp o,COND_EXPR *l,COND_EXPR *r)
    {
        BinaryOperator *res = new BinaryOperator(o);
        res->m_lhs = l;
        res->m_rhs = r;
        return res;
    }
    static BinaryOperator *LogicAnd(COND_EXPR *l,COND_EXPR *r)
    {
        return new BinaryOperator(DBL_AND,l,r);
    }
    static BinaryOperator *And(COND_EXPR *l,COND_EXPR *r)
    {
        return new BinaryOperator(AND,l,r);
    }
    static BinaryOperator *Or(COND_EXPR *l,COND_EXPR *r)
    {
        return new BinaryOperator(OR,l,r);
    }
    static BinaryOperator *LogicOr(COND_EXPR *l,COND_EXPR *r)
    {
        return new BinaryOperator(DBL_OR,l,r);
    }
    static BinaryOperator *CreateAdd(COND_EXPR *l,COND_EXPR *r);
    void changeBoolOp(condOp newOp);
    virtual COND_EXPR *inverse() const;
    virtual COND_EXPR *clone() const;
    virtual bool xClear(rICODE range_to_check, iICODE lastBBinst, const LOCAL_ID &locs);
    virtual COND_EXPR *insertSubTreeReg(COND_EXPR *_expr, eReg regi, const LOCAL_ID *locsym);
    virtual COND_EXPR *insertSubTreeLongReg(COND_EXPR *_expr, int longIdx);
    const COND_EXPR *lhs() const
    {
        return const_cast<const COND_EXPR *>(const_cast<BinaryOperator *>(this)->lhs());
    }
    const COND_EXPR *rhs() const
    {
        return const_cast<const COND_EXPR *>(const_cast<BinaryOperator *>(this)->rhs());
    }

    COND_EXPR *lhs()
    {
        assert(m_type==BOOLEAN_OP);
        return m_lhs;
    }
    COND_EXPR *rhs()
    {
        assert(m_type==BOOLEAN_OP);
        return m_rhs;
    }
    condOp op() const { return m_op;}
    /* Changes the boolean conditional operator at the root of this expression */
    void op(condOp o) { m_op=o;}
    std::string walkCondExpr (Function * pProc, int* numLoc) const;
public:
    hlType expType(Function *pproc) const;
    int hlTypeSize(Function *pproc) const;
};
struct AstIdent : public UnaryOperator
{
    AstIdent() : UnaryOperator(IDENTIFIER)
    {
        memset(&ident,0,sizeof(ident));
    }
    virtual COND_EXPR *clone() const
    {
        return new AstIdent(*this);
    }
    IDENTTYPE   ident;              /* for IDENTIFIER                   */
    static AstIdent *  RegIdx(int idx, regType reg_type);
    static AstIdent *  Kte(uint32_t kte, uint8_t size);
    static AstIdent *  Loc(int off, LOCAL_ID *localId);
    static AstIdent *  Reg(eReg regi, uint32_t icodeFlg, LOCAL_ID *locsym);
    static AstIdent *  LongIdx(int idx);
    static AstIdent *  Other(eReg seg, eReg regi, int16_t off);
    static AstIdent *  idParam(int off, const STKFRAME *argSymtab);
    static AstIdent *  idLong(LOCAL_ID *localId, opLoc sd, iICODE pIcode, hlFirst f, iICODE ix, operDu du, LLInst &atOffset);
    static AstIdent *  idFunc(Function *pproc, STKFRAME *args);
    static AstIdent *  idID(const ID *retVal, LOCAL_ID *locsym, iICODE ix_);
    static COND_EXPR * id(const LLInst &ll_insn, opLoc sd, Function *pProc, iICODE ix_, ICODE &duIcode, operDu du);

    virtual int hlTypeSize(Function *pproc) const;
    virtual hlType expType(Function *pproc) const;
    virtual void performLongRemoval(eReg regi, LOCAL_ID *locId);
    virtual std::string walkCondExpr(Function *pProc, int *numLoc) const;
    virtual COND_EXPR *insertSubTreeReg(COND_EXPR *_expr, eReg regi, const LOCAL_ID *locsym);
    virtual COND_EXPR *insertSubTreeLongReg(COND_EXPR *_expr, int longIdx);
    virtual bool xClear(rICODE range_to_check, iICODE lastBBinst, const LOCAL_ID &locId);
protected:
    eReg otherLongRegi (eReg regi, int idx, LOCAL_ID *locTbl);

};
struct GlobalVariable : public AstIdent
{
    static AstIdent *Create(int16_t segValue, int16_t off);
};
struct Constant : public COND_EXPR
{};
