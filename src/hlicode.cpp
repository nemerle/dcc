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
#define ICODE_DELTA 25

/* Masks off bits set by duReg[] */
uint32_t maskDuReg[] = { 0x00,
                      0xFEEFFE, 0xFDDFFD, 0xFBB00B, 0xF77007, /* uint16_t regs */
                      0xFFFFEF, 0xFFFFDF, 0xFFFFBF, 0xFFFF7F,
                      0xFFFEFF, 0xFFFDFF, 0xFFFBFF, 0xFFF7FF, /* seg regs  */
                      0xFFEFFF, 0xFFDFFF, 0xFFBFFF, 0xFF7FFF, /* uint8_t regs */
                      0xFEFFFF, 0xFDFFFF, 0xFBFFFF, 0xF7FFFF,
                      0xEFFFFF,                               /* tmp reg   */
                      0xFFFFB7, 0xFFFF77, 0xFFFF9F, 0xFFFF5F, /* index regs */
                      0xFFFFBF, 0xFFFF7F, 0xFFFFDF, 0xFFFFF7 };

static char buf[lineSize];     /* Line buffer for hl icode output */



/* Places the new HLI_ASSIGN high-level operand in the high-level icode array */
void ICODE::setAsgn(COND_EXPR *lhs, COND_EXPR *rhs)
{
    type = HIGH_LEVEL;
    ic.hl.set(lhs,rhs);

}
void ICODE::checkHlCall()
{
    //assert((ic.ll.immed.proc.cb != 0)||ic.ll.immed.proc.proc!=0);
}
/* Places the new HLI_CALL high-level operand in the high-level icode array */
void ICODE::newCallHl()
{
    type = HIGH_LEVEL;
    ic.hl.opcode = HLI_CALL;
    ic.hl.call.proc = ic.ll.src.proc.proc;
    ic.hl.call.args = new STKFRAME;

    if (ic.ll.src.proc.cb != 0)
        ic.hl.call.args->cb = ic.ll.src.proc.cb;
    else if(ic.hl.call.proc)
        ic.hl.call.args->cb =ic.hl.call.proc->cbParam;
    else
    {
        printf("Function with no cb set, and no valid oper.call.proc , probaby indirect call\n");
        ic.hl.call.args->cb = 0;
    }
}


/* Places the new HLI_POP/HLI_PUSH/HLI_RET high-level operand in the high-level icode
 * array */
void ICODE::setUnary(hlIcode op, COND_EXPR *exp)
{
    type = HIGH_LEVEL;
    ic.hl.set(op,exp);
}


/* Places the new HLI_JCOND high-level operand in the high-level icode array */
void ICODE::setJCond(COND_EXPR *cexp)
{
    type = HIGH_LEVEL;
    ic.hl.set(HLI_JCOND,cexp);
}


/* Sets the invalid field to TRUE as this low-level icode is no longer valid,
 * it has been replaced by a high-level icode. */
void ICODE ::invalidate()
{
    invalid = TRUE;
}


/* Removes the defined register regi from the lhs subtree.
 * If all registers
 * of this instruction are unused, the instruction is invalidated (ie. removed)
 */
bool ICODE::removeDefRegi (uint8_t regi, int thisDefIdx, LOCAL_ID *locId)
{
    int numDefs;

    numDefs = du1.numRegsDef;
//    if (numDefs == thisDefIdx)
//    {
//        for ( ; numDefs > 0; numDefs--)
//        {
//            if ((du1.idx[numDefs-1][0] != 0)||(du.lastDefRegi.any()))
//                break;
//        }
//    }

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
    HlTypeSupport *p=ic.hl.get();
    if(p and p->removeRegFromLong(regi,locId))
    {
        du1.numRegsDef--;
        du.def &= maskDuReg[regi];
    }
    return false;
}


/* Translates LOW_LEVEL icodes to HIGH_LEVEL icodes - 1st stage.
 * Note: this process should be done before data flow analysis, which
 *       refines the HIGH_LEVEL icodes. */
void Function::highLevelGen()
{   int i,                /* idx into icode array */
            numIcode;         /* number of icode instructions */
    iICODE pIcode;        /* ptr to current icode node */
    COND_EXPR *lhs, *rhs; /* left- and right-hand side of expression */
    uint32_t flg;          /* icode flags */

    numIcode = Icode.size();
    for (iICODE i = Icode.begin(); i!=Icode.end() ; ++i)
    {
        assert(numIcode==Icode.size());
        pIcode = i; //Icode.GetIcode(i)
        if ((pIcode->ic.ll.flg & NOT_HLL) == NOT_HLL)
            pIcode->invalidate();
        if ((pIcode->type == LOW_LEVEL) && (pIcode->invalid == FALSE))
        {
            flg = pIcode->ic.ll.flg;
            if ((flg & IM_OPS) != IM_OPS)   /* not processing IM_OPS yet */
                if ((flg & NO_OPS) != NO_OPS)       /* if there are opers */
                {
                    if ((flg & NO_SRC) != NO_SRC)   /* if there is src op */
                        rhs = COND_EXPR::id (*pIcode, SRC, this, i, *pIcode, NONE);
                    lhs = COND_EXPR::id (*pIcode, DST, this, i, *pIcode, NONE);
                }

            switch (pIcode->ic.ll.opcode)
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
                    pIcode->checkHlCall();
                    pIcode->newCallHl();
                    break;

                case iDEC:
                    rhs = COND_EXPR::idKte (1, 2);
                    rhs = COND_EXPR::boolOp (lhs, rhs, SUB);
                    pIcode->setAsgn(lhs, rhs);
                    break;

                case iDIV:
                case iIDIV:/* should be signed div */
                    rhs = COND_EXPR::boolOp (lhs, rhs, DIV);
                    if (pIcode->ic.ll.flg & B)
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
                    lhs = COND_EXPR::id (*pIcode, LHS_OP, this, i, *pIcode, NONE);
                    pIcode->setAsgn(lhs, rhs);
                    break;

                case iINC:
                    rhs = COND_EXPR::idKte (1, 2);
                    rhs = COND_EXPR::boolOp (lhs, rhs, ADD);
                    pIcode->setAsgn(lhs, rhs);
                    break;

                case iLEA:    rhs = COND_EXPR::unary (ADDRESSOF, rhs);
                    pIcode->setAsgn(lhs, rhs);
                    break;

                case iMOD:    rhs = COND_EXPR::boolOp (lhs, rhs, MOD);
                    if (pIcode->ic.ll.flg & B)
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
                    lhs = COND_EXPR::id (*pIcode, LHS_OP, this, i, *pIcode, NONE);
                    pIcode->setAsgn(lhs, rhs);
                    break;

                case iNEG:    rhs = COND_EXPR::unary (NEGATION, lhs);
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

                case iSUB:    rhs = COND_EXPR::boolOp (lhs, rhs, SUB);
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

}


/* Modifies the given conditional operator to its inverse.  This is used
 * in if..then[..else] statements, to reflect the condition that takes the
 * then part. 	*/
COND_EXPR *COND_EXPR::inverse ()
{
    static condOp invCondOp[] = {GREATER, GREATER_EQUAL, NOT_EQUAL, EQUAL,
                                 LESS_EQUAL, LESS, DUMMY,DUMMY,DUMMY,DUMMY,
                                 DUMMY, DUMMY, DUMMY, DUMMY, DUMMY, DUMMY,
                                 DUMMY, DBL_OR, DBL_AND};
    COND_EXPR *res=0;
    if (type == BOOLEAN_OP)
    {
        switch (expr.boolExpr.op)
        {
            case LESS_EQUAL: case LESS: case EQUAL:
            case NOT_EQUAL: case GREATER: case GREATER_EQUAL:
                res = this->clone();
                res->expr.boolExpr.op = invCondOp[expr.boolExpr.op];
                return res;

            case AND: case OR: case XOR: case NOT: case ADD:
            case SUB: case MUL: case DIV: case SHR: case SHL: case MOD:
                return COND_EXPR::unary (NEGATION, this->clone());

            case DBL_AND: case DBL_OR:
                res = this->clone();
                res->expr.boolExpr.op = invCondOp[expr.boolExpr.op];
                res->expr.boolExpr.lhs=expr.boolExpr.lhs->inverse ();
                res->expr.boolExpr.rhs=expr.boolExpr.rhs->inverse ();
                return res;
        } /* eos */

    }
    else if (type == NEGATION) //TODO: memleak here
    {
        return expr.unaryExp->clone();
    }
    return this->clone();
    /* other types are left unmodified */
}

/* Returns the string that represents the procedure call of tproc (ie. with
 * actual parameters) */
std::string writeCall (Function * tproc, STKFRAME * args, Function * pproc, int *numLoc)
{
    int i;                        /* counter of # arguments       */
    string condExp;
    ostringstream s;
    s<<tproc->name<<" (";
    for (i = 0; i < args->sym.size(); i++)
    {
        s << walkCondExpr (args->sym[i].actual, pproc, numLoc);
        if (i < (args->sym.size() - 1))
            s << ", ";
    }
    s << ")";
    return s.str();
}


/* Displays the output of a HLI_JCOND icode. */
char *writeJcond (HLTYPE h, Function * pProc, int *numLoc)
{
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
    ostr << writeCall (proc, args, pProc,numLoc);
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
    }
    return ostr.str();
}


int power2 (int i)
/* Returns the value of 2 to the power of i */
{
    if (i == 0)
        return (1);
    return (2 << (i-1));
}


/* Writes the registers/stack variables that are used and defined by this
 * instruction. */
void ICODE::writeDU(int idx)
{
    static char buf[100];
    int i, j;

    memset (buf, ' ', sizeof(buf));
    buf[0] = '\0';
    for (i = 0; i < (INDEXBASE-1); i++)
    {
        if (du.def[i])
        {
            strcat (buf, allRegs[i]);
            strcat (buf, " ");
        }
    }
    if (buf[0] != '\0')
        printf ("Def (reg) = %s\n", buf);

    memset (buf, ' ', sizeof(buf));
    buf[0] = '\0';
    for (i = 0; i < INDEXBASE; i++)
    {
        if (du.use[i])
        {
            strcat (buf, allRegs[i]);
            strcat (buf, " ");
        }
    }
    if (buf[0] != '\0')
        printf ("Use (reg) = %s\n", buf);

    /* Print du1 chain */
    printf ("# regs defined = %d\n", du1.numRegsDef);
    for (i = 0; i < MAX_REGS_DEF; i++)
    {
        if (du1.used(i))
        {
            printf ("%d: du1[%d][] = ", idx, i);
            for(std::list<ICODE>::iterator j : du1.idx[i].uses)
            {
                printf ("%d ", j->loc_ip);
            }
            printf ("\n");
        }
    }

    /* For HLI_CALL, print # parameter bytes */
    if (ic.hl.opcode == HLI_CALL)
        printf ("# param bytes = %d\n", ic.hl.call.args->cb);
    printf ("\n");
}




