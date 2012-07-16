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
#include "project.h"
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
            du.def.addReg(regi);
            du1.numRegsDef++;
            break;
        case eUSE:
            du.use.addReg(regi);
            break;
        case USE_DEF:
            du.addDefinedAndUsed(regi);
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
//COND_EXPR *COND_EXPR::boolOp(COND_EXPR *_lhs, COND_EXPR *_rhs, condOp _op)
//{
//    return BinaryOperator::Create(_op,_lhs,_rhs);
//}

/* Returns a unary conditional expression node.  This procedure should
 * only be used with the following conditional node types: NEGATION,
 * ADDRESSOF, DEREFERENCE, POST_INC, POST_DEC, PRE_INC, PRE_DEC	*/


/* Returns an identifier conditional expression node of type GLOB_VAR */
AstIdent *GlobalVariable::Create(int16_t segValue, int16_t off)
{
    AstIdent *newExp;
    uint32_t adr;

    newExp = new AstIdent();
    newExp->ident.idType = GLOB_VAR;
    adr = opAdr(segValue, off);
    auto i=Project::get()->getSymIdxByAdd(adr);
    if ( not Project::get()->validSymIdx(i) )
    {
        printf ("Error, glob var not found in symtab\n");
        delete newExp;
        return 0;
    }
    newExp->ident.idNode.globIdx = i;
    return (newExp);
}


/* Returns an identifier conditional expression node of type REGISTER */
AstIdent *AstIdent::Reg(eReg regi, uint32_t icodeFlg, LOCAL_ID *locsym)
{
    AstIdent *newExp;

    newExp = new AstIdent();
    newExp->ident.idType = REGISTER;
    hlType type_sel;
    regType reg_type;
    if ((icodeFlg & B) || (icodeFlg & SRC_B))
    {
        type_sel = TYPE_BYTE_SIGN;
        reg_type = BYTE_REG;
    }
    else    /* uint16_t */
    {
        type_sel = TYPE_WORD_SIGN;
        reg_type = WORD_REG;
    }
    newExp->ident.idNode.regiIdx = locsym->newByteWordReg(type_sel, regi);
    newExp->ident.regiType = reg_type;
    return (newExp);
}


/* Returns an identifier conditional expression node of type REGISTER */
AstIdent *AstIdent::RegIdx(int idx, regType reg_type)
{
    AstIdent *newExp;

    newExp = new AstIdent();
    newExp->ident.idType = REGISTER;
    newExp->ident.regiType = reg_type;
    newExp->ident.idNode.regiIdx = idx;
    return (newExp);
}

/* Returns an identifier conditional expression node of type LOCAL_VAR */
AstIdent *AstIdent::Loc(int off, LOCAL_ID *localId)
{
    size_t i;
    AstIdent *newExp;

    newExp = new AstIdent();
    newExp->ident.idType = LOCAL_VAR;
    for (i = 0; i < localId->csym(); i++)
    {
        const ID &lID(localId->id_arr[i]);
        if ((lID.id.bwId.off == off) && (lID.id.bwId.regOff == 0))
            break;
    }
    if (i == localId->csym())
        printf ("Error, cannot find local var\n");
    newExp->ident.idNode.localIdx = i;
    localId->id_arr[i].setLocalName(i);
    return (newExp);
}


/* Returns an identifier conditional expression node of type PARAM */
AstIdent *AstIdent::idParam(int off, const STKFRAME * argSymtab)
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
AstIdent *idCondExpIdxGlob (int16_t segValue, int16_t off, uint8_t regi, const LOCAL_ID *locSym)
{
    size_t i;
    AstIdent *newExp  = new AstIdent();
    newExp->ident.idType = GLOB_VAR_IDX;
    for (i = 0; i < locSym->csym(); i++)
    {
        const BWGLB_TYPE &lID(locSym->id_arr[i].id.bwGlb);
        if ((lID.seg == segValue) && (lID.off == off) && (lID.regi == regi))
            break;
    }
    if (i == locSym->csym())
        printf ("Error, indexed-glob var not found in local id table\n");
    newExp->ident.idNode.idxGlbIdx = i;
    return (newExp);
}


/* Returns an identifier conditional expression node of type CONSTANT */
AstIdent *AstIdent::Kte(uint32_t kte, uint8_t size)
{
    AstIdent *newExp = new AstIdent();
    newExp->ident.idType = CONSTANT;
    newExp->ident.idNode.kte.kte = kte;
    newExp->ident.idNode.kte.size = size;
    return (newExp);
}


/* Returns an identifier conditional expression node of type LONG_VAR,
 * that points to the given index idx.  */
AstIdent *AstIdent::LongIdx (int idx)
{
    AstIdent *newExp = new AstIdent();
    newExp->ident.idType = LONG_VAR;
    newExp->ident.idNode.longIdx = idx;
    return (newExp);
}


/* Returns an identifier conditional expression node of type LONG_VAR */
AstIdent *AstIdent::idLong(LOCAL_ID *localId, opLoc sd, iICODE pIcode, hlFirst f, iICODE ix, operDu du, LLInst &atOffset)
{
    int idx;
    AstIdent *newExp  = new AstIdent();
    /* Check for long constant and save it as a constant expression */
    if ((sd == SRC) && pIcode->ll()->testFlags(I))  /* constant */
    {
        newExp->ident.idType = CONSTANT;
        int value;
        if (f == HIGH_FIRST)
            value = (pIcode->ll()->src().getImm2() << 16) + atOffset.src().getImm2();
        else/* LOW_FIRST */
            value = (atOffset.src().getImm2() << 16)+ pIcode->ll()->src().getImm2();
        newExp->ident.idNode.kte.kte = value;
        newExp->ident.idNode.kte.size = 4;
    }
    /* Save it as a long expression (reg, stack or glob) */
    else
    {
        idx = localId->newLong(sd, pIcode, f, ix, du, atOffset);
        newExp->ident.idType = LONG_VAR;
        newExp->ident.idNode.longIdx = idx;
    }
    return (newExp);
}


/* Returns an identifier conditional expression node of type FUNCTION */
AstIdent *AstIdent::idFunc(Function * pproc, STKFRAME * args)
{
    AstIdent *newExp  = new AstIdent();

    newExp->ident.idType = FUNCTION;
    newExp->ident.idNode.call.proc = pproc;
    newExp->ident.idNode.call.args = args;
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
    AstIdent *newExp  = new AstIdent();
    int idx;

    if (retVal->type == TYPE_LONG_SIGN)
    {
        idx = locsym->newLongReg (TYPE_LONG_SIGN, retVal->id.longId.h,retVal->id.longId.l, ix_);
        newExp->ident.idType = LONG_VAR;
        newExp->ident.idNode.longIdx = idx;
    }
    else if (retVal->type == TYPE_WORD_SIGN)
    {
        newExp->ident.idType = REGISTER;
        newExp->ident.idNode.regiIdx = locsym->newByteWordReg(TYPE_WORD_SIGN, retVal->id.regi);
        newExp->ident.regiType = WORD_REG;
    }
    return (newExp);
}


/* Returns an identifier conditional expression node, according to the given
 * type.
 * Arguments:
 *            duIcode: icode instruction that needs the du set.
 *            du: operand is defined or used in current instruction.    */
COND_EXPR *AstIdent::id(const LLInst &ll_insn, opLoc sd, Function * pProc, iICODE ix_,ICODE &duIcode, operDu du)
{
    COND_EXPR *newExp;

    int idx;          /* idx into pIcode->localId table */

    const LLOperand &pm((sd == SRC) ? ll_insn.src() : ll_insn.dst);

    if (    ((sd == DST) && ll_insn.testFlags(IM_DST)) or
            ((sd == SRC) && ll_insn.testFlags(IM_SRC)) or
            (sd == LHS_OP))             /* for MUL lhs */
    {                                                   /* implicit dx:ax */
        idx = pProc->localId.newLongReg (TYPE_LONG_SIGN, rDX, rAX, ix_);
        newExp = AstIdent::LongIdx (idx);
        duIcode.setRegDU (rDX, du);
        duIcode.setRegDU (rAX, du);
    }

    else if ((sd == DST) && ll_insn.testFlags(IM_TMP_DST))
    {                                                   /* implicit tmp */
        newExp = AstIdent::Reg (rTMP, 0, &pProc->localId);
        duIcode.setRegDU(rTMP, (operDu)eUSE);
    }

    else if ((sd == SRC) && ll_insn.testFlags(I)) /* constant */
        newExp = AstIdent::Kte (ll_insn.src().getImm2(), 2);
    else if (pm.regi == rUNDEF) /* global variable */
        newExp = GlobalVariable::Create(pm.segValue, pm.off);
    else if ( pm.isReg() )      /* register */
    {
        newExp = AstIdent::Reg (pm.regi, (sd == SRC) ? ll_insn.getFlag() :
                                                       ll_insn.getFlag() & NO_SRC_B,
                                &pProc->localId);
        duIcode.setRegDU( pm.regi, du);
    }

    else if (pm.off)                                   /* offset */
    {
        if ((pm.seg == rSS) && (pm.regi == INDEX_BP)) /* idx on bp */
        {
            if (pm.off >= 0)                           /* argument */
                newExp = AstIdent::idParam (pm.off, &pProc->args);
            else                                        /* local variable */
                newExp = AstIdent::Loc (pm.off, &pProc->localId);
        }
        else if ((pm.seg == rDS) && (pm.regi == INDEX_BX)) /* bx */
        {
            if (pm.off > 0)        /* global variable */
                newExp = idCondExpIdxGlob (pm.segValue, pm.off, rBX,&pProc->localId);
            else
                newExp = AstIdent::Other (pm.seg, pm.regi, pm.off);
            duIcode.setRegDU( rBX, eUSE);
        }
        else                                            /* idx <> bp, bx */
            newExp = AstIdent::Other (pm.seg, pm.regi, pm.off);
        /**** check long ops, indexed global var *****/
    }

    else  /* (pm->regi >= INDEXBASE && pm->off = 0) => indexed && no off */
    {
        if ((pm.seg == rDS) && (pm.regi > INDEX_BP_DI)) /* dereference */
        {
            switch (pm.regi) {
                case INDEX_SI:
                    newExp = AstIdent::Reg(rSI, 0, &pProc->localId);
                    duIcode.setRegDU( rSI, du);
                    break;
                case INDEX_DI:
                    newExp = AstIdent::Reg(rDI, 0, &pProc->localId);
                    duIcode.setRegDU( rDI, du);
                    break;
                case INDEX_BP:
                    newExp = AstIdent::Reg(rBP, 0, &pProc->localId);
                    break;
                case INDEX_BX:
                    newExp = AstIdent::Reg(rBX, 0, &pProc->localId);
                    duIcode.setRegDU( rBX, du);
                    break;
                default:
                    newExp = 0;
                    assert(false);
            }
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
    const LLOperand &pm((sd == SRC) ? src() : dst);

    if ((sd == SRC) && testFlags(I))
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

int COND_EXPR::hlTypeSize(Function * pproc) const
{
    if (this == NULL)
        return (2);		/* for TYPE_UNKNOWN */

    switch (m_type) {
        case BOOLEAN_OP:
            assert(false);
            return 0;
            //            return expr->hlTypeSize(pproc);
        case NEGATION:	case ADDRESSOF:
        case POST_INC:	case POST_DEC:
        case PRE_INC:		case PRE_DEC:
        case DEREFERENCE:
            assert(false);
            return 0;
            //return expr->hlTypeSize(pproc);
        case IDENTIFIER:
            assert(false);
            return 0;
        default:
            fprintf(stderr,"hlTypeSize queried for Unkown type %d \n",m_type);
            break;
    }
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
int AstIdent::hlTypeSize(Function *pproc) const
{
    switch (ident.idType)
    {
        case GLOB_VAR:
            return (Project::get()->symbolSize(ident.idNode.globIdx));
        case REGISTER:
            if (ident.regiType == BYTE_REG)
                return (1);
            else
                return (2);
        case LOCAL_VAR:
            return (hlSize[pproc->localId.id_arr[ident.idNode.localIdx].type]);
        case PARAM:
            return (hlSize[pproc->args[ident.idNode.paramIdx].type]);
        case GLOB_VAR_IDX:
            return (hlSize[pproc->localId.id_arr[ident.idNode.idxGlbIdx].type]);
        case CONSTANT:
            return (ident.idNode.kte.size);
        case STRING:
            return (2);
        case LONG_VAR:
            return (4);
        case FUNCTION:
            return (hlSize[ident.idNode.call.proc->retVal.type]);
        case OTHER:
            return (2);
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
hlType AstIdent::expType(Function *pproc) const
{
    switch (ident.idType)
    {
        case GLOB_VAR:
            return Project::get()->symbolType(ident.idNode.globIdx);
        case REGISTER:
            if (ident.regiType == BYTE_REG)
                return (TYPE_BYTE_SIGN);
            else
                return (TYPE_WORD_SIGN);
        case LOCAL_VAR:
            return (pproc->localId.id_arr[ident.idNode.localIdx].type);
        case PARAM:
            return (pproc->args[ident.idNode.paramIdx].type);
        case GLOB_VAR_IDX:
            return (pproc->localId.id_arr[ident.idNode.idxGlbIdx].type);
        case CONSTANT:
            return (TYPE_CONST);
        case STRING:
            return (TYPE_STR);
        case LONG_VAR:
            return (pproc->localId.id_arr[ident.idNode.longIdx].type);
        case FUNCTION:
            return (ident.idNode.call.proc->retVal.type);
        default:
            return (TYPE_UNKNOWN);
    } /* eos */
    return (TYPE_UNKNOWN);
}
/* Returns the type of the expression */
hlType COND_EXPR::expType(Function * pproc) const
{

    if (this == nullptr)
        return (TYPE_UNKNOWN);

    switch (m_type)
    {
        case BOOLEAN_OP:
            assert(false);
            return TYPE_UNKNOWN;
        case POST_INC: case POST_DEC:
        case PRE_INC:  case PRE_DEC:
        case NEGATION:
            assert(false);
            return TYPE_UNKNOWN;
        case ADDRESSOF:	return (TYPE_PTR);		/***????****/
        case DEREFERENCE:	return (TYPE_PTR);
        case IDENTIFIER:
            assert(false);
            return TYPE_UNKNOWN;
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
    switch (tree->m_type) {
        case BOOLEAN_OP:
        case POST_INC: case POST_DEC:
        case PRE_INC: case PRE_DEC:
        case NEGATION: case ADDRESSOF:
        case DEREFERENCE:
        case IDENTIFIER:
            tree->performLongRemoval(regi,locId);
            break;
        default:
            fprintf(stderr,"performLongRemoval attemped on %d\n",tree->m_type);
            break;
    }
}

/* Returns the string located in image, formatted in C format. */
static std::string getString (int offset)
{
    PROG &prog(Project::get()->prog);
    ostringstream o;
    int strLen, i;

    strLen = strSize (&prog.Image[offset], '\0');
    o << '"';
    for (i = 0; i < strLen; i++)
        o<<cChar(prog.Image[offset+i]);
    o << "\"\0";
    return (o.str());
}
string BinaryOperator::walkCondExpr(Function * pProc, int* numLoc) const
{
    std::ostringstream outStr;
    outStr << "(";
    outStr << lhs()->walkCondExpr(pProc, numLoc);
    outStr << condOpSym[m_op];
    outStr << rhs()->walkCondExpr(pProc, numLoc);
    outStr << ")";
    return outStr.str();
}
string AstIdent::walkCondExpr(Function *pProc, int *numLoc) const
{
    int16_t off;              /* temporal - for OTHER */
    ID* id;                 /* Pointer to local identifier table */
    BWGLB_TYPE* bwGlb;      /* Ptr to BWGLB_TYPE (global indexed var) */
    STKSYM * psym;          /* Pointer to argument in the stack */
    std::ostringstream outStr,codeOut;

    std::ostringstream o;
    switch (ident.idType)
    {
        case GLOB_VAR:
            o << Project::get()->symtab[ident.idNode.globIdx].name;
            break;
        case REGISTER:
            id = &pProc->localId.id_arr[ident.idNode.regiIdx];
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
            o << pProc->localId.id_arr[ident.idNode.localIdx].name;
            break;

        case PARAM:
            psym = &pProc->args[ident.idNode.paramIdx];
            if (psym->hasMacro)
                o << psym->macro<<"("<<psym->name<< ")";
            else
                o << psym->name;
            break;

        case GLOB_VAR_IDX:
            bwGlb = &pProc->localId.id_arr[ident.idNode.idxGlbIdx].id.bwGlb;
            o << (bwGlb->seg << 4) + bwGlb->off <<  "["<<Machine_X86::regName(bwGlb->regi)<<"]";
            break;

        case CONSTANT:
            if (ident.idNode.kte.kte < 1000)
                o << ident.idNode.kte.kte;
            else
                o << "0x"<<std::hex << ident.idNode.kte.kte;
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
            o << writeCall (ident.idNode.call.proc,*ident.idNode.call.args, pProc, numLoc);
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
    } /* eos */
    outStr << o.str();
    cCode.appendDecl(codeOut.str());
    return outStr.str();

}
string UnaryOperator::walkCondExpr(Function *pProc, int *numLoc) const
{
    std::ostringstream outStr;
    bool needBracket=true;
    switch(m_type)
    {
        case NEGATION:
            if (unaryExp->m_type == IDENTIFIER)
            {
                needBracket = false;
                outStr << "!";
            }
            else
                outStr << "! (";
            outStr << unaryExp->walkCondExpr (pProc, numLoc);
            if (needBracket == true)
                outStr << ")";
            break;

        case ADDRESSOF:
            if (unaryExp->m_type == IDENTIFIER)
            {
                needBracket = false;
                outStr << "&";
            }
            else
                outStr << "&(";
            outStr << unaryExp->walkCondExpr (pProc, numLoc);
            if (needBracket == true)
                outStr << ")";
            break;

        case DEREFERENCE:
            outStr << "*";
            if (unaryExp->m_type == IDENTIFIER)
                needBracket = false;
            else
                outStr << "(";
            outStr << unaryExp->walkCondExpr (pProc, numLoc);
            if (needBracket == true)
                outStr << ")";
            break;

        case POST_INC:
            outStr <<  unaryExp->walkCondExpr (pProc, numLoc) << "++";
            break;

        case POST_DEC:
            outStr <<  unaryExp->walkCondExpr (pProc, numLoc) << "--";
            break;

        case PRE_INC:
            outStr << "++"<< unaryExp->walkCondExpr (pProc, numLoc);
            break;

        case PRE_DEC:
            outStr << "--"<< unaryExp->walkCondExpr (pProc, numLoc);
            break;
    }
    return outStr.str();
}

/* Walks the conditional expression tree and returns the result on a string */



/* Makes a copy of the given expression.  Allocates newExp storage for each
 * node.  Returns the copy. */
COND_EXPR *COND_EXPR::clone() const
{
    COND_EXPR* newExp=nullptr;        /* Expression node copy */

    switch (m_type)
    {
        case BOOLEAN_OP:
            assert(false);
            break;

        case NEGATION:
        case ADDRESSOF:
        case DEREFERENCE:
        case PRE_DEC: case POST_DEC:
        case PRE_INC: case POST_INC:
            assert(false);
            break;

        case IDENTIFIER:
            assert(false);
            break;

        default:
            fprintf(stderr,"Clone attempt on unhandled type %d\n",m_type);
    }
    return (newExp);
}


/* Changes the boolean conditional operator at the root of this expression */
void BinaryOperator::changeBoolOp (condOp newOp)
{
    m_op = newOp;
}
bool COND_EXPR::insertSubTreeReg (AstIdent *&tree, COND_EXPR *_expr, eReg regi,const LOCAL_ID *locsym)
{
    COND_EXPR *nd = tree;
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
bool COND_EXPR::insertSubTreeReg (COND_EXPR *&tree, COND_EXPR *_expr, eReg regi,const LOCAL_ID *locsym)
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

COND_EXPR *UnaryOperator::insertSubTreeReg(COND_EXPR *_expr, eReg regi, const LOCAL_ID *locsym)
{

    eReg treeReg;
    COND_EXPR *temp;

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
COND_EXPR *BinaryOperator::insertSubTreeReg(COND_EXPR *_expr, eReg regi, const LOCAL_ID *locsym)
{
    COND_EXPR *r;
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
COND_EXPR *AstIdent::insertSubTreeReg(COND_EXPR *_expr, eReg regi, const LOCAL_ID *locsym)
{
    eReg treeReg;
    if (ident.idType == REGISTER)
    {
        treeReg = locsym->id_arr[ident.idNode.regiIdx].id.regi;
        if (treeReg == regi)                        /* uint16_t reg */
        {
            return _expr;
        }
        else if(Machine_X86::isSubRegisterOf(treeReg,regi))    /* uint16_t/uint8_t reg */
        {
            return _expr;
        }
    }
    return nullptr;
}
/* Inserts the expression exp into the tree at the location specified by the
 * long register index longIdx*/
bool COND_EXPR::insertSubTreeLongReg(COND_EXPR *_expr, COND_EXPR *&tree, int longIdx)
{
    if (tree == NULL)
        return false;
    COND_EXPR *temp=tree->insertSubTreeLongReg(_expr,longIdx);
    if(nullptr!=temp)
    {
        tree=temp;
        return true;
    }
    return false;
}
COND_EXPR *UnaryOperator::insertSubTreeLongReg(COND_EXPR *_expr, int longIdx)
{
    COND_EXPR *temp = unaryExp->insertSubTreeLongReg(_expr,longIdx);
    if (nullptr!=temp)
    {
        unaryExp = temp;
        return this;
    }
    return nullptr;
}
COND_EXPR *BinaryOperator::insertSubTreeLongReg(COND_EXPR *_expr, int longIdx)
{
    COND_EXPR *r;
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
COND_EXPR *AstIdent::insertSubTreeLongReg(COND_EXPR *_expr, int longIdx)
{
    if (ident.idNode.longIdx == longIdx)
    {
        return _expr;
    }
    return nullptr;
}

/* Recursively deallocates the abstract syntax tree rooted at *exp */
COND_EXPR::~COND_EXPR()
{
    switch (m_type)
    {
        case BOOLEAN_OP:
        case NEGATION:
        case ADDRESSOF:
        case DEREFERENCE:
        case IDENTIFIER:
            break;
        default:
            fprintf(stderr,"release attempt on unhandled type %d\n",m_type);
    }
}
/* Makes a copy of the given expression.  Allocates newExp storage for each
 * node.  Returns the copy. */

COND_EXPR *BinaryOperator::clone() const
{
    BinaryOperator* newExp=new BinaryOperator(m_op);        /* Expression node copy */
    newExp->m_lhs = m_lhs->clone();
    newExp->m_rhs = m_rhs->clone();
    return newExp;
}

COND_EXPR *BinaryOperator::inverse() const
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
void AstIdent::performLongRemoval(eReg regi, LOCAL_ID *locId)
{
    eReg otherRegi;         /* high or low part of long register */

    IDENTTYPE* ident_2 = &ident;
    if (ident_2->idType == LONG_VAR)
    {
        otherRegi = otherLongRegi (regi, ident_2->idNode.longIdx, locId);
        ident_2->idType = REGISTER;
        ident_2->regiType = WORD_REG;
        ident_2->idNode.regiIdx = locId->newByteWordReg(TYPE_WORD_SIGN,otherRegi);
    }
}
eReg AstIdent::otherLongRegi (eReg regi, int idx, LOCAL_ID *locTbl)
{
    ID *id;

    id = &locTbl->id_arr[idx];
    if ((id->loc == REG_FRAME) && ((id->type == TYPE_LONG_SIGN) ||
                                   (id->type == TYPE_LONG_UNSIGN)))
    {
        if (id->id.longId.h == regi)
            return (id->id.longId.l);
        else if (id->id.longId.l == regi)
            return (id->id.longId.h);
    }
    return rUNDEF;	// Cristina: please check this!
}
