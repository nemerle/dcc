/*
 * File: ast.c
 * Purpose: Support module for abstract syntax trees.
 * Date: September 1993
 * (C) Cristina Cifuentes
 */
#include "ast.h"

#include "msvc_fixes.h"
#include "types.h"
#include "bundle.h"
#include "machine_x86.h"
#include "project.h"

#include <QtCore/QTextStream>
#include <boost/range.hpp>
#include <boost/range/adaptor/filtered.hpp>
#include <boost/range/algorithm.hpp>
#include <boost/assign.hpp>
#include <stdint.h>
#include <string>
#include <sstream>
#include <iostream>
#include <cassert>

using namespace std;
using namespace boost;
using namespace boost::adaptors;

extern int     strSize (const uint8_t *, char);
extern char   *cChar(uint8_t c);

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
    if(regi==rSP)
    {
        printf("Discarding SP def&use info for now\n");
        return;
    }
    switch (du_in)
    {
    case eDEF:
        du.def.addReg(regi);
        du1.addDef(regi);
        break;
    case eUSE:
        du.use.addReg(regi);
        break;
    case USE_DEF:
        du.addDefinedAndUsed(regi);
        du1.addDef(regi);
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
            du.use = duIcode.du.def;
        else
            du.use = duIcode.du.use;
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
//COND_EXPR *COND_EXPR::boolOp(COND_EXPR *_lhs, COND_EXPR *_rhs, condOp _op)
//{
//    return BinaryOperator::Create(_op,_lhs,_rhs);
//}

/* Returns a unary conditional expression node.  This procedure should
 * only be used with the following conditional node types: NEGATION,
 * ADDRESSOF, DEREFERENCE, POST_INC, POST_DEC, PRE_INC, PRE_DEC	*/


/* Returns an identifier conditional expression node of type GLOB_VAR */
GlobalVariable::GlobalVariable(int16_t segValue, int16_t off)
{
    uint32_t adr;
    valid = true;
    ident.idType = GLOB_VAR;
    adr = opAdr(segValue, off);
    auto i=Project::get()->getSymIdxByAdd(adr);
    if ( not Project::get()->validSymIdx(i) )
    {
        printf ("Error, glob var not found in symtab\n");
        valid = false;
    }
    globIdx = i;
}

QString GlobalVariable::walkCondExpr(Function *, int *) const
{
    if(valid)
        return Project::get()->symtab[globIdx].name;
    return "INVALID GlobalVariable";
}

/* Returns an identifier conditional expression node of type LOCAL_VAR */
AstIdent *AstIdent::Loc(int off, LOCAL_ID *localId)
{
    size_t i;
    AstIdent *newExp = new AstIdent();
    newExp->ident.idType = LOCAL_VAR;
    for (i = 0; i < localId->csym(); i++)
    {
        const ID &lID(localId->id_arr[i]);
        if ((lID.id.bwId.off == off) and (lID.id.bwId.regOff == 0))
            break;
    }
    if (i == localId->csym())
        printf ("Error, cannot find local var\n");
    newExp->ident.idNode.localIdx = i;
    localId->id_arr[i].setLocalName(i);
    return (newExp);
}


/* Returns an identifier conditional expression node of type PARAM */
AstIdent *AstIdent::Param(int off, const STKFRAME * argSymtab)
{
    AstIdent *newExp;

    newExp = new AstIdent();
    newExp->ident.idType = PARAM;
    auto iter=argSymtab->findByLabel(off);
    if (iter == argSymtab->end())
        printf ("Error, cannot find argument var\n");
    newExp->ident.idNode.localIdx = distance(argSymtab->begin(),iter);
    return (newExp);
}


/* Returns an identifier conditional expression node of type GLOB_VAR_IDX.
 * This global variable is indexed by regi.     */
GlobalVariableIdx::GlobalVariableIdx (int16_t segValue, int16_t off, uint8_t regi, const LOCAL_ID *locSym)
{
    size_t i;
    ident.type(GLOB_VAR_IDX);
    for (i = 0; i < locSym->csym(); i++)
    {
        const BWGLB_TYPE &lID(locSym->id_arr[i].id.bwGlb);
        if ((lID.seg == segValue) and (lID.off == off) and (lID.regi == regi))
            break;
    }
    if (i == locSym->csym())
        printf ("Error, indexed-glob var not found in local id table\n");
    idxGlbIdx = i;
}
QString GlobalVariableIdx::walkCondExpr(Function *pProc, int *) const
{
    auto bwGlb = &pProc->localId.id_arr[idxGlbIdx].id.bwGlb;
    return QString("%1[%2]").arg((bwGlb->seg << 4) + bwGlb->off).arg(Machine_X86::regName(bwGlb->regi));
}


/* Returns an identifier conditional expression node of type LONG_VAR,
 * that points to the given index idx.  */
AstIdent *AstIdent::LongIdx (int idx)
{
    AstIdent *newExp = new AstIdent;
    newExp->ident.idType = LONG_VAR;
    newExp->ident.idNode.longIdx = idx;
    return (newExp);
}

AstIdent *AstIdent::String(uint32_t idx)
{
    AstIdent *newExp = new AstIdent;
    newExp->ident.idNode.strIdx = idx;
    newExp->ident.type(STRING);
    return newExp;
}


/* Returns an identifier conditional expression node of type LONG_VAR */
AstIdent *AstIdent::Long(LOCAL_ID *localId, opLoc sd, iICODE pIcode, hlFirst f, iICODE ix, operDu du, LLInst &atOffset)
{
    AstIdent *newExp;
    /* Check for long constant and save it as a constant expression */
    if ((sd == SRC) and pIcode->ll()->testFlags(I))  /* constant */
    {
        int value;
        if (f == HIGH_FIRST)
            value = (pIcode->ll()->src().getImm2() << 16) + atOffset.src().getImm2();
        else/* LOW_FIRST */
            value = (atOffset.src().getImm2() << 16)+ pIcode->ll()->src().getImm2();
        newExp = new Constant(value,4);
    }
    /* Save it as a long expression (reg, stack or glob) */
    else
    {
        newExp  = new AstIdent();
        newExp->ident.idType = LONG_VAR;
        newExp->ident.idNode.longIdx = localId->newLong(sd, pIcode, f, ix, du, atOffset);
    }
    return (newExp);
}

/* Returns an identifier conditional expression node of type OTHER.
 * Temporary solution, should really be encoded as an indexed type (eg.
 * arrays). */
AstIdent *AstIdent::Other(eReg seg, eReg regi, int16_t off)
{
    AstIdent *newExp  = new AstIdent();
    newExp->ident.idType = OTHER;
    newExp->ident.idNode.other.seg = seg;
    newExp->ident.idNode.other.regi = regi;
    newExp->ident.idNode.other.off = off;
    return (newExp);
}


/* Returns an identifier conditional expression node of type TYPE_LONG or
 * TYPE_WORD_SIGN	*/
AstIdent *AstIdent::idID (const ID *retVal, LOCAL_ID *locsym, iICODE ix_)
{
    int idx;
    AstIdent *newExp=nullptr;
    switch(retVal->type)
    {
    case TYPE_LONG_SIGN:
    {
        newExp  = new AstIdent();
        idx = locsym->newLongReg (TYPE_LONG_SIGN, retVal->longId(), ix_);
        newExp->ident.idType = LONG_VAR;
        newExp->ident.idNode.longIdx = idx;
        break;
    }
    case TYPE_WORD_SIGN:
        newExp = new RegisterNode(locsym->newByteWordReg(retVal->type, retVal->id.regi),WORD_REG,locsym);
        break;
    case TYPE_BYTE_SIGN:
        newExp = new RegisterNode(locsym->newByteWordReg(retVal->type, retVal->id.regi),BYTE_REG,locsym);
        break;
    default:
        fprintf(stderr,"AstIdent::idID unhandled type %d\n",retVal->type);
    }
    return (newExp);
}


/* Returns an identifier conditional expression node, according to the given
 * type.
 * Arguments:
 *            duIcode: icode instruction that needs the du set.
 *            du: operand is defined or used in current instruction.    */
Expr *AstIdent::id(const LLInst &ll_insn, opLoc sd, Function * pProc, iICODE ix_,ICODE &duIcode, operDu du)
{
    Expr *newExp;

    int idx;          /* idx into pIcode->localId table */

    const LLOperand &pm(*ll_insn.get(sd));

    if (    ((sd == DST) and ll_insn.testFlags(IM_DST)) or
            ((sd == SRC) and ll_insn.testFlags(IM_SRC)) or
            (sd == LHS_OP))             /* for MUL lhs */
    {                                                   /* implicit dx:ax */
        idx = pProc->localId.newLongReg (TYPE_LONG_SIGN, LONGID_TYPE(rDX, rAX), ix_);
        newExp = AstIdent::LongIdx (idx);
        duIcode.setRegDU (rDX, du);
        duIcode.setRegDU (rAX, du);
    }

    else if ((sd == DST) and ll_insn.testFlags(IM_TMP_DST))
    {                                                   /* implicit tmp */
        newExp = new RegisterNode(LLOperand(rTMP,2), &pProc->localId);
        duIcode.setRegDU(rTMP, (operDu)eUSE);
    }

    else if ((sd == SRC) and ll_insn.testFlags(I)) /* constant */
        newExp = new Constant(ll_insn.src().getImm2(), 2);
    else if (pm.regi == rUNDEF) /* global variable */
        newExp = new GlobalVariable(pm.segValue, pm.off);
    else if ( pm.isReg() )      /* register */
    {
        //(sd == SRC) ? ll_insn.getFlag() : ll_insn.getFlag() & NO_SRC_B
        newExp = new RegisterNode(pm, &pProc->localId);
        duIcode.setRegDU( pm.regi, du);
    }

    else if (pm.off)                                   /* offset */
    { // TODO: this is ABI specific, should be actually based on Function calling conv
        if ((pm.seg == rSS) and (pm.regi == INDEX_BP)) /* idx on bp */
        {
            if (pm.off >= 0)                           /* argument */
                newExp = AstIdent::Param (pm.off, &pProc->args);
            else                                        /* local variable */
                newExp = AstIdent::Loc (pm.off, &pProc->localId);
        }
        else if ((pm.seg == rDS) and (pm.regi == INDEX_BX)) /* bx */
        {
            if (pm.off > 0)        /* global variable */
                newExp = new GlobalVariableIdx(pm.segValue, pm.off, rBX,&pProc->localId);
            else
                newExp = AstIdent::Other (pm.seg, pm.regi, pm.off);
            duIcode.setRegDU( rBX, eUSE);
        }
        else                                            /* idx <> bp, bx */
            newExp = AstIdent::Other (pm.seg, pm.regi, pm.off);
        /**** check long ops, indexed global var *****/
    }

    else  /* (pm->regi >= INDEXBASE and pm->off = 0) => indexed and no off */
    {
        if ((pm.seg == rDS) and (pm.regi > INDEX_BP_DI)) /* dereference */
        {
            eReg selected;
            switch (pm.regi) {
            case INDEX_SI: selected = rSI; break;
            case INDEX_DI: selected = rDI; break;
            case INDEX_BP: selected = rBP; break;
            case INDEX_BX: selected = rBX; break;
            default:
                newExp = nullptr;
                assert(false);
            }
            //NOTICE: was selected, 0
            newExp = new RegisterNode(LLOperand(selected, 0), &pProc->localId);
            duIcode.setRegDU( selected, du);
            newExp = UnaryOperator::Create(DEREFERENCE, newExp);
        }
        else
            newExp = AstIdent::Other (pm.seg, pm.regi, 0);
    }
    return (newExp);
}


/* Returns the identifier type */
condId LLInst::idType(opLoc sd) const
{
    const LLOperand &pm((sd == SRC) ? src() : m_dst);

    if ((sd == SRC) and testFlags(I))
        return (CONSTANT);
    else if (pm.regi == 0)
        return (GLOB_VAR);
    else if ( pm.isReg() )
        return (REGISTER);
    else if ((pm.seg == rSS) and (pm.regi == INDEX_BP))
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

int Expr::hlTypeSize(Function * pproc) const
{
    if (this == nullptr)
        return (2);		/* for TYPE_UNKNOWN */
    fprintf(stderr,"hlTypeSize queried for Unkown type %d \n",m_type);
    return 2;			// CC: is this correct?
}

/* Returns the type of the expression */
int BinaryOperator::hlTypeSize(Function * pproc) const
{
    return std::max(lhs()->hlTypeSize (pproc),rhs()->hlTypeSize (pproc));
}
int UnaryOperator::hlTypeSize(Function *pproc) const
{
    return (unaryExp->hlTypeSize (pproc));
}
int GlobalVariable::hlTypeSize(Function *pproc) const
{
    return (Project::get()->symbolSize(globIdx));
}
int GlobalVariableIdx::hlTypeSize(Function *pproc) const
{
    return (hlSize[pproc->localId.id_arr[idxGlbIdx].type]);
}

int AstIdent::hlTypeSize(Function *pproc) const
{
    switch (ident.idType)
    {
    case GLOB_VAR:
        assert(false);
        return 1;
    case LOCAL_VAR:
        return (hlSize[pproc->localId.id_arr[ident.idNode.localIdx].type]);
    case PARAM:
        return (hlSize[pproc->args[ident.idNode.paramIdx].type]);
    case STRING:
        return (2);
    case LONG_VAR:
        return (4);
    case OTHER:
        return (2);
    default:
        assert(false);
        return -1;
    } /* eos */
}
hlType BinaryOperator::expType(Function *pproc) const
{
    hlType first = lhs()->expType ( pproc );
    hlType second = rhs()->expType ( pproc );
    if (first != second)
    {
        if (lhs()->hlTypeSize(pproc) > rhs()->hlTypeSize (pproc))
            return (first);
        else
            return (second);
    }
    else
        return (first);
}
hlType UnaryOperator::expType(Function *pproc) const
{
    return unaryExp->expType (pproc);
}
hlType GlobalVariable::expType(Function *pproc) const
{
    return Project::get()->symbolType(globIdx);
}
hlType GlobalVariableIdx::expType(Function *pproc) const
{
    return (pproc->localId.id_arr[idxGlbIdx].type);
}

hlType AstIdent::expType(Function *pproc) const
{
    switch (ident.idType)
    {
    case UNDEF:
    case CONSTANT:
    case FUNCTION:
    case REGISTER:
    case GLOB_VAR:
    case GLOB_VAR_IDX:
        assert(false);
        return TYPE_UNKNOWN;
    case LOCAL_VAR:
        return (pproc->localId.id_arr[ident.idNode.localIdx].type);
    case PARAM:
        return (pproc->args[ident.idNode.paramIdx].type);
    case STRING:
        return (TYPE_STR);
    case LONG_VAR:
        return (pproc->localId.id_arr[ident.idNode.longIdx].type);
    default:
        return (TYPE_UNKNOWN);
    } /* eos */
    return (TYPE_UNKNOWN);
}
/* Returns the type of the expression */


/* Removes the register from the tree.  If the register was part of a long
 * register (eg. dx:ax), the node gets transformed into an integer register
 * node.        */
Expr * HlTypeSupport::performLongRemoval (eReg regi, LOCAL_ID *locId, Expr *tree)
{
    switch (tree->m_type) {
    case BOOLEAN_OP:
    case POST_INC: case POST_DEC:
    case PRE_INC: case PRE_DEC:
    case NEGATION: case ADDRESSOF:
    case DEREFERENCE:
    case IDENTIFIER:
        return tree->performLongRemoval(regi,locId);
        break;
    default:
        fprintf(stderr,"performLongRemoval attemped on %d\n",tree->m_type);
        break;
    }
    return tree;
}

/* Returns the string located in image, formatted in C format. */
static QString getString (int offset)
{
    PROG &prog(Project::get()->prog);
    QString o;
    int strLen, i;

    strLen = strSize (&prog.image()[offset], '\0');
    o += '"';
    for (i = 0; i < strLen; i++)
        o += cChar(prog.image()[offset+i]);
    o += "\"\0";
    return o;
}
QString BinaryOperator::walkCondExpr(Function * pProc, int* numLoc) const
{
    assert(rhs());

    return QString("(%1%2%3)")
            .arg((m_op!=NOT) ? lhs()->walkCondExpr(pProc, numLoc) : "")
            .arg(condOpSym[m_op])
            .arg(rhs()->walkCondExpr(pProc, numLoc));
}
QString AstIdent::walkCondExpr(Function *pProc, int *numLoc) const
{
    int16_t off;              /* temporal - for OTHER */
    ID* id;                 /* Pointer to local identifier table */
    BWGLB_TYPE* bwGlb;      /* Ptr to BWGLB_TYPE (global indexed var) */
    STKSYM * psym;          /* Pointer to argument in the stack */
    QString codeContents;
    QString collectedContents;

    QTextStream codeOut(&codeContents);
    QTextStream o(&collectedContents);

    switch (ident.idType)
    {
    case LOCAL_VAR:
        o << pProc->localId.id_arr[ident.idNode.localIdx].name;
        break;

    case PARAM:
        psym = &pProc->args[ident.idNode.paramIdx];
        if (psym->hasMacro)
            o << psym->macro<<"("<<psym->name<< ")";
        else
            o << psym->name;
        break;
    case STRING:
        o << getString (ident.idNode.strIdx);
        break;

    case LONG_VAR:
        id = &pProc->localId.id_arr[ident.idNode.longIdx];
        if (id->name[0] != '\0') /* STK_FRAME & REG w/name*/
            o << id->name;
        else if (id->loc == REG_FRAME)
        {
            id->setLocalName(++(*numLoc));
            codeOut <<TypeContainer::typeName(id->type)<< " "<<id->name<<"; ";
            codeOut <<"/* "<<Machine_X86::regName(id->longId().h()) << ":" <<
                      Machine_X86::regName(id->longId().l()) << " */\n";
            o << id->name;
            pProc->localId.propLongId (id->longId().l(),id->longId().h(), id->name);
        }
        else    /* GLB_FRAME */
        {
            if (id->id.longGlb.regi == 0)  /* not indexed */
                o << "[" << (id->id.longGlb.seg<<4) + id->id.longGlb.offH <<"]";
            else if (id->id.longGlb.regi == rBX)
                o << "[" << (id->id.longGlb.seg<<4) + id->id.longGlb.offH <<"][bx]";
        }
        break;
    case OTHER:
        off = ident.idNode.other.off;
        o << Machine_X86::regName(ident.idNode.other.seg)<< "[";
        o << Machine_X86::regName(ident.idNode.other.regi);
        if (off < 0)
            o << "-"<< hexStr (-off);
        else if (off>0)
            o << "+"<< hexStr (off);
        o << "]";
        break;
    default:
        assert(false);
        return "";


    } /* eos */
    cCode.appendDecl(codeContents);
    return collectedContents;

}
QString UnaryOperator::wrapUnary(Function *pProc, int *numLoc,QChar op) const
{
    QString outStr = op;
    QString inner = unaryExp->walkCondExpr (pProc, numLoc);
    if (unaryExp->m_type == IDENTIFIER)
        outStr += inner;
    else
        outStr += "("+inner+')';
    return outStr;
}

QString UnaryOperator::walkCondExpr(Function *pProc, int *numLoc) const
{
    QString outStr;
    switch(m_type)
    {
    case NEGATION:
        outStr+=wrapUnary(pProc,numLoc,'!');
        break;

    case ADDRESSOF:
        outStr+=wrapUnary(pProc,numLoc,'&');
        break;

    case DEREFERENCE:
        outStr+=wrapUnary(pProc,numLoc,'*');
        break;

    case POST_INC:
        outStr +=  unaryExp->walkCondExpr (pProc, numLoc) + "++";
        break;

    case POST_DEC:
        outStr +=  unaryExp->walkCondExpr (pProc, numLoc) + "--";
        break;

    case PRE_INC:
        outStr += "++" + unaryExp->walkCondExpr (pProc, numLoc);
        break;

    case PRE_DEC:
        outStr += "--" + unaryExp->walkCondExpr (pProc, numLoc);
        break;
    }
    return outStr;
}

/* Walks the conditional expression tree and returns the result on a string */





/* Changes the boolean conditional operator at the root of this expression */
void BinaryOperator::changeBoolOp (condOp newOp)
{
    m_op = newOp;
}
bool Expr::insertSubTreeReg (AstIdent *&tree, Expr *_expr, eReg regi,const LOCAL_ID *locsym)
{
    Expr *nd = tree;
    bool res=insertSubTreeReg (nd, _expr, regi,locsym);
    if(res)
    {
        tree= dynamic_cast<AstIdent *>(nd);
        return true;
        assert(tree);
    }
    return false;
}
/* Inserts the expression exp into the tree at the location specified by the
 * register regi */
bool Expr::insertSubTreeReg (Expr *&tree, Expr *_expr, eReg regi,const LOCAL_ID *locsym)
{

    if (tree == nullptr)
        return false;
    Expr *temp=tree->insertSubTreeReg(_expr,regi,locsym);
    if(nullptr!=temp)
    {
        tree=temp;
        return true;
    }
    return false;
}

Expr *UnaryOperator::insertSubTreeReg(Expr *_expr, eReg regi, const LOCAL_ID *locsym)
{
    Expr *temp;

    switch (m_type) {
    case NEGATION:
    case ADDRESSOF:
    case DEREFERENCE:
        temp = unaryExp->insertSubTreeReg( _expr, regi, locsym);
        if (nullptr!=temp)
        {
            unaryExp = temp;
            return this;
        }
        return nullptr;
    default:
        fprintf(stderr,"insertSubTreeReg attempt on unhandled type %d\n",m_type);
    }
    return nullptr;
}
Expr *BinaryOperator::insertSubTreeReg(Expr *_expr, eReg regi, const LOCAL_ID *locsym)
{
    Expr *r;
    if(this->op()!=NOT)
    {
        assert(m_lhs);
        r=m_lhs->insertSubTreeReg(_expr,regi,locsym);
        if(r)
        {
            m_lhs = r;
            return this;
        }
    }
    assert(m_rhs);
    r=m_rhs->insertSubTreeReg(_expr,regi,locsym);
    if(r)
    {
        m_rhs = r;
        return this;
    }
    return nullptr;
}
Expr *AstIdent::insertSubTreeReg(Expr *_expr, eReg regi, const LOCAL_ID *locsym)
{
    if (ident.idType == REGISTER)
    {
        assert(false);
    }
    return nullptr;
}
/* Inserts the expression exp into the tree at the location specified by the
 * long register index longIdx*/
bool Expr::insertSubTreeLongReg(Expr *_expr, Expr *&tree, int longIdx)
{
    if (tree == nullptr)
        return false;
    Expr *temp=tree->insertSubTreeLongReg(_expr,longIdx);
    if(nullptr!=temp)
    {
        tree=temp;
        return true;
    }
    return false;
}
Expr *UnaryOperator::insertSubTreeLongReg(Expr *_expr, int longIdx)
{
    Expr *temp = unaryExp->insertSubTreeLongReg(_expr,longIdx);
    if (nullptr!=temp)
    {
        unaryExp = temp;
        return this;
    }
    return nullptr;
}
Expr *BinaryOperator::insertSubTreeLongReg(Expr *_expr, int longIdx)
{
    Expr *r;
    if(m_op!=NOT)
    {
        r=m_lhs->insertSubTreeLongReg(_expr,longIdx);
        if(r)
        {
            m_lhs = r;
            return this;
        }
    }
    r=m_rhs->insertSubTreeLongReg(_expr,longIdx);
    if(r)
    {
        m_rhs = r;
        return this;
    }
    return nullptr;
}
Expr *AstIdent::insertSubTreeLongReg(Expr *_expr, int longIdx)
{
    if (ident.idNode.longIdx == longIdx)
    {
        return _expr;
    }
    return nullptr;
}


/* Makes a copy of the given expression.  Allocates newExp storage for each
 * node.  Returns the copy. */

Expr *BinaryOperator::clone() const
{
    /* Expression node copy */
    return new BinaryOperator(m_op,m_lhs->clone(),m_rhs->clone());
}

Expr *BinaryOperator::inverse() const
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
        return UnaryOperator::Create(NEGATION, res);

    case DBL_AND: case DBL_OR:
        res->m_op = invCondOp[m_op];
        res->m_lhs=m_lhs->inverse ();
        res->m_rhs=m_rhs->inverse ();
        return res;
    default:
        fprintf(stderr,"BinaryOperator::inverse attempt on unhandled op %d\n",m_op);
    } /* eos */
    assert(false);
    return res;

}
Expr *AstIdent::performLongRemoval(eReg regi, LOCAL_ID *locId)
{
    eReg otherRegi;         /* high or low part of long register */

    if (ident.idType == LONG_VAR)
    {
        otherRegi = otherLongRegi (regi, ident.idNode.longIdx, locId);
        delete this;
        return new RegisterNode(locId->newByteWordReg(TYPE_WORD_SIGN,otherRegi),WORD_REG,locId);
    }
    return this;
}
eReg AstIdent::otherLongRegi (eReg regi, int idx, LOCAL_ID *locTbl)
{
    ID *id = &locTbl->id_arr[idx];
    if ((id->loc == REG_FRAME) and ((id->type == TYPE_LONG_SIGN) or
                                    (id->type == TYPE_LONG_UNSIGN)))
    {
        if (id->longId().h() == regi)
            return (id->longId().l());
        else if (id->longId().l() == regi)
            return (id->longId().h());
    }
    return rUNDEF;	// Cristina: please check this!
}


QString Constant::walkCondExpr(Function *, int *) const
{
    if (kte.kte < 1000)
        return  QString::number(kte.kte);
    else
        return "0x" + QString::number(kte.kte,16);
}

int Constant::hlTypeSize(Function *) const
{
    return kte.size;
}

hlType Constant::expType(Function *pproc) const
{
    return TYPE_CONST;
}

QString FuncNode::walkCondExpr(Function *pProc, int *numLoc) const
{
    return pProc->writeCall(call.proc,*call.args, numLoc);
}

int FuncNode::hlTypeSize(Function *) const
{
    return hlSize[call.proc->retVal.type];
}

hlType FuncNode::expType(Function *) const
{
    return call.proc->retVal.type;
}
