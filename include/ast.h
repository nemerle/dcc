/*
 * File:    ast.h
 * Purpose: definition of the abstract syntax tree ADT.
 * Date:    September 1993
 * (C) Cristina Cifuentes
 */
#pragma once
static const int operandSize=20;
#include <cstring>
#include "Enums.h"
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

static const condOp invCondOpJCond[12] = {GREATER_EQUAL, GREATER, LESS, LESS_EQUAL,
                                    NOT_EQUAL, EQUAL, GREATER_EQUAL, LESS,
                                    GREATER, LESS_EQUAL, LESS, GREATER_EQUAL};

struct Function;
struct STKFRAME;
struct LOCAL_ID;
struct ICODE;
struct ID;
#include "IdentType.h"
//enum opLoc;
//enum hlFirst;
//enum operDu;
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
    static COND_EXPR *idGlob(int16 segValue, int16 off);
    static COND_EXPR *idRegIdx(Int idx, regType reg_type);
    static COND_EXPR *idKte(dword kte, byte size);
    static COND_EXPR *idLoc(Int off, LOCAL_ID *localId);
    static COND_EXPR *idReg(byte regi, flags32 icodeFlg, LOCAL_ID *locsym);
    static COND_EXPR *idLongIdx(Int idx);
    static COND_EXPR *idOther(byte seg, byte regi, int16 off);
    static COND_EXPR *idParam(Int off, const STKFRAME *argSymtab);
    static COND_EXPR *unary(condNodeType t, COND_EXPR *sub_expr);
    static COND_EXPR *idLong(LOCAL_ID *localId, opLoc sd, ICODE *pIcode, hlFirst f, Int ix, operDu du, Int off);
    static COND_EXPR *idFunc(Function *pproc, STKFRAME *args);
    static COND_EXPR *idID(const ID *retVal, LOCAL_ID *locsym, Int ix);
    static COND_EXPR *id(ICODE *pIcode, opLoc sd, Function *pProc, Int i, ICODE *duIcode, operDu du);
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
    COND_EXPR()
    {
        type=UNKNOWN_OP;
        memset(&expr,0,sizeof(_exprNode));
    }
};

/* Sequence of conditional expression data type */
/*** NOTE: not used at present ****/
//struct SEQ_COND_EXPR
//{
//    COND_EXPR           *expr;
//    struct _condExpSeq  *neccxt;
//};


