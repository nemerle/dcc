/*
 * File: ast.c
 * Purpose: Support module for abstract syntax trees.
 * Date: September 1993
 * (C) Cristina Cifuentes
 */
#include <stdint.h>
#include <malloc.h>		/* For free() */
#include <string>
#include <sstream>
#include <iostream>
#include <cassert>
#include "types.h"
#include "dcc.h"
using namespace std;
/* Index registers **** temp solution */
static const char *idxReg[8] = {"bx+si", "bx+di", "bp+si", "bp+di",
                                "si", "di", "bp", "bx" };
/* Conditional operator symbols in C.  Index by condOp enumeration type */
static const char *condOpSym[] = { " <= ", " < ", " == ", " != ", " > ", " >= ",
                                   " & ", " | ", " ^ ", " ~ ",
                                   " + ", " - ", " * ", " / ",
                                   " >> ", " << ", " % ", " && ", " || " };

#define EXP_SIZE 200		/* Size of the expression buffer */

/* Local expression stack */
//typedef struct _EXP_STK {
//    COND_EXPR       *exp;
//    struct _EXP_STK *next;
//} EXP_STK; - for local expression stack

/* Returns the integer i in C hexadecimal format */
static char *hexStr (uint16_t i)
{
    static char buf[10];
    sprintf (buf, "%s%x", (i > 9) ? "0x" : "", i);
    return (buf);
}


/* Sets the du record for registers according to the du flag    */
void ICODE::setRegDU (byte regi, operDu du_in)
{
    //    printf("%s %d %x\n",__FUNCTION__,regi,int(du_in));
    switch (du_in)
    {
    case eDEF:
        du.def |= duReg[regi];
        du1.numRegsDef++;
        printf("%s du.def |= %x\n",__FUNCTION__,duReg[regi]);
        break;
    case eUSE:
        du.use |= duReg[regi];
        printf("%s du.use |= %x\n",__FUNCTION__,duReg[regi]);
        break;
    case USE_DEF:
        du.def |= duReg[regi];
        du1.numRegsDef++;
        printf("%s du.def |= %x\n",__FUNCTION__,duReg[regi]);
        printf("%s du.use |= %x\n",__FUNCTION__,duReg[regi]);
        du.use |= duReg[regi];
        break;
    case NONE:    /* do nothing */
        break;
    }
}


/* Copies the def, use, or def and use fields of duIcode into pIcode */
void ICODE::copyDU(const ICODE &duIcode, operDu _du, operDu duDu)
{
    //    printf("%s %d,%d from %d to %d\n",__FUNCTION__,int(du),int(duDu),duIcode->ic.ll.opcode,pIcode->ic.ll.opcode);
    switch (_du)
    {
    case eDEF:
        if (duDu == eDEF)
            du.def=duIcode.du.def;
        else
            du.def=duIcode.du.use;
        break;
    case eUSE:
        if (duDu == eDEF)
            du.use=duIcode.du.def;
        else
            du.use =duIcode.du.use;
        break;
    case USE_DEF:
        du = duIcode.du;
        break;
    case NONE:
        assert(false);
        break;
    }
    printf("%s end: %x,%x\n",__FUNCTION__,du.def,du.use);
}


/* Creates a conditional boolean expression and returns it */
COND_EXPR *COND_EXPR::boolOp(COND_EXPR *lhs, COND_EXPR *rhs, condOp op)
{
    //printf("%s:%d\n",__FUNCTION__,int(op));
    COND_EXPR *newExp;

    newExp = new COND_EXPR(BOOLEAN_OP);
    newExp->expr.boolExpr.op = op;
    newExp->expr.boolExpr.lhs = lhs;
    newExp->expr.boolExpr.rhs = rhs;
    return (newExp);
}


/* Returns a unary conditional expression node.  This procedure should
 * only be used with the following conditional node types: NEGATION,
 * ADDRESSOF, DEREFERENCE, POST_INC, POST_DEC, PRE_INC, PRE_DEC	*/
COND_EXPR *COND_EXPR::unary(condNodeType t, COND_EXPR *sub_expr)
{
    COND_EXPR *newExp;

    newExp = new COND_EXPR(t);
    newExp->expr.unaryExp = sub_expr;
    return (newExp);
}


/* Returns an identifier conditional expression node of type GLOB_VAR */
COND_EXPR *COND_EXPR::idGlob (int16 segValue, int16 off)
{
    COND_EXPR *newExp;
    dword adr;
    size_t i;

    newExp = new COND_EXPR(IDENTIFIER);
    newExp->expr.ident.idType = GLOB_VAR;
    adr = opAdr(segValue, off);
    for (i = 0; i < symtab.size(); i++)
        if (symtab[i].label == adr)
            break;
    if (i == symtab.size())
        printf ("Error, glob var not found in symtab\n");
    newExp->expr.ident.idNode.globIdx = i;
    return (newExp);
}


/* Returns an identifier conditional expression node of type REGISTER */
COND_EXPR *COND_EXPR::idReg(byte regi, flags32 icodeFlg, LOCAL_ID *locsym)
{
    COND_EXPR *newExp;

    newExp = new COND_EXPR(IDENTIFIER);
    newExp->expr.ident.idType = REGISTER;
    if ((icodeFlg & B) || (icodeFlg & SRC_B))
    {
        newExp->expr.ident.idNode.regiIdx = locsym->newByteWordReg(TYPE_BYTE_SIGN, regi);
        newExp->expr.ident.regiType = BYTE_REG;
    }
    else    /* word */
    {
        newExp->expr.ident.idNode.regiIdx = locsym->newByteWordReg( TYPE_WORD_SIGN, regi);
        newExp->expr.ident.regiType = WORD_REG;
    }
    return (newExp);
}


/* Returns an identifier conditional expression node of type REGISTER */
COND_EXPR *COND_EXPR::idRegIdx(Int idx, regType reg_type)
{
    COND_EXPR *newExp;

    newExp = new COND_EXPR(IDENTIFIER);
    newExp->expr.ident.idType = REGISTER;
    newExp->expr.ident.regiType = reg_type;
    newExp->expr.ident.idNode.regiIdx = idx;
    return (newExp);
}

/* Returns an identifier conditional expression node of type LOCAL_VAR */
COND_EXPR *COND_EXPR::idLoc(Int off, LOCAL_ID *localId)
{
    COND_EXPR *newExp;
    size_t i;

    newExp = new COND_EXPR(IDENTIFIER);
    newExp->expr.ident.idType = LOCAL_VAR;
    for (i = 0; i < localId->csym(); i++)
        if ((localId->id_arr[i].id.bwId.off == off) &&
                (localId->id_arr[i].id.bwId.regOff == 0))
            break;
    if (i == localId->csym())
        printf ("Error, cannot find local var\n");
    newExp->expr.ident.idNode.localIdx = i;
    sprintf (localId->id_arr[i].name, "loc%ld", i);
    return (newExp);
}


/* Returns an identifier conditional expression node of type PARAM */
COND_EXPR *COND_EXPR::idParam(Int off, const STKFRAME * argSymtab)
{
    COND_EXPR *newExp;
    size_t i;

    newExp = new COND_EXPR(IDENTIFIER);
    newExp->expr.ident.idType = PARAM;
    for (i = 0; i < argSymtab->sym.size(); i++)
        if (argSymtab->sym[i].off == off)
            break;
    if (i == argSymtab->sym.size()) printf ("Error, cannot find argument var\n");
    newExp->expr.ident.idNode.localIdx = i;
    return (newExp);
}


/* Returns an identifier conditional expression node of type GLOB_VAR_IDX.
 * This global variable is indexed by regi.     */
COND_EXPR *idCondExpIdxGlob (int16 segValue, int16 off, byte regi, const LOCAL_ID *locSym)
{
    COND_EXPR *newExp;
    size_t i;

    newExp = new COND_EXPR(IDENTIFIER);
    newExp->expr.ident.idType = GLOB_VAR_IDX;
    for (i = 0; i < locSym->csym(); i++)
        if ((locSym->id_arr[i].id.bwGlb.seg == segValue) &&
                (locSym->id_arr[i].id.bwGlb.off == off) &&
                (locSym->id_arr[i].id.bwGlb.regi == regi))
            break;
    if (i == locSym->csym())
        printf ("Error, indexed-glob var not found in local id table\n");
    newExp->expr.ident.idNode.idxGlbIdx = i;
    return (newExp);
}


/* Returns an identifier conditional expression node of type CONSTANT */
COND_EXPR *COND_EXPR::idKte(dword kte, byte size)
{
    COND_EXPR *newExp = new COND_EXPR(IDENTIFIER);
    newExp->expr.ident.idType = CONSTANT;
    newExp->expr.ident.idNode.kte.kte = kte;
    newExp->expr.ident.idNode.kte.size = size;
    return (newExp);
}


/* Returns an identifier conditional expression node of type LONG_VAR,
 * that points to the given index idx.  */
COND_EXPR *COND_EXPR::idLongIdx (Int idx)
{
    COND_EXPR *newExp = new COND_EXPR(IDENTIFIER);
    newExp->expr.ident.idType = LONG_VAR;
    newExp->expr.ident.idNode.longIdx = idx;
    return (newExp);
}


/* Returns an identifier conditional expression node of type LONG_VAR */
COND_EXPR *COND_EXPR::idLong(LOCAL_ID *localId, opLoc sd, iICODE pIcode, hlFirst f, iICODE ix, operDu du, Int off)
{
    printf("**************** is equal %s ***************** \n",pIcode==ix ? "yes":"no");
    Int idx;
    COND_EXPR *newExp = new COND_EXPR(IDENTIFIER);

    /* Check for long constant and save it as a constant expression */
    if ((sd == SRC) && ((pIcode->ic.ll.flg & I) == I))  /* constant */
    {
        newExp->expr.ident.idType = CONSTANT;
        if (f == HIGH_FIRST)
            newExp->expr.ident.idNode.kte.kte = (pIcode->ic.ll.src.op() << 16) +
                    (pIcode+off)->ic.ll.src.op();
        else        /* LOW_FIRST */
            newExp->expr.ident.idNode.kte.kte =
                    ((pIcode+off)->ic.ll.src.op() << 16)+ pIcode->ic.ll.src.op();
        newExp->expr.ident.idNode.kte.size = 4;
    }
    /* Save it as a long expression (reg, stack or glob) */
    else
    {
        idx = localId->newLong(sd, &(*pIcode), f, ix, du, off);
        newExp->expr.ident.idType = LONG_VAR;
        newExp->expr.ident.idNode.longIdx = idx;
    }
    return (newExp);
}


/* Returns an identifier conditional expression node of type FUNCTION */
COND_EXPR *COND_EXPR::idFunc(Function * pproc, STKFRAME * args)
{
    COND_EXPR *newExp;

    newExp = new COND_EXPR(IDENTIFIER);
    newExp->expr.ident.idType = FUNCTION;
    newExp->expr.ident.idNode.call.proc = pproc;
    newExp->expr.ident.idNode.call.args = args;
    return (newExp);
}


/* Returns an identifier conditional expression node of type OTHER.
 * Temporary solution, should really be encoded as an indexed type (eg.
 * arrays). */
COND_EXPR *COND_EXPR::idOther(byte seg, byte regi, int16 off)
{
    COND_EXPR *newExp;

    newExp = new COND_EXPR(IDENTIFIER);
    newExp->expr.ident.idType = OTHER;
    newExp->expr.ident.idNode.other.seg = seg;
    newExp->expr.ident.idNode.other.regi = regi;
    newExp->expr.ident.idNode.other.off = off;
    return (newExp);
}


/* Returns an identifier conditional expression node of type TYPE_LONG or
 * TYPE_WORD_SIGN	*/
COND_EXPR *COND_EXPR::idID (const ID *retVal, LOCAL_ID *locsym, iICODE ix_)
{
    COND_EXPR *newExp;
    Int idx;

    newExp = new COND_EXPR(IDENTIFIER);
    if (retVal->type == TYPE_LONG_SIGN)
    {
        idx = locsym->newLongReg (TYPE_LONG_SIGN, retVal->id.longId.h,retVal->id.longId.l, ix_);
        newExp->expr.ident.idType = LONG_VAR;
        newExp->expr.ident.idNode.longIdx = idx;
    }
    else if (retVal->type == TYPE_WORD_SIGN)
    {
        newExp->expr.ident.idType = REGISTER;
        newExp->expr.ident.idNode.regiIdx = locsym->newByteWordReg(TYPE_WORD_SIGN, retVal->id.regi);
        newExp->expr.ident.regiType = WORD_REG;
    }
    return (newExp);
}


/* Returns an identifier conditional expression node, according to the given
 * type.
 * Arguments: i : index into the icode array, used for newLongRegId only.
 *            duIcode: icode instruction that needs the du set.
 *            du: operand is defined or used in current instruction.    */
COND_EXPR *COND_EXPR::id(const ICODE &pIcode, opLoc sd, Function * pProc, iICODE ix_,ICODE &duIcode, operDu du)
{
    COND_EXPR *newExp;

    Int idx;          /* idx into pIcode->localId table */

    const LLOperand &pm((sd == SRC) ? pIcode.ic.ll.src : pIcode.ic.ll.dst);

    if (    ((sd == DST) && pIcode.ic.ll.anyFlagSet(IM_DST)) or
            ((sd == SRC) && pIcode.ic.ll.anyFlagSet(IM_SRC)) or
            (sd == LHS_OP))             /* for MUL lhs */
    {                                                   /* implicit dx:ax */
        idx = pProc->localId.newLongReg (TYPE_LONG_SIGN, rDX, rAX, ix_);
        newExp = COND_EXPR::idLongIdx (idx);
        duIcode.setRegDU (rDX, du);
        duIcode.setRegDU (rAX, du);
    }

    else if ((sd == DST) && pIcode.ic.ll.anyFlagSet(IM_TMP_DST))
    {                                                   /* implicit tmp */
        newExp = COND_EXPR::idReg (rTMP, 0, &pProc->localId);
        duIcode.setRegDU(rTMP, (operDu)eUSE);
    }

    else if ((sd == SRC) && pIcode.ic.ll.anyFlagSet(I)) /* constant */
        newExp = COND_EXPR::idKte (pIcode.ic.ll.src.op(), 2);
    else if (pm.regi == 0)                             /* global variable */
        newExp = COND_EXPR::idGlob(pm.segValue, pm.off);
    else if (pm.regi < INDEXBASE)                      /* register */
    {
        newExp = COND_EXPR::idReg (pm.regi, (sd == SRC) ? pIcode.ic.ll.flg :
                                                           pIcode.ic.ll.flg & NO_SRC_B, &pProc->localId);
        duIcode.setRegDU( pm.regi, du);
    }

    else if (pm.off)                                   /* offset */
    {
        if ((pm.seg == rSS) && (pm.regi == INDEXBASE + 6)) /* idx on bp */
        {
            if (pm.off >= 0)                           /* argument */
                newExp = COND_EXPR::idParam (pm.off, &pProc->args);
            else                                        /* local variable */
                newExp = COND_EXPR::idLoc (pm.off, &pProc->localId);
        }
        else if ((pm.seg == rDS) && (pm.regi == INDEXBASE + 7)) /* bx */
        {
            if (pm.off > 0)        /* global variable */
                newExp = idCondExpIdxGlob (pm.segValue, pm.off, rBX,&pProc->localId);
            else
                newExp = COND_EXPR::idOther (pm.seg, pm.regi, pm.off);
            duIcode.setRegDU( rBX, eUSE);
        }
        else                                            /* idx <> bp, bx */
            newExp = COND_EXPR::idOther (pm.seg, pm.regi, pm.off);
        /**** check long ops, indexed global var *****/
    }

    else  /* (pm->regi >= INDEXBASE && pm->off = 0) => indexed && no off */
    {
        if ((pm.seg == rDS) && (pm.regi > INDEXBASE + 3)) /* dereference */
        {
            switch (pm.regi) {
            case INDEXBASE + 4:   newExp = COND_EXPR::idReg(rSI, 0, &pProc->localId);
                duIcode.setRegDU( rSI, du);
                break;
            case INDEXBASE + 5:   newExp = COND_EXPR::idReg(rDI, 0, &pProc->localId);
                duIcode.setRegDU( rDI, du);
                break;
            case INDEXBASE + 6:   newExp = COND_EXPR::idReg(rBP, 0, &pProc->localId);
                break;
            case INDEXBASE + 7:   newExp = COND_EXPR::idReg(rBX, 0, &pProc->localId);
                duIcode.setRegDU( rBX, du);
                break;
            default:
                newExp = 0;
                assert(false);
            }
            newExp = COND_EXPR::unary (DEREFERENCE, newExp);
        }
        else
            newExp = COND_EXPR::idOther (pm.seg, pm.regi, 0);
    }

    return (newExp);
}


/* Returns the identifier type */
condId ICODE::idType(opLoc sd)
{
    LLOperand &pm((sd == SRC) ? ic.ll.src : ic.ll.dst);

    if ((sd == SRC) && ((ic.ll.flg & I) == I))
        return (CONSTANT);
    else if (pm.regi == 0)
        return (GLOB_VAR);
    else if (pm.regi < INDEXBASE)
        return (REGISTER);
    else if ((pm.seg == rSS) && (pm.regi == INDEXBASE))
    {
        if (pm.off >= 0)
            return (PARAM);
        else
            return (LOCAL_VAR);
    }
    else
        return (OTHER);
}


/* Size of hl types */
Int hlSize[] = {2, 1, 1, 2, 2, 4, 4, 4, 2, 2, 1, 4, 4};


/* Returns the type of the expression */
Int hlTypeSize (const COND_EXPR *expr, Function * pproc)
{
    Int first, second;

    if (expr == NULL)
        return (2);		/* for TYPE_UNKNOWN */

    switch (expr->type) {
    case BOOLEAN_OP:
        first = hlTypeSize (expr->expr.boolExpr.lhs, pproc);
        second = hlTypeSize (expr->expr.boolExpr.rhs, pproc);
        if (first > second)
            return (first);
        else
            return (second);

    case NEGATION:	case ADDRESSOF:
    case POST_INC:	case POST_DEC:
    case PRE_INC:		case PRE_DEC:
    case DEREFERENCE: return (hlTypeSize (expr->expr.unaryExp, pproc));

    case IDENTIFIER:
        switch (expr->expr.ident.idType)
        {
        case GLOB_VAR:
            return (symtab[expr->expr.ident.idNode.globIdx].size);
        case REGISTER:
            if (expr->expr.ident.regiType == BYTE_REG)
                return (1);
            else
                return (2);
        case LOCAL_VAR:
            return (hlSize[pproc->localId.id_arr[expr->expr.ident.idNode.localIdx].type]);
        case PARAM:
            return (hlSize[pproc->args.sym[expr->expr.ident.idNode.paramIdx].type]);
        case GLOB_VAR_IDX:
            return (hlSize[pproc->localId.id_arr[expr->expr.ident.idNode.idxGlbIdx].type]);
        case CONSTANT:
            return (expr->expr.ident.idNode.kte.size);
        case STRING:
            return (2);
        case LONG_VAR:
            return (4);
        case FUNCTION:
            return (hlSize[expr->expr.ident.idNode.call.proc->retVal.type]);
        case OTHER:
            return (2);
        } /* eos */
        break;
    }
    return 2;			// CC: is this correct?
}


/* Returns the type of the expression */
hlType expType (const COND_EXPR *expr, Function * pproc)
{
    hlType first, second;

    if (expr == NULL)
        return (TYPE_UNKNOWN);

    switch (expr->type)
    {
    case BOOLEAN_OP:
        first = expType (expr->expr.boolExpr.lhs, pproc);
        second = expType (expr->expr.boolExpr.rhs, pproc);
        if (first != second)
        {
            if (hlTypeSize (expr->expr.boolExpr.lhs, pproc) >
                    hlTypeSize (expr->expr.boolExpr.rhs, pproc))
                return (first);
            else
                return (second);
        }
        else
            return (first);

    case POST_INC: case POST_DEC:
    case PRE_INC:  case PRE_DEC:
    case NEGATION:
        return (expType (expr->expr.unaryExp, pproc));

    case ADDRESSOF:	return (TYPE_PTR);		/***????****/
    case DEREFERENCE:	return (TYPE_PTR);
    case IDENTIFIER:
        switch (expr->expr.ident.idType)
        {
        case GLOB_VAR:
            return (symtab[expr->expr.ident.idNode.globIdx].type);
        case REGISTER:
            if (expr->expr.ident.regiType == BYTE_REG)
                return (TYPE_BYTE_SIGN);
            else
                return (TYPE_WORD_SIGN);
        case LOCAL_VAR:
            return (pproc->localId.id_arr[expr->expr.ident.idNode.localIdx].type);
        case PARAM:
            return (pproc->args.sym[expr->expr.ident.idNode.paramIdx].type);
        case GLOB_VAR_IDX:
            return (pproc->localId.id_arr[expr->expr.ident.idNode.idxGlbIdx].type);
        case CONSTANT:
            return (TYPE_CONST);
        case STRING:
            return (TYPE_STR);
        case LONG_VAR:
            return (pproc->localId.id_arr[expr->expr.ident.idNode.longIdx].type);
        case FUNCTION:
            return (expr->expr.ident.idNode.call.proc->retVal.type);
        case OTHER:
            return (TYPE_UNKNOWN);
        } /* eos */
    case UNKNOWN_OP:
        assert(false);
        return (TYPE_UNKNOWN);
    }
    return TYPE_UNKNOWN;		// CC: Correct?
}


/* Removes the register from the tree.  If the register was part of a long
 * register (eg. dx:ax), the node gets transformed into an integer register
 * node.        */
void removeRegFromLong (byte regi, LOCAL_ID *locId, COND_EXPR *tree)
{
    IDENTTYPE* ident;     	/* ptr to an identifier */
    byte otherRegi;         /* high or low part of long register */

    switch (tree->type) {
    case BOOLEAN_OP:
        break;
    case POST_INC: case POST_DEC:
    case PRE_INC: case PRE_DEC:
    case NEGATION: case ADDRESSOF:
    case DEREFERENCE:
        break;
    case IDENTIFIER:
        ident = &tree->expr.ident;
        if (ident->idType == LONG_VAR)
        {
            otherRegi = otherLongRegi (regi, ident->idNode.longIdx, locId);
            ident->idType = REGISTER;
            ident->regiType = WORD_REG;
            ident->idNode.regiIdx = locId->newByteWordReg(TYPE_WORD_SIGN,otherRegi);
        }
        break;
    }
}


/* Returns the string located in image, formatted in C format. */
static std::string getString (Int offset)
{
    ostringstream o;
    Int strLen, i;

    strLen = strSize (&prog.Image[offset], '\0');
    o << '"';
    for (i = 0; i < strLen; i++)
        o<<cChar(prog.Image[offset+i]);
    o << "\"\0";
    return (o.str());
}


/* Walks the conditional expression tree and returns the result on a string */
// TODO: use string stream here
string walkCondExpr (const COND_EXPR* expr, Function * pProc, Int* numLoc)
{
    int16 off;              /* temporal - for OTHER */
    ID* id;                 /* Pointer to local identifier table */
    //char* o;              /* Operand string pointer */
    bool needBracket;       /* Determine whether parenthesis is needed */
    BWGLB_TYPE* bwGlb;      /* Ptr to BWGLB_TYPE (global indexed var) */
    STKSYM * psym;          /* Pointer to argument in the stack */
    std::ostringstream outStr;

    if (expr == NULL)
        return "";

    needBracket = true;
    switch (expr->type)
    {
    case BOOLEAN_OP:
        outStr << "(";
        outStr << walkCondExpr(expr->expr.boolExpr.lhs, pProc, numLoc);
        outStr << condOpSym[expr->expr.boolExpr.op];
        outStr << walkCondExpr(expr->expr.boolExpr.rhs, pProc, numLoc);
        outStr << ")";
        break;

    case NEGATION:
        if (expr->expr.unaryExp->type == IDENTIFIER)
        {
            needBracket = FALSE;
            outStr << "!";
        }
        else
            outStr << "! (";
        outStr << walkCondExpr (expr->expr.unaryExp, pProc, numLoc);
        if (needBracket == TRUE)
            outStr << ")";
        break;

    case ADDRESSOF:
        if (expr->expr.unaryExp->type == IDENTIFIER)
        {
            needBracket = FALSE;
            outStr << "&";
        }
        else
            outStr << "&(";
        outStr << walkCondExpr (expr->expr.unaryExp, pProc, numLoc);
        if (needBracket == TRUE)
            outStr << ")";
        break;

    case DEREFERENCE:
        outStr << "*";
        if (expr->expr.unaryExp->type == IDENTIFIER)
            needBracket = FALSE;
        else
            outStr << "(";
        outStr << walkCondExpr (expr->expr.unaryExp, pProc, numLoc);
        if (needBracket == TRUE)
            outStr << ")";
        break;

    case POST_INC:
        outStr <<  walkCondExpr (expr->expr.unaryExp, pProc, numLoc) << "++";
        break;

    case POST_DEC:
        outStr <<  walkCondExpr (expr->expr.unaryExp, pProc, numLoc) << "--";
        break;

    case PRE_INC:
        outStr << "++"<< walkCondExpr (expr->expr.unaryExp, pProc, numLoc);
        break;

    case PRE_DEC:
        outStr << "--"<< walkCondExpr (expr->expr.unaryExp, pProc, numLoc);
        break;

    case IDENTIFIER:
        std::ostringstream o;
        switch (expr->expr.ident.idType)
        {
        case GLOB_VAR:
            o << symtab[expr->expr.ident.idNode.globIdx].name;
            break;
        case REGISTER:
            id = &pProc->localId.id_arr[expr->expr.ident.idNode.regiIdx];
            if (id->name[0] == '\0')	/* no name */
            {
                sprintf (id->name, "loc%ld", ++(*numLoc));
                if (id->id.regi < rAL)
                    cCode.appendDecl("%s %s; /* %s */\n",hlTypes[id->type], id->name,wordReg[id->id.regi - rAX]);
                else
                    cCode.appendDecl("%s %s; /* %s */\n",hlTypes[id->type], id->name,byteReg[id->id.regi - rAL]);
            }
            if (id->hasMacro)
                o << id->macro << "("<<id->name<<")";
            else
                o << id->name;
            break;

        case LOCAL_VAR:
            o << pProc->localId.id_arr[expr->expr.ident.idNode.localIdx].name;
            break;

        case PARAM:
            psym = &pProc->args.sym[expr->expr.ident.idNode.paramIdx];
            if (psym->hasMacro)
                o << psym->macro<<"("<<psym->name<< ")";
            else
                o << psym->name;
            break;

        case GLOB_VAR_IDX:
            bwGlb = &pProc->localId.id_arr[expr->expr.ident.idNode.idxGlbIdx].id.bwGlb;
            o << (bwGlb->seg << 4) + bwGlb->off <<  "["<<wordReg[bwGlb->regi - rAX]<<"]";
            break;

        case CONSTANT:
            if (expr->expr.ident.idNode.kte.kte < 1000)
                o << expr->expr.ident.idNode.kte.kte;
            else
                o << "0x"<<std::hex << expr->expr.ident.idNode.kte.kte;
            break;

        case STRING:
            o << getString (expr->expr.ident.idNode.strIdx);
            break;

        case LONG_VAR:
            id = &pProc->localId.id_arr[expr->expr.ident.idNode.longIdx];
            if (id->name[0] != '\0') /* STK_FRAME & REG w/name*/
                o << id->name;
            else if (id->loc == REG_FRAME)
            {
                sprintf (id->name, "loc%ld", ++(*numLoc));
                cCode.appendDecl("%s %s; /* %s:%s */\n",hlTypes[id->type], id->name,wordReg[id->id.longId.h - rAX],wordReg[id->id.longId.l - rAX]);
                o << id->name;
                pProc->localId.propLongId (id->id.longId.l,id->id.longId.h, id->name);
            }
            else    /* GLB_FRAME */
            {
                if (id->id.longGlb.regi == 0)  /* not indexed */
                    o << "[" << (id->id.longGlb.seg<<4) + id->id.longGlb.offH <<"]";
                else if (id->id.longGlb.regi == rBX)
                    o << "[" << (id->id.longGlb.seg<<4) + id->id.longGlb.offH <<"][bx]";
            }
            break;

        case FUNCTION:
            o << writeCall (expr->expr.ident.idNode.call.proc,expr->expr.ident.idNode.call.args, pProc, numLoc);
            break;

        case OTHER:
            off = expr->expr.ident.idNode.other.off;
            o << wordReg[expr->expr.ident.idNode.other.seg - rAX]<< "[";
            o << idxReg[expr->expr.ident.idNode.other.regi - INDEXBASE];
            if (off < 0)
                o << "-"<< hexStr (-off);
            else if (off>0)
                o << "+"<< hexStr (off);
            o << "]";
        } /* eos */
        outStr << o.str();
        break;
    }

    return outStr.str();
}


/* Makes a copy of the given expression.  Allocates newExp storage for each
 * node.  Returns the copy. */
COND_EXPR *COND_EXPR::clone()
{
    COND_EXPR* newExp=0;        /* Expression node copy */

    switch (type)
    {
    case BOOLEAN_OP:
        newExp = new COND_EXPR(*this);
        newExp->expr.boolExpr.lhs = expr.boolExpr.lhs->clone();
        newExp->expr.boolExpr.rhs = expr.boolExpr.rhs->clone();
        break;

    case NEGATION:
    case ADDRESSOF:
    case DEREFERENCE:
        newExp = new COND_EXPR(*this);
        newExp->expr.unaryExp = expr.unaryExp->clone();
        break;

    case IDENTIFIER:
        return new COND_EXPR(*this);
    }
    return (newExp);
}


/* Changes the boolean conditional operator at the root of this expression */
void COND_EXPR::changeBoolOp (condOp newOp)
{
    expr.boolExpr.op = newOp;
}


/* Inserts the expression exp into the tree at the location specified by the
 * register regi */
bool insertSubTreeReg (COND_EXPR *expr, COND_EXPR **tree, byte regi,LOCAL_ID *locsym)
{
    byte treeReg;

    if (*tree == NULL)
        return FALSE;

    switch ((*tree)->type) {
    case IDENTIFIER:
        if ((*tree)->expr.ident.idType == REGISTER)
        {
            treeReg = locsym->id_arr[(*tree)->expr.ident.idNode.regiIdx].id.regi;
            if (treeReg == regi)                        /* word reg */
            {
                *tree = expr;
                return TRUE;
            }
            else if ((regi >= rAX) && (regi <= rBX))    /* word/byte reg */
            {
                if ((treeReg == (regi + rAL-1)) || (treeReg == (regi + rAH-1)))
                {
                    *tree = expr;
                    return TRUE;
                }
            }
        }
        return FALSE;

    case BOOLEAN_OP:
        if (insertSubTreeReg (expr, &(*tree)->expr.boolExpr.lhs, regi, locsym))
            return TRUE;
        if (insertSubTreeReg (expr, &(*tree)->expr.boolExpr.rhs, regi, locsym))
            return TRUE;
        return FALSE;

    case NEGATION:
    case ADDRESSOF:
    case DEREFERENCE:
        if (insertSubTreeReg(expr, &(*tree)->expr.unaryExp,regi, locsym))
            return TRUE;
        return FALSE;
    }
    return FALSE;
}


/* Inserts the expression exp into the tree at the location specified by the
 * long register index longIdx*/
bool insertSubTreeLongReg(COND_EXPR *exp, COND_EXPR **tree, Int longIdx)
{
    switch ((*tree)->type)
    {
    case IDENTIFIER:
        if ((*tree)->expr.ident.idNode.longIdx == longIdx)
        {
            *tree = exp;
            return true;
        }
        return false;

    case BOOLEAN_OP:
        if (insertSubTreeLongReg (exp, &(*tree)->expr.boolExpr.lhs, longIdx))
            return true;
        if (insertSubTreeLongReg (exp, &(*tree)->expr.boolExpr.rhs, longIdx))
            return true;
        return false;

    case NEGATION:
    case ADDRESSOF:
    case DEREFERENCE:
        if (insertSubTreeLongReg (exp, &(*tree)->expr.unaryExp, longIdx))
            return true;
        return false;
    }
    return false;
}


/* Recursively deallocates the abstract syntax tree rooted at *exp */
void COND_EXPR::release()
{
    switch (type)
    {
    case BOOLEAN_OP:
        expr.boolExpr.lhs->release();
        expr.boolExpr.rhs->release();
        break;
    case NEGATION:
    case ADDRESSOF:
    case DEREFERENCE:
        expr.unaryExp->release();
        break;
    }
    delete (this);
}
