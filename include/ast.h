/*
 * File:    ast.h
 * Purpose: definition of the abstract syntax tree ADT.
 * Date:    September 1993
 * (C) Cristina Cifuentes
 */
#pragma once
#include <cstring>
#include <list>
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

struct Function;
struct STKFRAME;
struct LOCAL_ID;
struct ICODE;
struct LLInst;
struct ID;
typedef std::list<ICODE>::iterator iICODE;
#include "IdentType.h"

/* Expression data type */
struct COND_EXPR
{
protected:
    struct /* for BOOLEAN_OP					*/
    {
        condOp           op;
        COND_EXPR *lhs;
        COND_EXPR *rhs;
    } boolExpr;

public:
    condNodeType            m_type;       /* Conditional Expression Node Type */
    union _exprNode {                   /* Different cond expr nodes        */
        COND_EXPR    *unaryExp;  /* for NEGATION,ADDRESSOF,DEREFERENCE*/
        IDENTTYPE   ident;     /* for IDENTIFIER                   */
    }   expr;
    COND_EXPR *lhs()
    {
        assert(m_type==BOOLEAN_OP);
        return boolExpr.lhs;
    }
    const COND_EXPR *lhs() const
    {
        assert(m_type==BOOLEAN_OP);
        return boolExpr.lhs;
    }
    COND_EXPR *rhs()
    {
        assert(m_type==BOOLEAN_OP);
        return boolExpr.rhs;
    }
    const COND_EXPR *rhs() const
    {
        assert(m_type==BOOLEAN_OP);
        return boolExpr.rhs;
    }
    condOp op() const { return boolExpr.op;}
public:
    static COND_EXPR *idRegIdx(int idx, regType reg_type);
    static COND_EXPR *idKte(uint32_t kte, uint8_t size);
    static COND_EXPR *idLoc(int off, LOCAL_ID *localId);
    static COND_EXPR *idReg(eReg regi, uint32_t icodeFlg, LOCAL_ID *locsym);
    static COND_EXPR *idLongIdx(int idx);
    static COND_EXPR *idOther(eReg seg, eReg regi, int16_t off);
    static COND_EXPR *idParam(int off, const STKFRAME *argSymtab);
    static COND_EXPR *unary(condNodeType t, COND_EXPR *sub_expr);
    static COND_EXPR *idLong(LOCAL_ID *localId, opLoc sd, iICODE pIcode, hlFirst f, iICODE ix, operDu du, iICODE atOffset);
    static COND_EXPR *idFunc(Function *pproc, STKFRAME *args);
    static COND_EXPR *idID(const ID *retVal, LOCAL_ID *locsym, iICODE ix_);
    static COND_EXPR *  id(const LLInst &ll_insn, opLoc sd, Function *pProc, iICODE ix_, ICODE &duIcode, operDu du);
    static COND_EXPR *boolOp(COND_EXPR *_lhs, COND_EXPR *_rhs, condOp _op);
    static bool         insertSubTreeLongReg(COND_EXPR *exp, COND_EXPR **tree, int longIdx);
    static bool         insertSubTreeReg(COND_EXPR *&tree, COND_EXPR *_expr, eReg regi, LOCAL_ID *locsym);
public:
    virtual COND_EXPR *clone() const;
    void release();
    void changeBoolOp(condOp newOp);
    COND_EXPR(const COND_EXPR &other)
    {
        m_type=other.m_type;
        expr=other.expr;
        boolExpr=other.boolExpr;
    }
    COND_EXPR(condNodeType t=UNKNOWN_OP) : m_type(t)
    {
        memset(&expr,0,sizeof(_exprNode));
        memset(&boolExpr,0,sizeof(boolExpr));

    }
    virtual ~COND_EXPR() {}
public:
    virtual COND_EXPR *inverse() const; // return new COND_EXPR that is invarse of this
    virtual bool xClear(iICODE f, iICODE t, iICODE lastBBinst, Function *pproc);
    virtual COND_EXPR *insertSubTreeReg(COND_EXPR *_expr, eReg regi, LOCAL_ID *locsym);
    virtual COND_EXPR *insertSubTreeLongReg(COND_EXPR *_expr, int longIdx);
    virtual hlType expType(Function *pproc) const;
};
struct BinaryOperator : public COND_EXPR
{
    condOp      m_op;
    COND_EXPR *m_lhs;
    COND_EXPR *m_rhs;
    BinaryOperator(condOp o)
    {
        m_op = o;
        m_lhs=m_rhs=nullptr;
    }
    static BinaryOperator *Create(condOp o,COND_EXPR *l,COND_EXPR *r);
    static BinaryOperator *CreateAdd(COND_EXPR *l,COND_EXPR *r);
    virtual COND_EXPR *inverse();
    virtual COND_EXPR *clone();
    virtual bool xClear(iICODE f, iICODE t, iICODE lastBBinst, Function *pproc);
    virtual COND_EXPR *insertSubTreeReg(COND_EXPR *_expr, eReg regi, LOCAL_ID *locsym);
    virtual COND_EXPR *insertSubTreeLongReg(COND_EXPR *_expr, int longIdx);

    COND_EXPR *lhs()
    {
        assert(m_type==BOOLEAN_OP);
        return m_lhs;
    }
    const COND_EXPR *lhs() const
    {
        assert(m_type==BOOLEAN_OP);
        return m_lhs;
    }
    COND_EXPR *rhs()
    {
        assert(m_type==BOOLEAN_OP);
        return m_rhs;
    }
    const COND_EXPR *rhs() const
    {
        assert(m_type==BOOLEAN_OP);
        return m_rhs;
    }
    condOp op() const { return m_op;}
    /* Changes the boolean conditional operator at the root of this expression */
    void op(condOp o) { m_op=o;}
};
struct UnaryOperator : public COND_EXPR
{
    condOp      op;
    COND_EXPR *unaryExp;
    virtual COND_EXPR *inverse();
    virtual COND_EXPR *clone();
    virtual bool xClear(iICODE f, iICODE t, iICODE lastBBinst, Function *pproc);
    static UnaryOperator *Create(condNodeType t, COND_EXPR *sub_expr)
    {
        UnaryOperator *newExp = new UnaryOperator();
        newExp->m_type=t;
        newExp->unaryExp = sub_expr;
        return (newExp);
    }
};

struct GlobalVariable : public COND_EXPR
{
    static COND_EXPR *Create(int16_t segValue, int16_t off);
};
struct Constant : public COND_EXPR
{};
