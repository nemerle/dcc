/*****************************************************************************
 * Project: dcc
 * File:    dataflow.c
 * Purpose: Data flow analysis module.
 * (C) Cristina Cifuentes
 ****************************************************************************/

#include "dcc.h"
#include <string.h>
#include <iostream>
#include <iomanip>
#include <stdio.h>
struct ExpStack
{
    typedef std::list<COND_EXPR *> EXP_STK;
    EXP_STK expStk;      /* local expression stack */

    void        init();
    void        push(COND_EXPR *);
    COND_EXPR * pop();
    Int         numElem();
    boolT	empty();
};
/***************************************************************************
 * Expression stack functions
 **************************************************************************/

/* Reinitalizes the expression stack (expStk) to NULL, by freeing all the
 * space allocated (if any).        */
void ExpStack::init()
{
    expStk.clear();
}


/* Pushes the given expression onto the local stack (expStk). */
void ExpStack::push(COND_EXPR *expr)
{
    expStk.push_back(expr);
}


/* Returns the element on the top of the local expression stack (expStk),
 * and deallocates the space allocated by this node.
 * If there are no elements on the stack, returns NULL. */
COND_EXPR *ExpStack::pop()
{
    if(expStk.empty())
        return 0;
    COND_EXPR *topExp = expStk.back();
    expStk.pop_back();
    return topExp;
}

/* Returns the number of elements available in the expression stack */
Int ExpStack::numElem()
{
    return expStk.size();
}

/* Returns whether the expression stack is empty or not */
boolT ExpStack::empty()
{
    return expStk.empty();
}

using namespace std;
ExpStack g_exp_stk;

/* Returns the index of the local variable or parameter at offset off, if it
 * is in the stack frame provided.  */
Int STKFRAME::getLocVar(Int off)
{
    Int     i;

    for (i = 0; i < sym.size(); i++)
        if (sym[i].off == off)
            break;
    return (i);
}


/* Returns a string with the source operand of Icode */
static COND_EXPR *srcIdent (const ICODE &Icode, Function * pProc, Int i, ICODE & duIcode, operDu du)
{
    COND_EXPR *n;

    if (Icode.ic.ll.flg & I)   /* immediate operand */
    {
        if (Icode.ic.ll.flg & B)
            n = COND_EXPR::idKte (Icode.ic.ll.immed.op, 1);
        else
            n = COND_EXPR::idKte (Icode.ic.ll.immed.op, 2);
    }
    else
        n = COND_EXPR::id (Icode, SRC, pProc, i, duIcode, du);
    return (n);
}


/* Returns the destination operand */
static COND_EXPR *dstIdent (const ICODE & Icode, Function * pProc, Int i, ICODE & duIcode, operDu du)
{
    COND_EXPR *n;
    n = COND_EXPR::id (Icode, DST, pProc, i, duIcode, du);
    /** Is it needed? (pIcode->ic.ll.flg) & NO_SRC_B **/
    return (n);
}



/* Eliminates all condition codes and generates new hlIcode instructions */
void Function::elimCondCodes ()
{
    Int i;

    byte use;           /* Used flags bit vector                  */
    byte def;           /* Defined flags bit vector               */
    boolT notSup;       /* Use/def combination not supported      */
    COND_EXPR *rhs;     /* Source operand                         */
    COND_EXPR *lhs;     /* Destination operand                    */
    COND_EXPR *exp;     /* Boolean expression                     */
    BB * pBB;           /* Pointer to BBs in dfs last ordering    */
    riICODE useAt;      /* Instruction that used flag    */
    riICODE defAt;      /* Instruction that defined flag */
    for (i = 0; i < numBBs; i++)
    {
        pBB = dfsLast[i];
        if (pBB->flg & INVALID_BB)
            continue; /* Do not process invalid BBs */

        for (useAt = pBB->rbegin2(); useAt != pBB->rend2(); useAt++)
        {
            if ((useAt->type == LOW_LEVEL) &&
                    (useAt->invalid == FALSE) &&
                    (use = useAt->ic.ll.flagDU.u))
            {
                /* Find definition within the same basic block */
                for (defAt = useAt+1; defAt != pBB->rend2(); defAt++)
                {
                    def = defAt->ic.ll.flagDU.d;
                    if ((use & def) == use)
                    {
                        notSup = FALSE;
                        if ((useAt->GetLlOpcode() >= iJB) && (useAt->GetLlOpcode() <= iJNS))
                        {
                            switch (defAt->GetLlOpcode())
                            {
                            case iCMP:
                                rhs = srcIdent (*defAt, this, defAt->loc_ip,*useAt, eUSE);
                                lhs = dstIdent (*defAt, this, defAt->loc_ip,*useAt, eUSE);
                                break;

                            case iOR:
                                lhs = defAt->ic.hl.oper.asgn.lhs->clone();
                                useAt->copyDU(*defAt, eUSE, eDEF);
                                if (defAt->isLlFlag(B))
                                    rhs = COND_EXPR::idKte (0, 1);
                                else
                                    rhs = COND_EXPR::idKte (0, 2);
                                break;

                            case iTEST:
                                rhs = srcIdent (*defAt,this, defAt->loc_ip,*useAt, eUSE);
                                lhs = dstIdent (*defAt,this, defAt->loc_ip,*useAt, eUSE);
                                lhs = COND_EXPR::boolOp (lhs, rhs, AND);
                                if (defAt->isLlFlag(B))
                                    rhs = COND_EXPR::idKte (0, 1);
                                else
                                    rhs = COND_EXPR::idKte (0, 2);
                                break;

                            default:
                                notSup = TRUE;
                                std::cout << hex<<defAt->loc_ip;
                                reportError (JX_NOT_DEF, defAt->GetLlOpcode());
                                flg |= PROC_ASM;		/* generate asm */
                            }
                            if (! notSup)
                            {
                                exp = COND_EXPR::boolOp (lhs, rhs,condOpJCond[useAt->GetLlOpcode()-iJB]);
                                useAt->setJCond(exp);
                            }
                        }

                        else if (useAt->GetLlOpcode() == iJCXZ)
                        {
                            lhs = COND_EXPR::idReg (rCX, 0, &localId);
                            useAt->setRegDU (rCX, eUSE);
                            rhs = COND_EXPR::idKte (0, 2);
                            exp = COND_EXPR::boolOp (lhs, rhs, EQUAL);
                            useAt->setJCond(exp);
                        }

                        else
                        {
                            reportError (NOT_DEF_USE,defAt->GetLlOpcode(),useAt->GetLlOpcode());
                            flg |= PROC_ASM;		/* generate asm */
                        }
                        break;
                    }
                }

                /* Check for extended basic block */
                if ((pBB->size() == 1) &&(useAt->GetLlOpcode() >= iJB) && (useAt->GetLlOpcode() <= iJNS))
                {
                    ICODE & prev(pBB->back()); /* For extended basic blocks - previous icode inst */
                    if (prev.ic.hl.opcode == HLI_JCOND)
                    {
                        exp = prev.ic.hl.oper.exp->clone();
                        exp->changeBoolOp (condOpJCond[useAt->GetLlOpcode()-iJB]);
                        useAt->copyDU(prev, eUSE, eUSE);
                        useAt->setJCond(exp);
                    }
                }
                /* Error - definition not found for use of a cond code */
                else if (defAt == pBB->rend2())
                {
                    reportError(DEF_NOT_FOUND,useAt->GetLlOpcode());
                    //fatalError (DEF_NOT_FOUND, Icode.GetLlOpcode(useAt-1));
                }
            }
        }
    }
}


/* Generates the LiveUse() and Def() sets for each basic block in the graph.
 * Note: these sets are constant and could have been constructed during
 *       the construction of the graph, but since the code hasn't been
 *       analyzed yet for idioms, the procedure preamble misleads the
 *       analysis (eg: push si, would include si in LiveUse; although it
 *       is not really meant to be a register that is used before defined). */
void Function::genLiveKtes ()
{
    Int i;
    BB * pbb;
    dword liveUse, def;

    for (i = 0; i < numBBs; i++)
    {
        liveUse = def = 0;
        pbb = dfsLast[i];
        if (pbb->flg & INVALID_BB)
            continue;	/* skip invalid BBs */
        for (auto j = pbb->begin2(); j != pbb->end2(); j++)
        {
            if ((j->type == HIGH_LEVEL) && (j->invalid == FALSE))
            {
                liveUse |= (j->du.use & ~def);
                def |= j->du.def;
            }
        }
        pbb->liveUse = liveUse;
        pbb->def = def;
    }
}


/* Generates the liveIn() and liveOut() sets for each basic block via an
 * iterative approach.
 * Propagates register usage information to the procedure call. */
void Function::liveRegAnalysis (dword in_liveOut)
{
    Int i, j;
    BB * pbb=0;              /* pointer to current basic block   */
    Function * pcallee;        /* invoked subroutine               */
    ICODE  *ticode        /* icode that invokes a subroutine  */
            ;
    dword prevLiveOut,	/* previous live out 				*/
            prevLiveIn;		/* previous live in					*/
    boolT change;			/* is there change in the live sets?*/

    /* liveOut for this procedure */
    liveOut = in_liveOut;

    change = TRUE;
    while (change)
    {
        /* Process nodes in reverse postorder order */
        change = FALSE;
        for (i = numBBs; i > 0; i--)
        {
            pbb = dfsLast[i-1];
            if (pbb->flg & INVALID_BB)		/* Do not process invalid BBs */
                continue;

            /* Get current liveIn() and liveOut() sets */
            prevLiveIn = pbb->liveIn;
            prevLiveOut = pbb->liveOut;

            /* liveOut(b) = U LiveIn(s); where s is successor(b)
             * liveOut(b) = {liveOut}; when b is a HLI_RET node     */
            if (pbb->edges.empty())      /* HLI_RET node         */
            {
                pbb->liveOut = in_liveOut;

                /* Get return expression of function */
                if (flg & PROC_IS_FUNC)
                {
                    auto picode = pbb->rbegin2(); /* icode of function return */
                    if (picode->ic.hl.opcode == HLI_RET)
                    {
                        picode->ic.hl.oper.exp = COND_EXPR::idID (&retVal, &localId, pbb->back().loc_ip);
                        picode->du.use = in_liveOut;
                    }
                }
            }
            else                            /* Check successors */
            {
                for (j = 0; j < pbb->edges.size(); j++)
                    pbb->liveOut |= pbb->edges[j].BBptr->liveIn;

                /* propagate to invoked procedure */
                if (pbb->nodeType == CALL_NODE)
                {
                    ICODE &ticode(pbb->back());
                    pcallee = ticode.ic.hl.oper.call.proc;

                    /* user/runtime routine */
                    if (! (pcallee->flg & PROC_ISLIB))
                    {
                        if (pcallee->liveAnal == FALSE) /* hasn't been processed */
                            pcallee->dataFlow (pbb->liveOut);
                        pbb->liveOut = pcallee->liveIn;
                    }
                    else    /* library routine */
                    {
                        if ((pcallee->flg & PROC_IS_FUNC) && /* returns a value */
                                (pcallee->liveOut & pbb->edges[0].BBptr->liveIn))
                            pbb->liveOut = pcallee->liveOut;
                        else
                            pbb->liveOut = 0;
                    }

                    if ((! (pcallee->flg & PROC_ISLIB)) || (pbb->liveOut != 0))
                    {
                        switch (pcallee->retVal.type) {
                        case TYPE_LONG_SIGN: case TYPE_LONG_UNSIGN:
                            ticode.du1.numRegsDef = 2;
                            break;
                        case TYPE_WORD_SIGN: case TYPE_WORD_UNSIGN:
                        case TYPE_BYTE_SIGN: case TYPE_BYTE_UNSIGN:
                            ticode.du1.numRegsDef = 1;
                            break;
                        } /*eos*/

                        /* Propagate def/use results to calling icode */
                        ticode.du.use = pcallee->liveIn;
                        ticode.du.def = pcallee->liveOut;
                    }
                }
            }

            /* liveIn(b) = liveUse(b) U (liveOut(b) - def(b) */
            pbb->liveIn = pbb->liveUse | (pbb->liveOut & ~pbb->def);

            /* Check if live sets have been modified */
            if ((prevLiveIn != pbb->liveIn) || (prevLiveOut != pbb->liveOut))
                change = TRUE;
        }
    }

    /* Propagate liveIn(b) to procedure header */
    if (pbb->liveIn != 0)   /* uses registers */
        liveIn = pbb->liveIn;

    /* Remove any references to register variables */
    if (flg & SI_REGVAR)
    {
        liveIn &= maskDuReg[rSI];
        pbb->liveIn &= maskDuReg[rSI];
    }
    if (flg & DI_REGVAR)
    {
        liveIn &= maskDuReg[rDI];
        pbb->liveIn &= maskDuReg[rDI];
    }
}


/* Generates the du chain of each instruction in a basic block */
void Function::genDU1 ()
{
    byte regi;            /* Register that was defined */
    Int i,  k, defRegIdx, useIdx;
    iICODE picode, ticode,lastInst;/* Current and target bb    */
    BB * pbb, *tbb;         /* Current and target basic block */
    boolT res;
    COND_EXPR *exp, *lhs;

    /* Traverse tree in dfsLast order */
    assert(dfsLast.size()==numBBs);
    for (i = 0; i < numBBs; i++)
    {
        pbb = dfsLast[i];
        if (pbb->flg & INVALID_BB)	continue;

        /* Process each register definition of a HIGH_LEVEL icode instruction.
         * Note that register variables should not be considered registers.
         */
        lastInst = pbb->end2();
        for (picode = pbb->begin2(); picode != lastInst; picode++)
        {
            if (picode->type == HIGH_LEVEL)
            {
                regi = 0;
                defRegIdx = 0;
                for (k = 0; k < INDEXBASE; k++)
                {
                    if ((picode->du.def & power2(k)) == 0)
                        continue;
                    regi = (byte)(k + 1);       /* defined register */
                    picode->du1.regi[defRegIdx] = regi;

                    /* Check remaining instructions of the BB for all uses
                     * of register regi, before any definitions of the
                     * register */
                    if ((regi == rDI) && (flg & DI_REGVAR))
                        continue;
                    if ((regi == rSI) && (flg & SI_REGVAR))
                        continue;
                    if ((picode + 1) != lastInst)		/* several instructions */
                    {
                        useIdx = 0;
                        for (auto ricode = picode + 1; ricode != lastInst; ricode++)
                        {
                            ticode=ricode;
                            /* Only check uses of HIGH_LEVEL icodes */
                            if (ricode->type == HIGH_LEVEL)
                            {
                                /* if used, get icode index */
                                if (ricode->du.use & duReg[regi])
                                    picode->du1.idx[defRegIdx][useIdx++] = ricode->loc_ip;

                                /* if defined, stop finding uses for this reg */
                                if (ricode->du.def & duReg[regi])
                                    break;
                            }
                        }

                        /* Check if last definition of this register */
                        if ((! (ticode->du.def & duReg[regi])) && (pbb->liveOut & duReg[regi]))
                            picode->du.lastDefRegi |= duReg[regi];
                    }
                    else		/* only 1 instruction in this basic block */
                    {
                        /* Check if last definition of this register */
                        if (pbb->liveOut & duReg[regi])
                            picode->du.lastDefRegi |= duReg[regi];
                    }

                    /* Find target icode for HLI_CALL icodes to procedures
                                     * that are functions.  The target icode is in the
                                     * next basic block (unoptimized code) or somewhere else
                                     * on optimized code. */
                    if ((picode->ic.hl.opcode == HLI_CALL) &&
                            (picode->ic.hl.oper.call.proc->flg & PROC_IS_FUNC))
                    {
                        tbb = pbb->edges[0].BBptr;
                        useIdx = 0;
                        for (ticode = tbb->begin2(); ticode != tbb->end2(); ticode++)
                        {
                            if (ticode->type == HIGH_LEVEL)
                            {
                                /* if used, get icode index */
                                if (ticode->du.use & duReg[regi])
                                    picode->du1.idx[defRegIdx][useIdx++] = ticode->loc_ip;

                                /* if defined, stop finding uses for this reg */
                                if (ticode->du.def & duReg[regi])
                                    break;
                            }
                        }

                        /* if not used in this basic block, check if the
                         * register is live out, if so, make it the last
                         * definition of this register */
                        if ((picode->du1.idx[defRegIdx][useIdx] == 0) &&
                                (tbb->liveOut & duReg[regi]))
                            picode->du.lastDefRegi |= duReg[regi];
                    }

                    /* If not used within this bb or in successors of this
                     * bb (ie. not in liveOut), then register is useless,
                     * thus remove it.  Also check that this is not a return
                     * from a library function (routines such as printf
                     * return an integer, which is normally not taken into
                     * account by the programmer). 	*/
                    if ((picode->invalid == FALSE) &&
                            (picode->du1.idx[defRegIdx][0] == 0) &&
                            (! (picode->du.lastDefRegi & duReg[regi])) &&
                            //						(! ((picode->ic.hl.opcode != HLI_CALL) &&
                            (! ((picode->ic.hl.opcode == HLI_CALL) &&
                                (picode->ic.hl.oper.call.proc->flg & PROC_ISLIB))))
                    {
                        if (! (pbb->liveOut & duReg[regi]))	/* not liveOut */
                        {
                            res = picode->removeDefRegi (regi, defRegIdx+1,&localId);

                            /* Backpatch any uses of this instruction, within
                             * the same BB, if the instruction was invalidated */
                            if (res == TRUE)
                                for (auto ticode = riICODE(picode); ticode != pbb->rend2(); ticode++)
                                {
                                    for (int n = 0; n < MAX_USES; n++)
                                    {
                                        if (ticode->du1.idx[0][n] == picode->loc_ip)
                                        {
                                            if (n < MAX_USES - 1)
                                            {
                                                memmove (&ticode->du1.idx[0][n],
                                                         &ticode->du1.idx[0][n+1],
                                                         (size_t)((MAX_USES - n - 1) * sizeof(Int)));
                                                n--;
                                            }
                                            ticode->du1.idx[0][MAX_USES - 1] = 0;
                                        }
                                    }
                                }
                        }
                        else		/* liveOut */
                            picode->du.lastDefRegi |= duReg[regi];
                    }
                    defRegIdx++;

                    /* Check if all defined registers have been processed */
                    if ((defRegIdx >= picode->du1.numRegsDef) ||
                            (defRegIdx == MAX_REGS_DEF))
                        break;
                }
            }
        }
    }

}


/* Substitutes the rhs (or lhs if rhs not possible) of ticode for the rhs
 * of picode. */
static void forwardSubs (COND_EXPR *lhs, COND_EXPR *rhs, ICODE * picode,
                         ICODE * ticode, LOCAL_ID *locsym, Int *numHlIcodes)
{
    boolT res;

    if (rhs == NULL)        /* In case expression popped is NULL */
        return;

    /* Insert on rhs of ticode, if possible */
    res = insertSubTreeReg (rhs, &ticode->ic.hl.oper.asgn.rhs,
                            locsym->id_arr[lhs->expr.ident.idNode.regiIdx].id.regi,
                            locsym);
    if (res)
    {
        picode->invalidate();
        (*numHlIcodes)--;
    }
    else
    {
        /* Try to insert it on lhs of ticode*/
        res = insertSubTreeReg (rhs, &ticode->ic.hl.oper.asgn.lhs,
                                locsym->id_arr[lhs->expr.ident.idNode.regiIdx].id.regi,
                                locsym);
        if (res)
        {
            picode->invalidate();
            (*numHlIcodes)--;
        }
    }
}


/* Substitutes the rhs (or lhs if rhs not possible) of ticode for the
 * expression exp given */
static void forwardSubsLong (Int longIdx, COND_EXPR *exp, ICODE * picode,
                             ICODE * ticode, Int *numHlIcodes)
{ boolT res;

    if (exp == NULL)        /* In case expression popped is NULL */
        return;

    /* Insert on rhs of ticode, if possible */
    res = insertSubTreeLongReg (exp, &ticode->ic.hl.oper.asgn.rhs, longIdx);
    if (res)
    {
        picode->invalidate();
        (*numHlIcodes)--;
    }
    else
    {
        /* Try to insert it on lhs of ticode*/
        res = insertSubTreeLongReg (exp, &ticode->ic.hl.oper.asgn.lhs, longIdx);
        if (res)
        {
            picode->invalidate();
            (*numHlIcodes)--;
        }
    }
}


/* Returns whether the elements of the expression rhs are all x-clear from
 * instruction f up to instruction t.	*/
static boolT xClear (COND_EXPR *rhs, iICODE f, Int t, iICODE lastBBinst, Function * pproc)
{
    iICODE i;
    boolT res;
    byte regi;
    ICODE * picode;

    if (rhs == NULL)
        return false;

    switch (rhs->type) {
    case IDENTIFIER:
        if (rhs->expr.ident.idType == REGISTER)
        {
            picode = &pproc->Icode.front();
            regi= pproc->localId.id_arr[rhs->expr.ident.idNode.regiIdx].id.regi;
            for (i = (f + 1); (i != lastBBinst) && (i->loc_ip < t); i++)
                if ((i->type == HIGH_LEVEL) && (i->invalid == FALSE))
                {
                    if (i->du.def & duReg[regi])
                        return false;
                }
            if (i != lastBBinst)
                return true;
            return false;
        }
        else
            return true;
        /* else if (rhs->expr.ident.idType == LONG_VAR)
                        {
                            missing all other identifiers ****
                        } */

    case BOOLEAN_OP:
        res = xClear (rhs->expr.boolExpr.rhs, f, t, lastBBinst, pproc);
        if (res == FALSE)
            return false;
        return (xClear (rhs->expr.boolExpr.lhs, f, t, lastBBinst, pproc));

    case NEGATION:
    case ADDRESSOF:
    case DEREFERENCE:
        return (xClear (rhs->expr.unaryExp, f, t, lastBBinst, pproc));
    } /* eos */
    return false;
}


/* Checks the type of the formal argument as against to the actual argument,
 * whenever possible, and then places the actual argument on the procedure's
 * argument list.	*/
static void processCArg (Function * pp, Function * pProc, ICODE * picode, Int numArgs, Int *k)
{
    COND_EXPR *exp;
    boolT res;

    /* if (numArgs == 0)
                return; */

    exp = g_exp_stk.pop();
    if (pp->flg & PROC_ISLIB) /* library function */
    {
        if (pp->args.numArgs > 0)
            if (pp->flg & PROC_VARARG)
            {
                if (numArgs < pp->args.sym.size())
                    adjustActArgType (exp, pp->args.sym[numArgs].type, pProc);
            }
            else
                adjustActArgType (exp, pp->args.sym[numArgs].type, pProc);
        res = newStkArg (picode, exp, picode->ic.ll.opcode, pProc);
    }
    else			/* user function */
    {
        if (pp->args.numArgs > 0)
            pp->args.adjustForArgType (numArgs, expType (exp, pProc));
        res = newStkArg (picode, exp, picode->ic.ll.opcode, pProc);
    }

    /* Do not update the size of k if the expression was a segment register
         * in a near call */
    if (res == FALSE)
        *k += hlTypeSize (exp, pProc);
}

/* Eliminates extraneous intermediate icode instructions when finding
 * expressions.  Generates new hlIcodes in the form of expression trees.
 * For HLI_CALL hlIcodes, places the arguments in the argument list.    */
void Function::findExps()
{
    Int i, k, numHlIcodes;
    iICODE lastInst,
            picode, /* Current icode                            */
            ticode;        /* Target icode                             */
    BB * pbb;         /* Current and next basic block             */
    boolT res;
    COND_EXPR *exp,       /* expression pointer - for HLI_POP and HLI_CALL    */
            *lhs;		/* exp ptr for return value of a HLI_CALL		*/
    STKFRAME * args;       /* pointer to arguments - for HLI_CALL          */
    byte regi, regi2;		/* register(s) to be forward substituted	*/
    ID *retVal;			/* function return value 					*/

    /* Initialize expression stack */
    g_exp_stk.init();

    /* Traverse tree in dfsLast order */
    for (i = 0; i < numBBs; i++)
    {
        /* Process one BB */
        pbb = dfsLast[i];
        if (pbb->flg & INVALID_BB)
            continue;
        lastInst = pbb->end2();
        numHlIcodes = 0;
        for (picode = pbb->begin2(); picode != lastInst; picode++)
        {
            if ((picode->type == HIGH_LEVEL) && (picode->invalid == FALSE))
            {
                numHlIcodes++;
                if (picode->du1.numRegsDef == 1)    /* byte/word regs */
                {
                    /* Check for only one use of this register.  If this is
                     * the last definition of the register in this BB, check
                     * that it is not liveOut from this basic block */
                    if ((picode->du1.idx[0][0] != 0) &&
                            (picode->du1.idx[0][1] == 0))
                    {
                        /* Check that this register is not liveOut, if it
                         * is the last definition of the register */
                        regi = picode->du1.regi[0];

                        /* Check if we can forward substitute this register */
                        switch (picode->ic.hl.opcode) {
                        case HLI_ASSIGN:
                            /* Replace rhs of current icode into target
                             * icode expression */
                            ticode = Icode.begin()+picode->du1.idx[0][0];
                            if ((picode->du.lastDefRegi & duReg[regi]) &&
                                    ((ticode->ic.hl.opcode != HLI_CALL) &&
                                     (ticode->ic.hl.opcode != HLI_RET)))
                                continue;

                            if (xClear (picode->ic.hl.oper.asgn.rhs, picode,
                                        picode->du1.idx[0][0],  lastInst, this))
                            {
                                switch (ticode->ic.hl.opcode) {
                                case HLI_ASSIGN:
                                    forwardSubs (picode->ic.hl.oper.asgn.lhs,
                                                 picode->ic.hl.oper.asgn.rhs,
                                                 &(*picode), &(*ticode), &localId,
                                                 &numHlIcodes);
                                    break;

                                case HLI_JCOND: case HLI_PUSH: case HLI_RET:
                                    res = insertSubTreeReg (
                                                picode->ic.hl.oper.asgn.rhs,
                                                &ticode->ic.hl.oper.exp,
                                                localId.id_arr[picode->ic.hl.oper.asgn.lhs->expr.ident.idNode.regiIdx].id.regi,
                                                &localId);
                                    if (res)
                                    {
                                        picode->invalidate();
                                        numHlIcodes--;
                                    }
                                    break;

                                case HLI_CALL:    /* register arguments */
                                    newRegArg (&(*picode), &(*ticode));
                                    picode->invalidate();
                                    numHlIcodes--;
                                    break;
                                } /* eos */
                            }
                            break;

                        case HLI_POP:
                            ticode = Icode.begin()+(picode->du1.idx[0][0]);
                            if ((picode->du.lastDefRegi & duReg[regi]) &&
                                    ((ticode->ic.hl.opcode != HLI_CALL) &&
                                     (ticode->ic.hl.opcode != HLI_RET)))
                                continue;

                            exp = g_exp_stk.pop();  /* pop last exp pushed */
                            switch (ticode->ic.hl.opcode) {
                            case HLI_ASSIGN:
                                forwardSubs (picode->ic.hl.oper.exp, exp,
                                             &(*picode), &(*ticode), &localId,
                                             &numHlIcodes);
                                break;

                            case HLI_JCOND: case HLI_PUSH: case HLI_RET:
                                res = insertSubTreeReg (exp,
                                                        &ticode->ic.hl.oper.exp,
                                                        localId.id_arr[picode->ic.hl.oper.exp->expr.ident.idNode.regiIdx].id.regi,
                                                        &localId);
                                if (res)
                                {
                                    picode->invalidate();
                                    numHlIcodes--;
                                }
                                break;

                                /****case HLI_CALL:    /* register arguments
                                newRegArg (pProc, picode, ticode);
                                picode->invalidate();
                                numHlIcodes--;
                                break;	*/
                            } /* eos */
                            break;

                        case HLI_CALL:
                            ticode = Icode.begin()+(picode->du1.idx[0][0]);
                            switch (ticode->ic.hl.opcode) {
                            case HLI_ASSIGN:
                                exp = COND_EXPR::idFunc (
                                            picode->ic.hl.oper.call.proc,
                                            picode->ic.hl.oper.call.args);
                                res = insertSubTreeReg (exp,
                                                        &ticode->ic.hl.oper.asgn.rhs,
                                                        picode->ic.hl.oper.call.proc->retVal.id.regi,
                                                        &localId);
                                if (! res)
                                    insertSubTreeReg (exp,
                                                      &ticode->ic.hl.oper.asgn.lhs,
                                                      picode->ic.hl.oper.call.proc->retVal.id.regi,
                                                      &localId);
                                /***  HERE missing: 2 regs ****/
                                picode->invalidate();
                                numHlIcodes--;
                                break;

                            case HLI_PUSH: case HLI_RET:
                                exp = COND_EXPR::idFunc (
                                            picode->ic.hl.oper.call.proc,
                                            picode->ic.hl.oper.call.args);
                                ticode->ic.hl.oper.exp = exp;
                                picode->invalidate();
                                numHlIcodes--;
                                break;

                            case HLI_JCOND:
                                exp = COND_EXPR::idFunc (
                                            picode->ic.hl.oper.call.proc,
                                            picode->ic.hl.oper.call.args);
                                retVal = &picode->ic.hl.oper.call.proc->retVal,
                                        res = insertSubTreeReg (exp,
                                                                &ticode->ic.hl.oper.exp,
                                                                retVal->id.regi, &localId);
                                if (res)	/* was substituted */
                                {
                                    picode->invalidate();
                                    numHlIcodes--;
                                }
                                else	/* cannot substitute function */
                                {
                                    lhs = COND_EXPR::idID(retVal,&localId,picode->loc_ip);
                                    picode->setAsgn(lhs, exp);
                                }
                                break;
                            } /* eos */
                            break;
                        } /* eos */
                    }
                }

                else if (picode->du1.numRegsDef == 2)   /* long regs */
                {
                    /* Check for only one use of these registers */
                    if ((picode->du1.idx[0][0] != 0) &&
                            (picode->du1.idx[0][1] == 0) &&
                            (picode->du1.idx[1][0] != 0) &&
                            (picode->du1.idx[1][1] == 0))
                    {
                        switch (picode->ic.hl.opcode) {
                        case HLI_ASSIGN:
                            /* Replace rhs of current icode into target
                             * icode expression */
                            if (picode->du1.idx[0][0] == picode->du1.idx[1][0])
                            {
                                ticode = Icode.begin()+(picode->du1.idx[0][0]);
                                if ((picode->du.lastDefRegi & duReg[regi]) &&
                                        ((ticode->ic.hl.opcode != HLI_CALL) &&
                                         (ticode->ic.hl.opcode != HLI_RET)))
                                    continue;

                                switch (ticode->ic.hl.opcode) {
                                case HLI_ASSIGN:
                                    forwardSubsLong (picode->ic.hl.oper.asgn.lhs->expr.ident.idNode.longIdx,
                                                     picode->ic.hl.oper.asgn.rhs, &(*picode),
                                                     &(*ticode), &numHlIcodes);
                                    break;

                                case HLI_JCOND:  case HLI_PUSH:  case HLI_RET:
                                    res = insertSubTreeLongReg (
                                                picode->ic.hl.oper.asgn.rhs,
                                                &ticode->ic.hl.oper.exp,
                                                picode->ic.hl.oper.asgn.lhs->expr.ident.idNode.longIdx);
                                    if (res)
                                    {
                                        picode->invalidate();
                                        numHlIcodes--;
                                    }
                                    break;

                                case HLI_CALL:    /* register arguments */
                                    newRegArg ( &(*picode), &(*ticode));
                                    picode->invalidate();
                                    numHlIcodes--;
                                    break;
                                } /* eos */
                            }
                            break;

                        case HLI_POP:
                            if (picode->du1.idx[0][0] == picode->du1.idx[1][0])
                            {
                                ticode = Icode.begin()+(picode->du1.idx[0][0]);
                                if ((picode->du.lastDefRegi & duReg[regi]) &&
                                        ((ticode->ic.hl.opcode != HLI_CALL) &&
                                         (ticode->ic.hl.opcode != HLI_RET)))
                                    continue;

                                exp = g_exp_stk.pop(); /* pop last exp pushed */
                                switch (ticode->ic.hl.opcode) {
                                case HLI_ASSIGN:
                                    forwardSubsLong (picode->ic.hl.oper.exp->expr.ident.idNode.longIdx,
                                                     exp, &(*picode), &(*ticode), &numHlIcodes);
                                    break;
                                case HLI_JCOND: case HLI_PUSH:
                                    res = insertSubTreeLongReg (exp,
                                                                &ticode->ic.hl.oper.exp,
                                                                picode->ic.hl.oper.asgn.lhs->expr.ident.idNode.longIdx);
                                    if (res)
                                    {
                                        picode->invalidate();
                                        numHlIcodes--;
                                    }
                                    break;
                                case HLI_CALL:	/*** missing ***/
                                    break;
                                } /* eos */
                            }
                            break;

                        case HLI_CALL:    /* check for function return */
                            ticode = Icode.begin()+(picode->du1.idx[0][0]);
                            switch (ticode->ic.hl.opcode)
                            {
                            case HLI_ASSIGN:
                                exp = COND_EXPR::idFunc (
                                            picode->ic.hl.oper.call.proc,
                                            picode->ic.hl.oper.call.args);
                                ticode->ic.hl.oper.asgn.lhs =
                                        COND_EXPR::idLong(&localId, DST, ticode,
                                                          HIGH_FIRST, picode->loc_ip, eDEF, 1);
                                ticode->ic.hl.oper.asgn.rhs = exp;
                                picode->invalidate();
                                numHlIcodes--;
                                break;

                            case HLI_PUSH:  case HLI_RET:
                                exp = COND_EXPR::idFunc (
                                            picode->ic.hl.oper.call.proc,
                                            picode->ic.hl.oper.call.args);
                                ticode->ic.hl.oper.exp = exp;
                                picode->invalidate();
                                numHlIcodes--;
                                break;

                            case HLI_JCOND:
                                exp = COND_EXPR::idFunc (
                                            picode->ic.hl.oper.call.proc,
                                            picode->ic.hl.oper.call.args);
                                retVal = &picode->ic.hl.oper.call.proc->retVal;
                                res = insertSubTreeLongReg (exp,
                                                            &ticode->ic.hl.oper.exp,
                                                            localId.newLongReg
                                                            (
                                                                retVal->type, retVal->id.longId.h,
                                                                retVal->id.longId.l, picode->loc_ip));
                                if (res)	/* was substituted */
                                {
                                    picode->invalidate();
                                    numHlIcodes--;
                                }
                                else	/* cannot substitute function */
                                {
                                    lhs = COND_EXPR::idID(retVal,&localId,picode->loc_ip);
                                    picode->setAsgn(lhs, exp);
                                }
                                break;
                            } /* eos */
                        } /* eos */
                    }
                }

                /* HLI_PUSH doesn't define any registers, only uses registers.
                 * Push the associated expression to the register on the local
                 * expression stack */
                else if (picode->ic.hl.opcode == HLI_PUSH)
                {
                    g_exp_stk.push(picode->ic.hl.oper.exp);
                    picode->invalidate();
                    numHlIcodes--;
                }

                /* For HLI_CALL instructions that use arguments from the stack,
                                 * pop them from the expression stack and place them on the
                                 * procedure's argument list */
                if ((picode->ic.hl.opcode == HLI_CALL) &&
                        ! (picode->ic.hl.oper.call.proc->flg & REG_ARGS))
                { Function * pp;
                    Int cb, numArgs;
                    boolT res;

                    pp = picode->ic.hl.oper.call.proc;
                    if (pp->flg & CALL_PASCAL)
                    {
                        cb = pp->cbParam;	/* fixed # arguments */
                        for (k = 0, numArgs = 0; k < cb; numArgs++)
                        {
                            exp = g_exp_stk.pop();
                            if (pp->flg & PROC_ISLIB)	/* library function */
                            {
                                if (pp->args.numArgs > 0)
                                    adjustActArgType(exp, pp->args.sym[numArgs].type, this);
                                res = newStkArg (&(*picode), exp, picode->ic.ll.opcode, this);
                            }
                            else			/* user function */
                            {
                                if (pp->args.numArgs >0)
                                    pp->args.adjustForArgType (numArgs,expType (exp, this));
                                res = newStkArg (&(*picode), exp,picode->ic.ll.opcode, this);
                            }
                            if (res == FALSE)
                                k += hlTypeSize (exp, this);
                        }
                    }
                    else		/* CALL_C */
                    {
                        cb = picode->ic.hl.oper.call.args->cb;
                        numArgs = 0;
                        if (cb)
                            for (k = 0; k < cb; numArgs++)
                                processCArg (pp, this, &(*picode), numArgs, &k);
                        else if ((cb == 0) && (picode->ic.ll.flg & REST_STK))
                            while (! g_exp_stk.empty())
                            {
                                processCArg (pp, this, &(*picode), numArgs, &k);
                                numArgs++;
                            }
                    }
                }

                /* If we could not substitute the result of a function,
                                 * assign it to the corresponding registers */
                if ((picode->ic.hl.opcode == HLI_CALL) &&
                        ((picode->ic.hl.oper.call.proc->flg & PROC_ISLIB) !=
                         PROC_ISLIB) && (picode->du1.idx[0][0] == 0) &&
                        (picode->du1.numRegsDef > 0))
                {
                    exp = COND_EXPR::idFunc (picode->ic.hl.oper.call.proc,
                                             picode->ic.hl.oper.call.args);
                    lhs = COND_EXPR::idID (&picode->ic.hl.oper.call.proc->retVal,
                                           &localId, picode->loc_ip);
                    picode->setAsgn(lhs, exp);
                }
            }
        }

        /* Store number of high-level icodes in current basic block */
        pbb->numHlIcodes = numHlIcodes;
    }
}


/* Invokes procedures related with data flow analysis.  Works on a procedure
 * at a time basis.
 * Note: indirect recursion in liveRegAnalysis is possible. */
void Function::dataFlow(dword liveOut)
{
    boolT isAx, isBx, isCx, isDx;
    Int idx;

    /* Remove references to register variables */
    if (flg & SI_REGVAR)
        liveOut &= maskDuReg[rSI];
    if (flg & DI_REGVAR)
        liveOut &= maskDuReg[rDI];

    /* Function - return value register(s) */
    if (liveOut != 0)
    {
        flg |= PROC_IS_FUNC;
        isAx = (boolT)(liveOut & power2(rAX - rAX));
        isBx = (boolT)(liveOut & power2(rBX - rAX));
        isCx = (boolT)(liveOut & power2(rCX - rAX));
        isDx = (boolT)(liveOut & power2(rDX - rAX));

        if (isAx && isDx)       /* long or pointer */
        {
            retVal.type = TYPE_LONG_SIGN;
            retVal.loc = REG_FRAME;
            retVal.id.longId.h = rDX;
            retVal.id.longId.l = rAX;
            idx = localId.newLongReg(TYPE_LONG_SIGN, rDX, rAX, 0);
            localId.propLongId (rAX, rDX, "\0");
        }
        else if (isAx || isBx || isCx || isDx)	/* word */
        {
            retVal.type = TYPE_WORD_SIGN;
            retVal.loc = REG_FRAME;
            if (isAx)
                retVal.id.regi = rAX;
            else if (isBx)
                retVal.id.regi = rBX;
            else if (isCx)
                retVal.id.regi = rCX;
            else
                retVal.id.regi = rDX;
            idx = localId.newByteWordReg(TYPE_WORD_SIGN,retVal.id.regi);
        }
    }

    /* Data flow analysis */
    liveAnal = TRUE;
    elimCondCodes();
    genLiveKtes();
    liveRegAnalysis (liveOut);   /* calls dataFlow() recursively */
    if (! (flg & PROC_ASM))		/* can generate C for pProc		*/
    {
        genDU1 ();			/* generate def/use level 1 chain */
        findExps (); 		/* forward substitution algorithm */
    }
}

