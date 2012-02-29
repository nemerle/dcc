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
struct ID;
typedef std::list<ICODE>::iterator iICODE;
#include "IdentType.h"

/* Expression data type */
struct COND_EXPR
{
    condNodeType            type;       /* Conditional Expression Node Type */
    union _exprNode {                   /* Different cond expr nodes        */
        struct /* for BOOLEAN_OP					*/
        {
            condOp           op;
            COND_EXPR *lhs;
            COND_EXPR *rhs;
        } boolExpr;
        COND_EXPR    *unaryExp;  /* for NEGATION,ADDRESSOF,DEREFERENCE*/
        IDENTTYPE   ident;     /* for IDENTIFIER                   */
    }   expr;
public:
    static COND_EXPR *idGlob(int16_t segValue, int16_t off);
    static COND_EXPR *idRegIdx(int idx, regType reg_type);
    static COND_EXPR *idKte(uint32_t kte, uint8_t size);
    static COND_EXPR *idLoc(int off, LOCAL_ID *localId);
    static COND_EXPR *idReg(uint8_t regi, uint32_t icodeFlg, LOCAL_ID *locsym);
    static COND_EXPR *idLongIdx(int idx);
    static COND_EXPR *idOther(uint8_t seg, uint8_t regi, int16_t off);
    static COND_EXPR *idParam(int off, const STKFRAME *argSymtab);
    static COND_EXPR *unary(condNodeType t, COND_EXPR *sub_expr);
    static COND_EXPR *idLong(LOCAL_ID *localId, opLoc sd, iICODE pIcode, hlFirst f, iICODE ix, operDu du, iICODE atOffset);
    static COND_EXPR *idFunc(Function *pproc, STKFRAME *args);
    static COND_EXPR *idID(const ID *retVal, LOCAL_ID *locsym, iICODE ix_);
    static COND_EXPR *id(const ICODE &pIcode, opLoc sd, Function *pProc, iICODE ix_, ICODE &duIcode, operDu du);
    static COND_EXPR *boolOp(COND_EXPR *lhs, COND_EXPR *rhs, condOp op);
public:
    COND_EXPR *clone();
    void release();
    void changeBoolOp(condOp newOp);
    COND_EXPR(COND_EXPR &other)
    {
        type=other.type;
        expr=other.expr;
    }
    COND_EXPR(condNodeType t=UNKNOWN_OP) : type(t)
    {
        memset(&expr,0,sizeof(_exprNode));
    }
public:
    COND_EXPR *inverse(); // return new COND_EXPR that is invarse of this
};


