/*
 * File: ast.c
 * Purpose: Support module for abstract syntax trees.
 * Date: September 1993
 * (C) Cristina Cifuentes
 */
#include <stdint.h>
#include <string>
#include <sstream>
#include <iostream>
#include <cassert>
#include "types.h"
#include "dcc.h"
#include "machine_x86.h"
using namespace std;
// Conditional operator symbols in C.  Index by condOp enumeration type
static const char * const condOpSym[] = { " <= ", " < ", " == ", " != ", " > ", " >= ",
                                          " & ", " | ", " ^ ", " ~ ",
                                          " + ", " - ", " * ", " / ",
                                          " >> ", " << ", " % ", " && ", " || " };


/* Local expression stack */
//typedef struct _EXP_STK {
//    COND_EXPR       *exp;
//    struct _EXP_STK *next;
//} EXP_STK; - for local expression stack

/* Returns the integer i in C hexadecimal format */
static const char *hexStr (uint16_t i)
{
    static char buf[10];
    sprintf (buf, "%s%x", (i > 9) ? "0x" : "", i);
    return (buf);
}


/* Sets the du record for registers according to the du flag    */
void ICODE::setRegDU (eReg regi, operDu du_in)
{
    //    printf("%s %d %x\n",__FUNCTION__,regi,int(du_in));
    switch (du_in)
    {
    case eDEF:
        du.def |= duReg[regi];
        du1.numRegsDef++;
        break;
    case eUSE:
        du.use |= duReg[regi];
        break;
    case USE_DEF:
        du.def |= duReg[regi];
        du.use |= duReg[regi];
        du1.numRegsDef++;
        break;
    case NONE:    /* do nothing */
        break;
    }
}


/* Copies the def, use, or def and use fields of duIcode into pIcode */
void ICODE::copyDU(const ICODE &duIcode, operDu _du, operDu duDu)
{
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
}


/* Creates a conditional boolean expression and returns it */
COND_EXPR *COND_EXPR::boolOp(COND_EXPR *_lhs, COND_EXPR *_rhs, condOp _op)
{
    COND_EXPR *newExp;

    newExp = new COND_EXPR(BOOLEAN_OP);
    newExp->boolExpr.op = _op;
    newExp->boolExpr.lhs = _lhs;
    newExp->boolExpr.rhs = _rhs;
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
COND_EXPR *GlobalVariable::Create(int16_t segValue, int16_t off)
{
    COND_EXPR *newExp;
    uint32_t adr;
    size_t i;

    newExp = new COND_EXPR(IDENTIFIER);
    newExp->expr.ident.idType = GLOB_VAR;
    adr = opAdr(segValue, off);
    for (i = 0; i < symtab.size(); i++)
        if (symtab[i].label == adr)
            break;
    if (i == symtab.size())
    {
        printf ("Error, glob var not found in symtab\n");
        delete newExp;
        return 0;
    }
    newExp->expr.ident.idNode.globIdx = i;
    return (newExp);
}


/* Returns an identifier conditional expression node of type REGISTER */
COND_EXPR *COND_EXPR::idReg(eReg regi, uint32_t icodeFlg, LOCAL_ID *locsym)
{
    COND_EXPR *newExp;

    newExp = new COND_EXPR(IDENTIFIER);
    newExp->expr.ident.idType = REGISTER;
    if ((icodeFlg & B) || (icodeFlg & SRC_B))
    {
        newExp->expr.ident.idNode.regiIdx = locsym->newByteWordReg(TYPE_BYTE_SIGN, regi);
        newExp->expr.ident.regiType = BYTE_REG;
    }
    else    /* uint16_t */
    {
        newExp->expr.ident.idNode.regiIdx = locsym->newByteWordReg( TYPE_WORD_SIGN, regi);
        newExp->expr.ident.regiType = WORD_REG;
    }
    return (newExp);
}


/* Returns an identifier conditional expression node of type REGISTER */
COND_EXPR *COND_EXPR::idRegIdx(int idx, regType reg_type)
{
    COND_EXPR *newExp;

    newExp = new COND_EXPR(IDENTIFIER);
    newExp->expr.ident.idType = REGISTER;
    newExp->expr.ident.regiType = reg_type;
    newExp->expr.ident.idNode.regiIdx = idx;
    return (newExp);
}

/* Returns an identifier conditional expression node of type LOCAL_VAR */
COND_EXPR *COND_EXPR::idLoc(int off, LOCAL_ID *localId)
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
    localId->id_arr[i].setLocalName(i);
    return (newExp);
}


/* Returns an identifier conditional expression node of type PARAM */
COND_EXPR *COND_EXPR::idParam(int off, const STKFRAME * argSymtab)
{
    COND_EXPR *newExp;

    newExp = new COND_EXPR(IDENTIFIER);
    newExp->expr.ident.idType = PARAM;
    auto iter=argSymtab->findByLabel(off);
    if (iter == argSymtab->end())
        printf ("Error, cannot find argument var\n");
    newExp->expr.ident.idNode.localIdx = distance(argSymtab->begin(),iter);
    return (newExp);
}


/* Returns an identifier conditional expression node of type GLOB_VAR_IDX.
 * This global variable is indexed by regi.     */
COND_EXPR *idCondExpIdxGlob (int16_t segValue, int16_t off, uint8_t regi, const LOCAL_ID *locSym)
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
COND_EXPR *COND_EXPR::idKte(uint32_t kte, uint8_t size)
{
    COND_EXPR *newExp = new COND_EXPR(IDENTIFIER);
    newExp->expr.ident.idType = CONSTANT;
    newExp->expr.ident.idNode.kte.kte = kte;
    newExp->expr.ident.idNode.kte.size = size;
    return (newExp);
}


/* Returns an identifier conditional expression node of type LONG_VAR,
 * that points to the given index idx.  */
COND_EXPR *COND_EXPR::idLongIdx (int idx)
{
    COND_EXPR *newExp = new COND_EXPR(IDENTIFIER);
    newExp->expr.ident.idType = LONG_VAR;
    newExp->expr.ident.idNode.longIdx = idx;
    return (newExp);
}


/* Returns an identifier conditional expression node of type LONG_VAR */
COND_EXPR *COND_EXPR::idLong(LOCAL_ID *localId, opLoc sd, iICODE pIcode, hlFirst f, iICODE ix, operDu du, iICODE atOffset)
{
    int idx;
    COND_EXPR *newExp = new COND_EXPR(IDENTIFIER);
    /* Check for long constant and save it as a constant expression */
    if ((sd == SRC) && pIcode->ll()->testFlags(I))  /* constant */
    {
        newExp->expr.ident.idType = CONSTANT;
        if (f == HIGH_FIRST)
            newExp->expr.ident.idNode.kte.kte = (pIcode->ll()->src.op() << 16) +
                    atOffset->ll()->src.op();
        else        /* LOW_FIRST */
            newExp->expr.ident.idNode.kte.kte =
                    (atOffset->ll()->src.op() << 16)+ pIcode->ll()->src.op();
        newExp->expr.ident.idNode.kte.size = 4;
    }
    /* Save it as a long expression (reg, stack or glob) */
    else
    {
        idx = localId->newLong(sd, pIcode, f, ix, du, atOffset);
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
COND_EXPR *COND_EXPR::idOther(eReg seg, eReg regi, int16_t off)
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
    int idx;

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
 * Arguments:
 *            duIcode: icode instruction that needs the du set.
 *            du: operand is defined or used in current instruction.    */
COND_EXPR *COND_EXPR::id(const LLInst &ll_insn, opLoc sd, Function * pProc, iICODE ix_,ICODE &duIcode, operDu du)
{
    COND_EXPR *newExp;

    int idx;          /* idx into pIcode->localId table */

    const LLOperand &pm((sd == SRC) ? ll_insn.src : ll_insn.dst);

    if (    ((sd == DST) && ll_insn.testFlags(IM_DST)) or
            ((sd == SRC) && ll_insn.testFlags(IM_SRC)) or
            (sd == LHS_OP))             /* for MUL lhs */
    {                                                   /* implicit dx:ax */
        idx = pProc->localId.newLongReg (TYPE_LONG_SIGN, rDX, rAX, ix_);
        newExp = COND_EXPR::idLongIdx (idx);
        duIcode.setRegDU (rDX, du);
        duIcode.setRegDU (rAX, du);
    }

    else if ((sd == DST) && ll_insn.testFlags(IM_TMP_DST))
    {                                                   /* implicit tmp */
        newExp = COND_EXPR::idReg (rTMP, 0, &pProc->localId);
        duIcode.setRegDU(rTMP, (operDu)eUSE);
    }

    else if ((sd == SRC) && ll_insn.testFlags(I)) /* constant */
        newExp = COND_EXPR::idKte (ll_insn.src.op(), 2);
    else if (pm.regi == rUNDEF) /* global variable */
        newExp = GlobalVariable::Create(pm.segValue, pm.off);
    else if ( pm.isReg() )      /* register */
    {
        newExp = COND_EXPR::idReg (pm.regi, (sd == SRC) ? ll_insn.getFlag() :
                                                          ll_insn.getFlag() & NO_SRC_B,
                                   &pProc->localId);
        duIcode.setRegDU( pm.regi, du);
    }

    else if (pm.off)                                   /* offset */
    {
        if ((pm.seg == rSS) && (pm.regi == INDEX_BP)) /* idx on bp */
        {
            if (pm.off >= 0)                           /* argument */
                newExp = COND_EXPR::idParam (pm.off, &pProc->args);
            else                                        /* local variable */
                newExp = COND_EXPR::idLoc (pm.off, &pProc->localId);
        }
        else if ((pm.seg == rDS) && (pm.regi == INDEX_BX)) /* bx */
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
        if ((pm.seg == rDS) && (pm.regi > INDEX_BP_DI)) /* dereference */
        {
            switch (pm.regi) {
            case INDEX_SI:
                newExp = COND_EXPR::idReg(rSI, 0, &pProc->localId);
                duIcode.setRegDU( rSI, du);
                break;
            case INDEX_DI:
                newExp = COND_EXPR::idReg(rDI, 0, &pProc->localId);
                duIcode.setRegDU( rDI, du);
                break;
            case INDEX_BP:
                newExp = COND_EXPR::idReg(rBP, 0, &pProc->localId);
                break;
            case INDEX_BX:
                newExp = COND_EXPR::idReg(rBX, 0, &pProc->localId);
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
    LLOperand &pm((sd == SRC) ? ll()->src : ll()->dst);

    if ((sd == SRC) && ll()->testFlags(I))
        return (CONSTANT);
    else if (pm.regi == 0)
        return (GLOB_VAR);
    else if ( pm.isReg() )
        return (REGISTER);
    else if ((pm.seg == rSS) && (pm.regi == INDEX_BP))
    {
        //TODO: which pm.seg/pm.regi pairs should produce PARAM/LOCAL_VAR ?
        if (pm.off >= 0)
            return (PARAM);
        else
            return (LOCAL_VAR);
    }
    else
        return (OTHER);
}


/* Size of hl types */
int hlSize[] = {2, 1, 1, 2, 2, 4, 4, 4, 2, 2, 1, 4, 4};


/* Returns the type of the expression */
int hlTypeSize (const COND_EXPR *expr, Function * pproc)
{
    int first, second;

    if (expr == NULL)
        return (2);		/* for TYPE_UNKNOWN */

    switch (expr->type) {
    case BOOLEAN_OP:
        first = hlTypeSize (expr->lhs(), pproc);
        second = hlTypeSize (expr->rhs(), pproc);
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
            return (hlSize[pproc->args[expr->expr.ident.idNode.paramIdx].type]);
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
        first = expType (expr->lhs(), pproc);
        second = expType (expr->rhs(), pproc);
        if (first != second)
        {
            if (hlTypeSize (expr->lhs(), pproc) > hlTypeSize (expr->rhs(), pproc))
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
            return (pproc->args[expr->expr.ident.idNode.paramIdx].type);
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
void HlTypeSupport::performLongRemoval (eReg regi, LOCAL_ID *locId, COND_EXPR *tree)
{
    IDENTTYPE* ident;     	/* ptr to an identifier */
    eReg otherRegi;         /* high or low part of long register */

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
static std::string getString (int offset)
{
    ostringstream o;
    int strLen, i;

    strLen = strSize (&prog.Image[offset], '\0');
    o << '"';
    for (i = 0; i < strLen; i++)
        o<<cChar(prog.Image[offset+i]);
    o << "\"\0";
    return (o.str());
}


/* Walks the conditional expression tree and returns the result on a string */
string walkCondExpr (const COND_EXPR* expr, Function * pProc, int* numLoc)
{
    int16_t off;              /* temporal - for OTHER */
    ID* id;                 /* Pointer to local identifier table */
    //char* o;              /* Operand string pointer */
    bool needBracket;       /* Determine whether parenthesis is needed */
    BWGLB_TYPE* bwGlb;      /* Ptr to BWGLB_TYPE (global indexed var) */
    STKSYM * psym;          /* Pointer to argument in the stack */
    std::ostringstream outStr,codeOut;

    if (expr == NULL)
        return "";

    needBracket = true;
    switch (expr->type)
    {
    case BOOLEAN_OP:
        outStr << "(";
        outStr << walkCondExpr(expr->lhs(), pProc, numLoc);
        outStr << condOpSym[expr->op()];
        outStr << walkCondExpr(expr->rhs(), pProc, numLoc);
        outStr << ")";
        break;

    case NEGATION:
        if (expr->expr.unaryExp->type == IDENTIFIER)
        {
            needBracket = false;
            outStr << "!";
        }
        else
            outStr << "! (";
        outStr << walkCondExpr (expr->expr.unaryExp, pProc, numLoc);
        if (needBracket == true)
            outStr << ")";
        break;

    case ADDRESSOF:
        if (expr->expr.unaryExp->type == IDENTIFIER)
        {
            needBracket = false;
            outStr << "&";
        }
        else
            outStr << "&(";
        outStr << walkCondExpr (expr->expr.unaryExp, pProc, numLoc);
        if (needBracket == true)
            outStr << ")";
        break;

    case DEREFERENCE:
        outStr << "*";
        if (expr->expr.unaryExp->type == IDENTIFIER)
            needBracket = false;
        else
            outStr << "(";
        outStr << walkCondExpr (expr->expr.unaryExp, pProc, numLoc);
        if (needBracket == true)
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
                id->setLocalName(++(*numLoc));
                codeOut <<TypeContainer::typeName(id->type)<< " "<<id->name<<"; ";
                codeOut <<"/* "<<Machine_X86::regName(id->id.regi)<<" */\n";
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
            psym = &pProc->args[expr->expr.ident.idNode.paramIdx];
            if (psym->hasMacro)
                o << psym->macro<<"("<<psym->name<< ")";
            else
                o << psym->name;
            break;

        case GLOB_VAR_IDX:
            bwGlb = &pProc->localId.id_arr[expr->expr.ident.idNode.idxGlbIdx].id.bwGlb;
            o << (bwGlb->seg << 4) + bwGlb->off <<  "["<<Machine_X86::regName(bwGlb->regi)<<"]";
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
                id->setLocalName(++(*numLoc));
                codeOut <<TypeContainer::typeName(id->type)<< " "<<id->name<<"; ";
                codeOut <<"/* "<<Machine_X86::regName(id->id.longId.h) << ":" <<
                            Machine_X86::regName(id->id.longId.l) << " */\n";
                o << id->name;
                pProc->localId.propLongId (id->id.longId.l,id->id.longId.h, id->name.c_str());
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
            o << writeCall (expr->expr.ident.idNode.call.proc,*expr->expr.ident.idNode.call.args, pProc, numLoc);
            break;

        case OTHER:
            off = expr->expr.ident.idNode.other.off;
            o << Machine_X86::regName(expr->expr.ident.idNode.other.seg)<< "[";
            o << Machine_X86::regName(expr->expr.ident.idNode.other.regi);
            if (off < 0)
                o << "-"<< hexStr (-off);
            else if (off>0)
                o << "+"<< hexStr (off);
            o << "]";
        } /* eos */
        outStr << o.str();
        break;
    }
    cCode.appendDecl(codeOut.str());
    return outStr.str();
}


/* Makes a copy of the given expression.  Allocates newExp storage for each
 * node.  Returns the copy. */
COND_EXPR *COND_EXPR::clone() const
{
    COND_EXPR* newExp=0;        /* Expression node copy */

    switch (type)
    {
    case BOOLEAN_OP:
        newExp = new COND_EXPR(*this);
        newExp->boolExpr.lhs = lhs()->clone();
        newExp->boolExpr.rhs = rhs()->clone();
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
    boolExpr.op = newOp;
}

/* Inserts the expression exp into the tree at the location specified by the
 * register regi */
bool COND_EXPR::insertSubTreeReg (COND_EXPR *&tree, COND_EXPR *_expr, eReg regi,LOCAL_ID *locsym)
{

    if (tree == NULL)
        return false;
    COND_EXPR *temp=tree->insertSubTreeReg(_expr,regi,locsym);
    if(nullptr!=temp)
    {
        tree=temp;
        return true;
    }
    return false;
}
bool isSubRegisterOf(eReg reg,eReg parent)
{
    if ((parent < rAX) || (parent > rBX))
        return false; // only AX -> BX are coverede by subregisters
    return ((reg==subRegH(parent)) || (reg == subRegL(parent)));
}
COND_EXPR *COND_EXPR::insertSubTreeReg (COND_EXPR *_expr, eReg regi,LOCAL_ID *locsym)
{

    eReg treeReg;
    COND_EXPR *temp;

    switch (type) {
    case IDENTIFIER:
        if (expr.ident.idType == REGISTER)
        {
            treeReg = locsym->id_arr[expr.ident.idNode.regiIdx].id.regi;
            if (treeReg == regi)                        /* uint16_t reg */
            {
                return _expr;
            }
            else if(isSubRegisterOf(treeReg,regi))    /* uint16_t/uint8_t reg */
            {
                return _expr;
            }
        }
        return false;

    case BOOLEAN_OP:
        temp = lhs()->insertSubTreeReg( _expr, regi, locsym);
        if (nullptr!=temp)
        {
            boolExpr.lhs = temp;
            return this;
        }
        temp = rhs()->insertSubTreeReg( _expr, regi, locsym);
        if (nullptr!=temp)
        {
            boolExpr.rhs = temp;
            return this;
        }
        return nullptr;

    case NEGATION:
    case ADDRESSOF:
    case DEREFERENCE:
        temp = expr.unaryExp->insertSubTreeReg( _expr, regi, locsym);
        if (nullptr!=temp)
        {
            expr.unaryExp = temp;
            return this;
        }
        return nullptr;
    }

    return nullptr;
}
COND_EXPR *BinaryOperator::insertSubTreeReg(COND_EXPR *_expr, eReg regi, LOCAL_ID *locsym)
{
    COND_EXPR *r;
    r=m_lhs->insertSubTreeReg(_expr,regi,locsym);
    if(r)
    {
        m_lhs = r;
        return this;
    }
    r=m_rhs->insertSubTreeReg(_expr,regi,locsym);
    if(r)
    {
        m_rhs = r;
        return this;
    }
    return nullptr;
}

/* Inserts the expression exp into the tree at the location specified by the
 * long register index longIdx*/
bool COND_EXPR::insertSubTreeLongReg(COND_EXPR *_expr, COND_EXPR **tree, int longIdx)
{
    if (tree == NULL)
        return false;
    COND_EXPR *temp=(*tree)->insertSubTreeLongReg(_expr,longIdx);
    if(nullptr!=temp)
    {
        *tree=temp;
        return true;
    }
    return false;
}
COND_EXPR *COND_EXPR::insertSubTreeLongReg(COND_EXPR *_expr, int longIdx)
{
    COND_EXPR *temp;
    switch (type)
    {
    case IDENTIFIER:
        if (expr.ident.idNode.longIdx == longIdx)
        {
            return _expr;
        }
        return nullptr;

    case BOOLEAN_OP:
        temp = lhs()->insertSubTreeLongReg( _expr,longIdx);
        if (nullptr!=temp)
        {
            boolExpr.lhs = temp;
            return this;
        }
        temp = rhs()->insertSubTreeLongReg( _expr,longIdx);
        if (nullptr!=temp)
        {
            boolExpr.rhs = temp;
            return this;
        }
        return nullptr;

    case NEGATION:
    case ADDRESSOF:
    case DEREFERENCE:
        temp = expr.unaryExp->insertSubTreeLongReg(_expr,longIdx);
        if (nullptr!=temp)
        {
            expr.unaryExp = temp;
            return this;
        }
        return nullptr;
    }
    return nullptr;
}
COND_EXPR *BinaryOperator::insertSubTreeLongReg(COND_EXPR *_expr, int longIdx)
{
    COND_EXPR *r;
    r=m_lhs->insertSubTreeLongReg(_expr,longIdx);
    if(r)
    {
        m_lhs = r;
        return this;
    }
    r=m_rhs->insertSubTreeLongReg(_expr,longIdx);
    if(r)
    {
        m_rhs = r;
        return this;
    }
    return nullptr;
}


/* Recursively deallocates the abstract syntax tree rooted at *exp */
void COND_EXPR::release()
{
    switch (type)
    {
    case BOOLEAN_OP:
        lhs()->release();
        rhs()->release();
        break;
    case NEGATION:
    case ADDRESSOF:
    case DEREFERENCE:
        expr.unaryExp->release();
        break;
    }
    delete (this);
}
/* Makes a copy of the given expression.  Allocates newExp storage for each
 * node.  Returns the copy. */

COND_EXPR *BinaryOperator::clone()
{
    BinaryOperator* newExp=new BinaryOperator(m_op);        /* Expression node copy */
    newExp->m_lhs = m_lhs->clone();
    newExp->m_rhs = m_rhs->clone();
    return newExp;
}

COND_EXPR *BinaryOperator::inverse()
{
    static condOp invCondOp[] = {GREATER, GREATER_EQUAL, NOT_EQUAL, EQUAL,
                                 LESS_EQUAL, LESS, DUMMY,DUMMY,DUMMY,DUMMY,
                                 DUMMY, DUMMY, DUMMY, DUMMY, DUMMY, DUMMY,
                                 DUMMY, DBL_OR, DBL_AND};
    BinaryOperator *res=reinterpret_cast<BinaryOperator *>(this->clone());
    switch (m_op)
    {
    case LESS_EQUAL: case LESS: case EQUAL:
    case NOT_EQUAL: case GREATER: case GREATER_EQUAL:
        res->m_op = invCondOp[m_op];
        return res;

    case AND: case OR: case XOR: case NOT: case ADD:
    case SUB: case MUL: case DIV: case SHR: case SHL: case MOD:
        return COND_EXPR::unary (NEGATION, res);

    case DBL_AND: case DBL_OR:
        res->m_op = invCondOp[m_op];
        res->m_lhs=m_lhs->inverse ();
        res->m_rhs=m_rhs->inverse ();
        return res;
    } /* eos */
    assert(false);
    return res;

}
