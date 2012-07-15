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


/* Masks off bits set by duReg[] */
LivenessSet maskDuReg[] = { 0x00,
                            /* uint16_t regs */
                            0xFEEFFE, //rAX
                            0xFDDFFD, //rCX
                            0xFBB00B, //rDX
                            0xF77007, //rBX
                            0xFFFFEF, //rSP
                            0xFFFFDF, //rBP
                            0xFFFFBF, //rSI
                            0xFFFF7F, //rDI
                            0xFFFEFF,
                            0xFFFDFF,
                            0xFFFBFF,
                            0xFFF7FF, /* seg regs  */
                            0xFFEFFF, 0xFFDFFF, 0xFFBFFF, 0xFF7FFF, /* uint8_t regs */
                            0xFEFFFF, 0xFDFFFF, 0xFBFFFF, 0xF7FFFF,
                            0xEFFFFF,                               /* tmp reg   */
                            0xFFFFB7, 0xFFFF77, 0xFFFF9F, 0xFFFF5F, /* index regs */
                            0xFFFFBF, 0xFFFF7F, 0xFFFFDF, 0xFFFFF7 };

static char buf[lineSize];     /* Line buffer for hl icode output */



/* Places the new HLI_ASSIGN high-level operand in the high-level icode array */
void HLTYPE::setAsgn(COND_EXPR *lhs, COND_EXPR *rhs)
{
    set(lhs,rhs);

}
void ICODE::checkHlCall()
{
    //assert((ll()->immed.proc.cb != 0)||ll()->immed.proc.proc!=0);
}
/* Places the new HLI_CALL high-level operand in the high-level icode array */
void ICODE::newCallHl()
{
    type = HIGH_LEVEL;
    hl()->opcode = HLI_CALL;
    hl()->call.proc = ll()->src().proc.proc;
    hl()->call.args = new STKFRAME;

    if (ll()->src().proc.cb != 0)
        hl()->call.args->cb = ll()->src().proc.cb;
    else if(hl()->call.proc)
        hl()->call.args->cb =hl()->call.proc->cbParam;
    else
    {
        printf("Function with no cb set, and no valid oper.call.proc , probaby indirect call\n");
        hl()->call.args->cb = 0;
    }
}


/* Places the new HLI_POP/HLI_PUSH/HLI_RET high-level operand in the high-level icode
 * array */
void ICODE::setUnary(hlIcode op, COND_EXPR *_exp)
{
    type = HIGH_LEVEL;
    hl()->set(op,_exp);
}


/* Places the new HLI_JCOND high-level operand in the high-level icode array */
void ICODE::setJCond(COND_EXPR *cexp)
{
    type = HIGH_LEVEL;
    hl()->set(HLI_JCOND,cexp);
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

    numDefs = du1.numRegsDef;
    if (numDefs == thisDefIdx)
    {
        for ( ; numDefs > 0; numDefs--)
        {

            if (du1.used(numDefs-1)||(du.lastDefRegi[regi]))
                break;
        }
    }

    if (numDefs == 0)
    {
        invalidate();
        return true;
    }
    HlTypeSupport *p=hl()->get();
    if(p and p->removeRegFromLong(regi,locId))
    {
        du1.numRegsDef--;
        du.def &= maskDuReg[regi];
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
            rhs = COND_EXPR::idKte (1, 2);
            rhs = COND_EXPR::boolOp (lhs, rhs, SUB);
            res.setAsgn(lhs, rhs);
            break;

        case iDIV:
        case iIDIV:/* should be signed div */
            rhs = COND_EXPR::boolOp (lhs, rhs, DIV);
            if ( ll->testFlags(B) )
            {
                lhs = COND_EXPR::idReg (rAL, 0, &localId);
                pIcode->setRegDU( rAL, eDEF);
            }
            else
            {
                lhs = COND_EXPR::idReg (rAX, 0, &localId);
                pIcode->setRegDU( rAX, eDEF);
            }
            res.setAsgn(lhs, rhs);
            break;

        case iIMUL:
            rhs = COND_EXPR::boolOp (lhs, rhs, MUL);
            lhs = COND_EXPR::id (*pIcode, LHS_OP, func, i, *pIcode, NONE);
            res.setAsgn(lhs, rhs);
            break;

        case iINC:
            rhs = COND_EXPR::idKte (1, 2);
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
                lhs = COND_EXPR::idReg (rAH, 0, &localId);
                pIcode->setRegDU( rAH, eDEF);
            }
            else
            {
                lhs = COND_EXPR::idReg (rDX, 0, &localId);
                pIcode->setRegDU( rDX, eDEF);
            }
            res.setAsgn(lhs, rhs);
            break;

        case iMOV:    res.setAsgn(lhs, rhs);
            break;

        case iMUL:
            rhs = COND_EXPR::boolOp (lhs, rhs, MUL);
            lhs = COND_EXPR::id (*pIcode, LHS_OP, this, i, *pIcode, NONE);
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
/* Translates LOW_LEVEL icodes to HIGH_LEVEL icodes - 1st stage.
 * Note: this process should be done before data flow analysis, which
 *       refines the HIGH_LEVEL icodes. */
void Function::highLevelGen()
{
    size_t numIcode;         /* number of icode instructions */
    iICODE pIcode;        /* ptr to current icode node */
    COND_EXPR *lhs, *rhs; /* left- and right-hand side of expression */
    uint32_t _flg;          /* icode flags */
    numIcode = Icode.size();
    for (iICODE i = Icode.begin(); i!=Icode.end() ; ++i)
    {
        assert(numIcode==Icode.size());
        pIcode = i; //Icode.GetIcode(i)
        LLInst *ll = pIcode->ll();
        if ( ll->testFlags(NOT_HLL) )
            pIcode->invalidate();
        if ((pIcode->type != LOW_LEVEL) or not pIcode->valid() )
            continue;
        _flg = ll->getFlag();
        if ((_flg & IM_OPS) != IM_OPS)   /* not processing IM_OPS yet */
            if ((_flg & NO_OPS) != NO_OPS)       /* if there are opers */
            {
                if ( not ll->testFlags(NO_SRC) )   /* if there is src op */
                    rhs = COND_EXPR::id (*pIcode->ll(), SRC, this, i, *pIcode, NONE);
                lhs = COND_EXPR::id (*pIcode->ll(), DST, this, i, *pIcode, NONE);
            }
        switch (ll->getOpcode())
        {
            case iADD:
                rhs = COND_EXPR::boolOp (lhs, rhs, ADD);
                pIcode->setAsgn(lhs, rhs);
                break;

            case iAND:
                rhs = COND_EXPR::boolOp (lhs, rhs, AND);
                pIcode->setAsgn(lhs, rhs);
                break;

            case iCALL:
            case iCALLF:
                pIcode->type = HIGH_LEVEL;
                pIcode->hl( ll->createCall() );
                break;

            case iDEC:
                rhs = COND_EXPR::idKte (1, 2);
                rhs = COND_EXPR::boolOp (lhs, rhs, SUB);
                pIcode->setAsgn(lhs, rhs);
                break;

            case iDIV:
            case iIDIV:/* should be signed div */
                rhs = COND_EXPR::boolOp (lhs, rhs, DIV);
                if ( ll->testFlags(B) )
                {
                    lhs = COND_EXPR::idReg (rAL, 0, &localId);
                    pIcode->setRegDU( rAL, eDEF);
                }
                else
                {
                    lhs = COND_EXPR::idReg (rAX, 0, &localId);
                    pIcode->setRegDU( rAX, eDEF);
                }
                pIcode->setAsgn(lhs, rhs);
                break;

            case iIMUL:
                rhs = COND_EXPR::boolOp (lhs, rhs, MUL);
                lhs = COND_EXPR::id (*ll, LHS_OP, this, i, *pIcode, NONE);
                pIcode->setAsgn(lhs, rhs);
                break;

            case iINC:
                rhs = COND_EXPR::idKte (1, 2);
                rhs = COND_EXPR::boolOp (lhs, rhs, ADD);
                pIcode->setAsgn(lhs, rhs);
                break;

            case iLEA:
                rhs = COND_EXPR::unary (ADDRESSOF, rhs);
                pIcode->setAsgn(lhs, rhs);
                break;

            case iMOD:
                rhs = COND_EXPR::boolOp (lhs, rhs, MOD);
                if ( ll->testFlags(B) )
                {
                    lhs = COND_EXPR::idReg (rAH, 0, &localId);
                    pIcode->setRegDU( rAH, eDEF);
                }
                else
                {
                    lhs = COND_EXPR::idReg (rDX, 0, &localId);
                    pIcode->setRegDU( rDX, eDEF);
                }
                pIcode->setAsgn(lhs, rhs);
                break;

            case iMOV:    pIcode->setAsgn(lhs, rhs);
                break;

            case iMUL:
                rhs = COND_EXPR::boolOp (lhs, rhs, MUL);
                lhs = COND_EXPR::id (*ll, LHS_OP, this, i, *pIcode, NONE);
                pIcode->setAsgn(lhs, rhs);
                break;

            case iNEG:
                rhs = COND_EXPR::unary (NEGATION, lhs);
                pIcode->setAsgn(lhs, rhs);
                break;

            case iNOT:
                rhs = COND_EXPR::boolOp (NULL, rhs, NOT);
                pIcode->setAsgn(lhs, rhs);
                break;

            case iOR:
                rhs = COND_EXPR::boolOp (lhs, rhs, OR);
                pIcode->setAsgn(lhs, rhs);
                break;

            case iPOP:    pIcode->setUnary(HLI_POP, lhs);
                break;

            case iPUSH:   pIcode->setUnary(HLI_PUSH, lhs);
                break;

            case iRET:
            case iRETF:   pIcode->setUnary(HLI_RET, NULL);
                break;

            case iSHL:
                rhs = COND_EXPR::boolOp (lhs, rhs, SHL);
                pIcode->setAsgn(lhs, rhs);
                break;

            case iSAR:    /* signed */
            case iSHR:
                rhs = COND_EXPR::boolOp (lhs, rhs, SHR); /* unsigned*/
                pIcode->setAsgn(lhs, rhs);
                break;

            case iSIGNEX: pIcode->setAsgn(lhs, rhs);
                break;

            case iSUB:
                rhs = COND_EXPR::boolOp (lhs, rhs, SUB);
                pIcode->setAsgn(lhs, rhs);
                break;

            case iXCHG:
                break;

            case iXOR:
                rhs = COND_EXPR::boolOp (lhs, rhs, XOR);
                pIcode->setAsgn(lhs, rhs);
                break;
        }
    }

}


/* Modifies the given conditional operator to its inverse.  This is used
 * in if..then[..else] statements, to reflect the condition that takes the
 * then part. 	*/
COND_EXPR *COND_EXPR::inverse () const
{
    static condOp invCondOp[] = {GREATER, GREATER_EQUAL, NOT_EQUAL, EQUAL,
                                 LESS_EQUAL, LESS, DUMMY,DUMMY,DUMMY,DUMMY,
                                 DUMMY, DUMMY, DUMMY, DUMMY, DUMMY, DUMMY,
                                 DUMMY, DBL_OR, DBL_AND};
    COND_EXPR *res=0;
    if (m_type == BOOLEAN_OP)
    {
        switch ( op() )
        {
            case LESS_EQUAL: case LESS: case EQUAL:
            case NOT_EQUAL: case GREATER: case GREATER_EQUAL:
                res = this->clone();
                res->boolExpr.op = invCondOp[op()];
                return res;

            case AND: case OR: case XOR: case NOT: case ADD:
            case SUB: case MUL: case DIV: case SHR: case SHL: case MOD:
                return COND_EXPR::unary (NEGATION, this->clone());

            case DBL_AND: case DBL_OR:
            // Binary::Create(invertop,lhs->inverse(),rhs->inverse());
                res = this->clone();
                res->boolExpr.op = invCondOp[op()];
                res->boolExpr.lhs=lhs()->inverse ();
                res->boolExpr.rhs=rhs()->inverse ();
                return res;
            default:
                fprintf(stderr,"COND_EXPR::inverse unhandled op %d",op());
        } /* eos */

    }
    else if (m_type == NEGATION) //TODO: memleak here
    {
        return expr.unaryExp->clone();
    }
    return this->clone();
    /* other types are left unmodified */
}

/* Returns the string that represents the procedure call of tproc (ie. with
 * actual parameters) */
std::string writeCall (Function * tproc, STKFRAME & args, Function * pproc, int *numLoc)
{
    //string condExp;
    ostringstream ostr;
    ostr<<tproc->name<<" (";
    for(const STKSYM &sym : args)
    {
        ostr << walkCondExpr (sym.actual, pproc, numLoc);
        if((&sym)!=&(args.back()))
            ostr << ", ";
    }
    ostr << ")";
    return ostr.str();
}


/* Displays the output of a HLI_JCOND icode. */
char *writeJcond (const HLTYPE &h, Function * pProc, int *numLoc)
{
    assert(h.expr());
    memset (buf, ' ', sizeof(buf));
    buf[0] = '\0';
    strcat (buf, "if ");
    COND_EXPR *inverted=h.expr()->inverse();
    //inverseCondOp (&h.exp);
    std::string e = walkCondExpr (inverted, pProc, numLoc);
    delete inverted;
    strcat (buf, e.c_str());
    strcat (buf, " {\n");
    return (buf);
}


/* Displays the inverse output of a HLI_JCOND icode.  This is used in the case
 * when the THEN clause of an if..then..else is empty.  The clause is
 * negated and the ELSE clause is used instead.	*/
char *writeJcondInv (HLTYPE h, Function * pProc, int *numLoc)
{
    memset (buf, ' ', sizeof(buf));
    buf[0] = '\0';
    strcat (buf, "if ");
    std::string e = walkCondExpr (h.expr(), pProc, numLoc);
    strcat (buf, e.c_str());
    strcat (buf, " {\n");
    return (buf);
}

string AssignType::writeOut(Function *pProc, int *numLoc)
{
    ostringstream ostr;
    ostr << walkCondExpr (lhs, pProc, numLoc);
    ostr << " = ";
    ostr << walkCondExpr (rhs, pProc, numLoc);
    ostr << ";\n";
    return ostr.str();
}
string CallType::writeOut(Function *pProc, int *numLoc)
{
    ostringstream ostr;
    ostr << writeCall (proc, *args, pProc,numLoc);
    ostr << ";\n";
    return ostr.str();
}
string ExpType::writeOut(Function *pProc, int *numLoc)
{
    return walkCondExpr (v, pProc, numLoc);
}

/* Returns a string with the contents of the current high-level icode.
 * Note: this routine does not output the contens of HLI_JCOND icodes.  This is
 * 		 done in a separate routine to be able to support the removal of
 *		 empty THEN clauses on an if..then..else.	*/
string HLTYPE::write1HlIcode (Function * pProc, int *numLoc)
{
    string e;
    ostringstream ostr;
    HlTypeSupport *p = get();
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
    printf ("# regs defined = %d\n", du1.numRegsDef);
    for (int i = 0; i < MAX_REGS_DEF; i++)
    {
        if (not du1.used(i))
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
