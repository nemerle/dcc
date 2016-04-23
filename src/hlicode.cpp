/*
 * File:    hlIcode.c
 * Purpose: High-level icode routines
 * Date:    September-October 1993
 * (C) Cristina Cifuentes
 */
#include <cassert>
#include <string.h>
#include <string>
#include <sstream>
#include "dcc.h"
using namespace std;

static char buf[lineSize];     /* Line buffer for hl icode output */



/* Places the new HLI_ASSIGN high-level operand in the high-level icode array */
void HLTYPE::setAsgn(Expr *lhs, Expr *rhs)
{
    assert(lhs);
    set(lhs,rhs);

}
void ICODE::checkHlCall()
{
    //assert((ll()->immed.proc.cb != 0)||ll()->immed.proc.proc!=0);
}
/* Places the new HLI_CALL high-level operand in the high-level icode array */
void ICODE::newCallHl()
{
    type = HIGH__LEVEL;
    hlU()->setCall(ll()->src().proc.proc);

    if (ll()->src().proc.cb != 0)
        hlU()->call.args->cb = ll()->src().proc.cb;
    else if(hl()->call.proc)
        hlU()->call.args->cb = hl()->call.proc->cbParam;
    else
    {
        printf("Function with no cb set, and no valid oper.call.proc , probaby indirect call\n");
        hl()->call.args->cb = 0;
    }
}


/* Places the new HLI_POP/HLI_PUSH/HLI_RET high-level operand in the high-level icode
 * array */
void ICODE::setUnary(hlIcode op, Expr *_exp)
{
    type = HIGH__LEVEL;
    hlU()->set(op,_exp);
}


/* Places the new HLI_JCOND high-level operand in the high-level icode array */
void ICODE::setJCond(Expr *cexp)
{
    type = HIGH__LEVEL;
    hlU()->set(HLI_JCOND,cexp);
}


/* Sets the invalid field to true as this low-level icode is no longer valid,
 * it has been replaced by a high-level icode. */
void ICODE ::invalidate()
{
    invalid = true;
}


/* Removes the defined register regi from the lhs subtree.
 * If all registers
 * of this instruction are unused, the instruction is invalidated (ie. removed)
 */
bool ICODE::removeDefRegi (eReg regi, int thisDefIdx, LOCAL_ID *locId)
{
    int numDefs;

    numDefs = du1.getNumRegsDef();
    if (numDefs == thisDefIdx)
    {
        for ( ; numDefs > 0; numDefs--)
        {

            if (du1.used(numDefs-1)||(du.lastDefRegi.testReg(regi)))
                break;
        }
    }

    if (numDefs == 0)
    {
        invalidate();
        return true;
    }
    HlTypeSupport *p=hlU()->get();
    if(p && p->removeRegFromLong(regi,locId))
    {
        du1.removeDef(regi); //du1.numRegsDef--;
        //du.def &= maskDuReg[regi];
        du.def.clrReg(regi);
    }
    return false;
}
HLTYPE LLInst::createCall()
{
    HLTYPE res(HLI_CALL);
    res.call.proc = src().proc.proc;
    res.call.args = new STKFRAME;

    if (src().proc.cb != 0)
        res.call.args->cb = src().proc.cb;
    else if(res.call.proc)
        res.call.args->cb =res.call.proc->cbParam;
    else
    {
        printf("Function with no cb set, and no valid oper.call.proc , probaby indirect call\n");
        res.call.args->cb = 0;
    }
    return res;
}
#if 0
HLTYPE LLInst::toHighLevel(COND_EXPR *lhs,COND_EXPR *rhs,Function *func)
{
    HLTYPE res(HLI_INVALID);
    if ( testFlags(NOT_HLL) )
        return res;
    flg = getFlag();

    switch (getOpcode())
    {
        case iADD:
            rhs = COND_EXPR::boolOp (lhs, rhs, ADD);
            res.setAsgn(lhs, rhs);
            break;

        case iAND:
            rhs = COND_EXPR::boolOp (lhs, rhs, AND);
            res.setAsgn(lhs, rhs);
            break;

        case iCALL:
        case iCALLF:
            //TODO: this is noop pIcode->checkHlCall();
            res=createCall();
            break;

        case iDEC:
            rhs = AstIdent::idKte (1, 2);
            rhs = COND_EXPR::boolOp (lhs, rhs, SUB);
            res.setAsgn(lhs, rhs);
            break;

        case iDIV:
        case iIDIV:/* should be signed div */
            rhs = COND_EXPR::boolOp (lhs, rhs, DIV);
            if ( ll->testFlags(B) )
            {
                lhs = AstIdent::idReg (rAL, 0, &localId);
                pIcode->setRegDU( rAL, eDEF);
            }
            else
            {
                lhs = AstIdent::idReg (rAX, 0, &localId);
                pIcode->setRegDU( rAX, eDEF);
            }
            res.setAsgn(lhs, rhs);
            break;

        case iIMUL:
            rhs = COND_EXPR::boolOp (lhs, rhs, MUL);
            lhs = AstIdent::id (*pIcode, LHS_OP, func, i, *pIcode, NONE);
            res.setAsgn(lhs, rhs);
            break;

        case iINC:
            rhs = AstIdent::idKte (1, 2);
            rhs = COND_EXPR::boolOp (lhs, rhs, ADD);
            res.setAsgn(lhs, rhs);
            break;

        case iLEA:
            rhs = COND_EXPR::unary (ADDRESSOF, rhs);
            res.setAsgn(lhs, rhs);
            break;

        case iMOD:
            rhs = COND_EXPR::boolOp (lhs, rhs, MOD);
            if ( ll->testFlags(B) )
            {
                lhs = AstIdent::idReg (rAH, 0, &localId);
                pIcode->setRegDU( rAH, eDEF);
            }
            else
            {
                lhs = AstIdent::idReg (rDX, 0, &localId);
                pIcode->setRegDU( rDX, eDEF);
            }
            res.setAsgn(lhs, rhs);
            break;

        case iMOV:    res.setAsgn(lhs, rhs);
            break;

        case iMUL:
            rhs = COND_EXPR::boolOp (lhs, rhs, MUL);
            lhs = AstIdent::id (*pIcode, LHS_OP, this, i, *pIcode, NONE);
            res.setAsgn(lhs, rhs);
            break;

        case iNEG:
            rhs = COND_EXPR::unary (NEGATION, lhs);
            res.setAsgn(lhs, rhs);
            break;

        case iNOT:
            rhs = COND_EXPR::boolOp (NULL, rhs, NOT);
            res.setAsgn(lhs, rhs);
            break;

        case iOR:
            rhs = COND_EXPR::boolOp (lhs, rhs, OR);
            res.setAsgn(lhs, rhs);
            break;

        case iPOP:    res.set(HLI_POP, lhs);
            break;

        case iPUSH:   res.set(HLI_PUSH, lhs);
            break;

        case iRET:
        case iRETF:
            res.set(HLI_RET, NULL);
            break;

        case iSHL:
            rhs = COND_EXPR::boolOp (lhs, rhs, SHL);
            res.setAsgn(lhs, rhs);
            break;

        case iSAR:    /* signed */
        case iSHR:
            rhs = COND_EXPR::boolOp (lhs, rhs, SHR); /* unsigned*/
            res.setAsgn(lhs, rhs);
            break;

        case iSIGNEX:
            res.setAsgn(lhs, rhs);
            break;

        case iSUB:
            rhs = COND_EXPR::boolOp (lhs, rhs, SUB);
            res.setAsgn(lhs, rhs);
            break;

        case iXCHG:
            break;

        case iXOR:
            rhs = COND_EXPR::boolOp (lhs, rhs, XOR);
            res.setAsgn(lhs, rhs);
            break;
    }
    return res;
}
#endif
static bool  needsLhs(unsigned opc)
{
    switch (opc)
    {
        case iCALL:
        case iCALLF:
        case iRET:
        case iRETF:
        default: return false;
        case iADD:
        case iAND:
        case iDEC:
        case iDIV:
        case iIDIV:/* should be signed div */
        case iIMUL:
        case iINC:
        case iLEA:
        case iMOD:
        case iMOV:
        case iMUL:
        case iNEG:
        case iNOT:
        case iOR:
        case iPOP:
        case iPUSH:
        case iSHL:
        case iSAR:    /* signed */
        case iSHR:
        case iSIGNEX:
        case iSUB:
        case iXCHG:
        case iXOR:
            return true;
    }
}
/* Translates LOW__LEVEL icodes to HIGH__LEVEL icodes - 1st stage.
 * Note: this process should be done before data flow analysis, which
 *       refines the HIGH__LEVEL icodes. */
void Function::highLevelGen()
{
    size_t numIcode;        /* number of icode instructions */
    iICODE pIcode;          /* ptr to current icode node */
    Expr *rhs;              /* left- and right-hand side of expression */
    uint32_t _flg;          /* icode flags */
    numIcode = Icode.size();
    for (iICODE i = Icode.begin(); i!=Icode.end() ; ++i)
    {
        Expr *lhs=nullptr;
        assert(numIcode==Icode.size());
        pIcode = i; //Icode.GetIcode(i)
        LLInst *ll = pIcode->ll();
        LLOperand *dst_ll = ll->get(DST);
        LLOperand *src_ll = ll->get(SRC);
        if ( ll->testFlags(NOT_HLL) )
            pIcode->invalidate();
        if ((pIcode->type != LOW__LEVEL) || !pIcode->valid() )
            continue;
        _flg = ll->getFlag();
        if (!ll->testFlags(IM_OPS))   /* not processing IM_OPS yet */
            if (!ll->testFlags(NO_OPS) )       /* if there are opers */
            {
                if (!ll->testFlags(NO_SRC) )   /* if there is src op */
                    rhs = AstIdent::id (*pIcode->ll(), SRC, this, i, *pIcode, NONE);
                if(ll->m_dst.isSet() || (ll->getOpcode()==iMOD))
                    lhs = AstIdent::id (*pIcode->ll(), DST, this, i, *pIcode, NONE);
            }
        if(ll->getOpcode()==iPUSH) {
            if(ll->testFlags(I)) {
                lhs = new Constant(src_ll->opz,src_ll->byteWidth());
            }
//            lhs = AstIdent::id (*pIcode->ll(), DST, this, i, *pIcode, NONE);
        }
        if(needsLhs(ll->getOpcode()))
            assert(lhs!=nullptr);
        switch (ll->getOpcode())
        {
            case iADD:
                rhs = new BinaryOperator(ADD,lhs, rhs);
                pIcode->setAsgn(lhs, rhs);
                break;

            case iAND:
                rhs = BinaryOperator::And(lhs, rhs);
                pIcode->setAsgn(lhs, rhs);
                break;

            case iCALL:
            case iCALLF:
                pIcode->type = HIGH__LEVEL;
                pIcode->hl( ll->createCall() );
                break;

            case iDEC:
                rhs = new BinaryOperator(SUB,lhs, new Constant(1, 2));
                pIcode->setAsgn(lhs, rhs);
                break;

            case iDIV:
            case iIDIV:/* should be signed div */
            {
                eReg v = ( dst_ll->byteWidth()==1) ? rAL:rAX;
                rhs = new BinaryOperator(DIV,lhs, rhs);
                lhs = new RegisterNode(LLOperand(v, dst_ll->byteWidth()), &localId);
                pIcode->setRegDU( v, eDEF);
                pIcode->setAsgn(lhs, rhs);
            }
                break;

            case iIMUL:
                rhs = new BinaryOperator(MUL,lhs, rhs);
                lhs = AstIdent::id (*ll, LHS_OP, this, i, *pIcode, NONE);
                pIcode->setAsgn(lhs, rhs);
                break;

            case iINC:
                rhs = new BinaryOperator(ADD,lhs, new Constant(1, 2));
                pIcode->setAsgn(lhs, rhs);
                break;

            case iLEA:
                rhs =UnaryOperator::Create(ADDRESSOF, rhs);
                pIcode->setAsgn(lhs, rhs);
                break;

            case iMOD:
            {
                rhs = new BinaryOperator(MOD,lhs, rhs);
                eReg lhs_reg = (dst_ll->byteWidth()==1) ? rAH : rDX;
                lhs = new RegisterNode(LLOperand(lhs_reg, dst_ll->byteWidth()), &localId);
                pIcode->setRegDU( lhs_reg, eDEF);
                pIcode->setAsgn(lhs, rhs);
            }
                break;

            case iMOV:    pIcode->setAsgn(lhs, rhs);
                break;

            case iMUL:
                rhs = new BinaryOperator(MUL,lhs, rhs);
                lhs = AstIdent::id (*ll, LHS_OP, this, i, *pIcode, NONE);
                pIcode->setAsgn(lhs, rhs);
                break;

            case iNEG:
                rhs = UnaryOperator::Create(NEGATION, lhs);
                pIcode->setAsgn(lhs, rhs);
                break;

            case iNOT:
                rhs = new BinaryOperator(NOT,nullptr, rhs); // TODO: change this to unary NOT ?
                pIcode->setAsgn(lhs, rhs);
                break;

            case iOR:
                rhs = new BinaryOperator(OR,lhs, rhs);
                pIcode->setAsgn(lhs, rhs);
                break;

            case iPOP:
                pIcode->setUnary(HLI_POP, lhs);
                break;

            case iPUSH:
                pIcode->setUnary(HLI_PUSH, lhs);
                break;

            case iRET:
            case iRETF:   pIcode->setUnary(HLI_RET, nullptr);
                break;

            case iSHL:
                rhs = new BinaryOperator(SHL,lhs, rhs);
                pIcode->setAsgn(lhs, rhs);
                break;

            case iSAR:    /* signed */
            case iSHR:
                rhs = new BinaryOperator(SHR,lhs, rhs); /* unsigned*/
                pIcode->setAsgn(lhs, rhs);
                break;

            case iSIGNEX: pIcode->setAsgn(lhs, rhs);
                break;

            case iSUB:
                rhs = new BinaryOperator(SUB,lhs, rhs);
                pIcode->setAsgn(lhs, rhs);
                break;

            case iXCHG:
                break;

            case iXOR:
                rhs = new BinaryOperator(XOR,lhs, rhs);
                pIcode->setAsgn(lhs, rhs);
                break;
        }
    }

}


/**
 \fn COND_EXPR::inverse
 Modifies the given conditional operator to its inverse.  This is used
 in if..then[..else] statements, to reflect the condition that takes the
 then part.
*/


/* Returns the string that represents the procedure call of tproc (ie. with
 * actual parameters) */
std::string Function::writeCall (Function * tproc, STKFRAME & args, int *numLoc)
{
    //string condExp;
    ostringstream ostr;
    ostr<<tproc->name<<" (";
    for(const STKSYM &sym : args)
    {
        if(sym.actual)
            ostr << sym.actual->walkCondExpr (this, numLoc);
        else
            ostr <<  "";
        if((&sym)!=&(args.back()))
            ostr << ", ";
    }
    ostr << ")";
    return ostr.str();
}


/* Displays the output of a HLI_JCOND icode. */
const char *writeJcond (const HLTYPE &h, Function * pProc, int *numLoc)
{
    memset (buf, ' ', sizeof(buf));
    buf[0] = '\0';
    strcat (buf, "if ");
    if(h.opcode==HLI_INVALID)
    {
        return "if (*HLI_INVALID*) {\n";
    }
    assert(h.expr());
    Expr *inverted=h.expr()->inverse();
    //inverseCondOp (&h.exp);
    std::string e = inverted->walkCondExpr (pProc, numLoc);
    delete inverted;
    strcat (buf, e.c_str());
    strcat (buf, " {\n");
    return (buf);
}


/* Displays the inverse output of a HLI_JCOND icode.  This is used in the case
 * when the THEN clause of an if..then..else is empty.  The clause is
 * negated and the ELSE clause is used instead.	*/
const char *writeJcondInv(HLTYPE h, Function * pProc, int *numLoc)
{
    memset (buf, ' ', sizeof(buf));
    buf[0] = '\0';
    strcat (buf, "if ");
    std::string e;
    if(h.expr()==nullptr)
        e = "( *failed condition recovery* )";
    else
        e = h.expr()->walkCondExpr (pProc, numLoc);

    strcat (buf, e.c_str());
    strcat (buf, " {\n");
    return (buf);
}

string AssignType::writeOut(Function *pProc, int *numLoc) const
{
    ostringstream ostr;
    ostr << m_lhs->walkCondExpr (pProc, numLoc);
    ostr << " = ";
    ostr << rhs->walkCondExpr (pProc, numLoc);
    ostr << ";\n";
    return ostr.str();
}
string CallType::writeOut(Function *pProc, int *numLoc) const
{
    ostringstream ostr;
    ostr << pProc->writeCall (proc, *args, numLoc);
    ostr << ";\n";
    return ostr.str();
}
string ExpType::writeOut(Function *pProc, int *numLoc) const
{
    if(v==nullptr)
        return "";
    return v->walkCondExpr (pProc, numLoc);
}

void HLTYPE::set(Expr *l, Expr *r)
{
    assert(l);
    assert(r);
    opcode = HLI_ASSIGN;
    //assert((asgn.lhs==0) and (asgn.rhs==0)); //prevent memory leaks
    assert(dynamic_cast<UnaryOperator *>(l));
    asgn.m_lhs=l;
    asgn.rhs=r;
}
/* Returns a string with the contents of the current high-level icode.
 * Note: this routine does not output the contens of HLI_JCOND icodes.  This is
 * 		 done in a separate routine to be able to support the removal of
 *		 empty THEN clauses on an if..then..else.	*/
string HLTYPE::write1HlIcode (Function * pProc, int *numLoc) const
{
    string e;
    ostringstream ostr;
    const HlTypeSupport *p = get();
    switch (opcode)
    {
        case HLI_ASSIGN:
            return p->writeOut(pProc,numLoc);
        case HLI_CALL:
            return p->writeOut(pProc,numLoc);
        case HLI_RET:
            e = p->writeOut(pProc,numLoc);
            if (! e.empty())
                ostr << "return (" << e << ");\n";
            break;
        case HLI_POP:
            ostr << "HLI_POP ";
            ostr << p->writeOut(pProc,numLoc);
            ostr << "\n";
            break;
        case HLI_PUSH:
            ostr << "HLI_PUSH ";
            ostr << p->writeOut(pProc,numLoc);
            ostr << "\n";
            break;
        case HLI_JCOND: //Handled elsewhere
            break;
        default:
            fprintf(stderr," HLTYPE::write1HlIcode - Unhandled opcode %d\n",opcode);
    }
    return ostr.str();
}


//TODO: replace  all "< (INDEX_BX_SI-1)" with machine_x86::lastReg

//TODO: replace all >= INDEX_BX_SI with machine_x86::isRegExpression


/* Writes the registers/stack variables that are used and defined by this
 * instruction. */
void ICODE::writeDU()
{
    int my_idx = loc_ip;
    {
        ostringstream ostr;
        Machine_X86::writeRegVector(ostr,du.def);
        if (!ostr.str().empty())
            printf ("Def (reg) = %s\n", ostr.str().c_str());
    }
    {
        ostringstream ostr;
        Machine_X86::writeRegVector(ostr,du.use);
        if (!ostr.str().empty())
            printf ("Use (reg) = %s\n", ostr.str().c_str());
    }

    /* Print du1 chain */
    printf ("# regs defined = %d\n", du1.getNumRegsDef());
    for (int i = 0; i < MAX_REGS_DEF; i++)
    {
        if (!du1.used(i))
            continue;
        printf ("%d: du1[%d][] = ", my_idx, i);
        for(auto j : du1.idx[i].uses)
        {
            printf ("%d ", j->loc_ip);
        }
        printf ("\n");
    }

    /* For HLI_CALL, print # parameter bytes */
    if (hl()->opcode == HLI_CALL)
        printf ("# param bytes = %d\n", hl()->call.args->cb);
    printf ("\n");
}
