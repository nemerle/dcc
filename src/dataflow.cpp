/*****************************************************************************
 * Project: dcc
 * File:    dataflow.c
 * Purpose: Data flow analysis module.
 * (C) Cristina Cifuentes
 ****************************************************************************/

#include "dcc.h"
#include <boost/range.hpp>
#include <boost/range/adaptors.hpp>
#include <boost/range/algorithm.hpp>
#include <boost/assign.hpp>
#include <string.h>
#include <iostream>
#include <iomanip>
#include <stdio.h>
using namespace boost;
using namespace boost::adaptors;
struct ExpStack
{
    typedef std::list<COND_EXPR *> EXP_STK;
    EXP_STK expStk;      /* local expression stack */

    void        init();
    void        push(COND_EXPR *);
    COND_EXPR * pop();
    int         numElem();
    boolT	empty();
    void processExpPush(int &numHlIcodes, iICODE picode)
    {
        push(picode->hl()->expr());
        picode->invalidate();
        numHlIcodes--;
    }

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
int ExpStack::numElem()
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
size_t STKFRAME::getLocVar(int off)
{
    auto iter=findByLabel(off);
    return distance(begin(),iter);
}


/* Returns a string with the source operand of Icode */
static COND_EXPR *srcIdent (const LLInst &ll_insn, Function * pProc, iICODE i, ICODE & duIcode, operDu du)
{
    if (ll_insn.testFlags(I))   /* immediate operand */
    {
        if (ll_insn.testFlags(B))
            return COND_EXPR::idKte (ll_insn.src.op(), 1);
        return COND_EXPR::idKte (ll_insn.src.op(), 2);
    }
    // otherwise
    return COND_EXPR::id (ll_insn, SRC, pProc, i, duIcode, du);
}


/* Returns the destination operand */
static COND_EXPR *dstIdent (const LLInst & ll_insn, Function * pProc, iICODE i, ICODE & duIcode, operDu du)
{
    COND_EXPR *n;
    n = COND_EXPR::id (ll_insn, DST, pProc, i, duIcode, du);
    /** Is it needed? (pIcode->ll()->flg) & NO_SRC_B **/
    return (n);
}
/* Eliminates all condition codes and generates new hlIcode instructions */
void Function::elimCondCodes ()
{
    int i;

    uint8_t use;           /* Used flags bit vector                  */
    uint8_t def;           /* Defined flags bit vector               */
    boolT notSup;       /* Use/def combination not supported      */
    COND_EXPR *rhs;     /* Source operand                         */
    COND_EXPR *lhs;     /* Destination operand                    */
    COND_EXPR *_expr;   /* Boolean expression                     */
    BB * pBB;           /* Pointer to BBs in dfs last ordering    */
    riICODE useAt;      /* Instruction that used flag    */
    riICODE defAt;      /* Instruction that defined flag */
        //lhs=rhs=_expr=0;
    for (i = 0; i < numBBs; i++)
    {
        pBB = m_dfsLast[i];
        if (pBB->flg & INVALID_BB)
            continue; /* Do not process invalid BBs */
        //        auto v(pBB | boost::adaptors::reversed);
        //        for (const ICODE &useAt : v)
        //        {}
        for (useAt = pBB->rbegin(); useAt != pBB->rend(); useAt++)
        {
            llIcode useAtOp = llIcode(useAt->ll()->getOpcode());
            use = useAt->ll()->flagDU.u;
            if ((useAt->type != LOW_LEVEL) || ( ! useAt->valid() ) || ( 0 == use ))
                continue;
            /* Find definition within the same basic block */
            defAt=useAt;
            ++defAt;
            for (; defAt != pBB->rend(); defAt++)
            {
                def = defAt->ll()->flagDU.d;
                if ((use & def) != use)
                    continue;
                notSup = false;
                if ((useAtOp >= iJB) && (useAtOp <= iJNS))
                {
                    iICODE befDefAt = (++riICODE(defAt)).base();
                    switch (defAt->ll()->getOpcode())
                    {
                        case iCMP:
                            rhs = srcIdent (*defAt->ll(), this, befDefAt,*useAt, eUSE);
                            lhs = dstIdent (*defAt->ll(), this, befDefAt,*useAt, eUSE);
                            break;

                        case iOR:
                            lhs = defAt->hl()->asgn.lhs->clone();
                            useAt->copyDU(*defAt, eUSE, eDEF);
                            if (defAt->ll()->testFlags(B))
                                rhs = COND_EXPR::idKte (0, 1);
                            else
                                rhs = COND_EXPR::idKte (0, 2);
                            break;

                        case iTEST:
                            rhs = srcIdent (*defAt->ll(),this, befDefAt,*useAt, eUSE);
                            lhs = dstIdent (*defAt->ll(),this, befDefAt,*useAt, eUSE);
                            lhs = COND_EXPR::boolOp (lhs, rhs, AND);
                            if (defAt->ll()->testFlags(B))
                                rhs = COND_EXPR::idKte (0, 1);
                            else
                                rhs = COND_EXPR::idKte (0, 2);
                            break;

                        default:
                            notSup = true;
                            std::cout << hex<<defAt->loc_ip;
                            reportError (JX_NOT_DEF, defAt->ll()->getOpcode());
                            flg |= PROC_ASM;		/* generate asm */
                    }
                    if (! notSup)
                    {
                        assert(lhs);
                        assert(rhs);
                        _expr = COND_EXPR::boolOp (lhs, rhs,condOpJCond[useAtOp-iJB]);
                        useAt->setJCond(_expr);
                    }
                }

                else if (useAtOp == iJCXZ)
                {
                    lhs = COND_EXPR::idReg (rCX, 0, &localId);
                    useAt->setRegDU (rCX, eUSE);
                    rhs = COND_EXPR::idKte (0, 2);
                    _expr = COND_EXPR::boolOp (lhs, rhs, EQUAL);
                    useAt->setJCond(_expr);
                }
                //                    else if (useAt->getOpcode() == iRCL)
                //                    {
                //                    }
                else
                {
                    ICODE &a(*defAt);
                    ICODE &b(*useAt);
                    reportError (NOT_DEF_USE,a.ll()->getOpcode(),b.ll()->getOpcode());
                    flg |= PROC_ASM;		/* generate asm */
                }
                break;
            }

            /* Check for extended basic block */
            if ((pBB->size() == 1) &&(useAtOp >= iJB) && (useAtOp <= iJNS))
            {
                ICODE & _prev(pBB->back()); /* For extended basic blocks - previous icode inst */
                if (_prev.hl()->opcode == HLI_JCOND)
                {
                    _expr = _prev.hl()->expr()->clone();
                    _expr->changeBoolOp (condOpJCond[useAtOp-iJB]);
                    useAt->copyDU(_prev, eUSE, eUSE);
                    useAt->setJCond(_expr);
                }
            }
            /* Error - definition not found for use of a cond code */
            else if (defAt == pBB->rend())
            {
                reportError(DEF_NOT_FOUND,useAtOp);
                //fatalError (DEF_NOT_FOUND, Icode.getOpcode(useAt-1));
            }
        }
    }
}


/** Generates the LiveUse() and Def() sets for each basic block in the graph.
 * Note: these sets are constant and could have been constructed during
 *       the construction of the graph, but since the code hasn't been
 *       analyzed yet for idioms, the procedure preamble misleads the
 *       analysis (eg: push si, would include si in LiveUse; although it
 *       is not really meant to be a register that is used before defined). */
void Function::genLiveKtes ()
{
    int i;
    BB * pbb;
    bitset<32> liveUse, def;

    for (i = 0; i < numBBs; i++)
    {
        liveUse.reset();
        def.reset();
        pbb = m_dfsLast[i];
        if (pbb->flg & INVALID_BB)
            continue;	// skip invalid BBs
        for(ICODE &insn : *pbb)
        {
            if ((insn.type == HIGH_LEVEL) && ( insn.valid() ))
            {
                liveUse |= (insn.du.use & ~def);
                def |= insn.du.def;
            }
        }
        pbb->liveUse = liveUse;
        pbb->def = def;
    }
}


/* Generates the liveIn() and liveOut() sets for each basic block via an
 * iterative approach.
 * Propagates register usage information to the procedure call. */
void Function::liveRegAnalysis (std::bitset<32> &in_liveOut)
{
    using namespace boost::adaptors;
    using namespace boost::assign;
    BB * pbb=0;              /* pointer to current basic block   */
    Function * pcallee;        /* invoked subroutine               */
    //ICODE  *ticode        /* icode that invokes a subroutine  */
    ;
    std::bitset<32> prevLiveOut,	/* previous live out 				*/
            prevLiveIn;		/* previous live in					*/
    boolT change;			/* is there change in the live sets?*/

    /* liveOut for this procedure */
    liveOut = in_liveOut;

    change = true;
    while (change)
    {
        /* Process nodes in reverse postorder order */
        change = false;
        auto valid_reversed_bbs = (m_dfsLast | reversed | filtered(BB::ValidFunctor()) );
        for( BB * _pbb : valid_reversed_bbs)
        {
            pbb = _pbb;//*iBB;//m_dfsLast[i-1];

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
                    auto picode = pbb->rbegin(); /* icode of function return */
                    if (picode->hl()->opcode == HLI_RET)
                    {
                        picode->hl()->expr(COND_EXPR::idID (&retVal, &localId, (++pbb->rbegin()).base()));
                        picode->du.use = in_liveOut;
                    }
                }
            }
            else                            /* Check successors */
            {
                for(TYPEADR_TYPE &e : pbb->edges)
                {
                    pbb->liveOut |= e.BBptr->liveIn;
                }

                /* propagate to invoked procedure */
                if (pbb->nodeType == CALL_NODE)
                {
                    ICODE &ticode(pbb->back());
                    pcallee = ticode.hl()->call.proc;

                    /* user/runtime routine */
                    if (! (pcallee->flg & PROC_ISLIB))
                    {
                        if (pcallee->liveAnal == false) /* hasn't been processed */
                            pcallee->dataFlow (pbb->liveOut);
                        pbb->liveOut = pcallee->liveIn;
                    }
                    else    /* library routine */
                    {
                        if ( (pcallee->flg & PROC_IS_FUNC) && /* returns a value */
                             (pcallee->liveOut & pbb->edges[0].BBptr->liveIn).any()
                             )
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
                        default:
                            ticode.du1.numRegsDef = 0;
                            //fprintf(stderr,"Function::liveRegAnalysis : Unknown return type %d, assume 0\n",pcallee->retVal.type);
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
                change = true;
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

void BB::genDU1()
{
    eReg regi;            /* Register that was defined */
    int k, defRegIdx, useIdx;
    iICODE picode, ticode,lastInst;
    BB *tbb;         /* Target basic block */
    bool res;
    //COND_EXPR *e
    /* Process each register definition of a HIGH_LEVEL icode instruction.
     * Note that register variables should not be considered registers.
     */
    assert(0!=Parent);
    lastInst = this->end();
    ICODE::TypeFilter<HIGH_LEVEL> zq;
    auto all_high_levels =  instructions | filtered(zq);
    printf("\n");
    for (auto picode=all_high_levels.begin(); picode!=all_high_levels.end(); ++picode)
    {
//        if (picode->type != HIGH_LEVEL)
//            continue;
        regi = rUNDEF;
        defRegIdx = 0;
        // foreach defined register
        bitset<32> processed=0;
        for (k = 0; k < INDEX_BX_SI; k++)
        {
            if (not picode->du.def.test(k))
                continue;
            //printf("Processing reg")
            processed |= duReg[k];
            regi = (eReg)(k + 1);       /* defined register */
            picode->du1.regi[defRegIdx] = regi;

            /* Check remaining instructions of the BB for all uses
             * of register regi, before any definitions of the
             * register */
            if ((regi == rDI) && (flg & DI_REGVAR))
                continue;
            if ((regi == rSI) && (flg & SI_REGVAR))
                continue;
            if (distance(picode,all_high_levels.end())>1) /* several instructions */
            {
                useIdx = 0;
                for (auto ricode = ++iICODE(picode.base()); ricode != lastInst; ricode++)
                {
                    ticode=ricode;
                    if (ricode->type != HIGH_LEVEL) // Only check uses of HIGH_LEVEL icodes
                        continue;
                    /* if used, get icode index */
                    if ((ricode->du.use & duReg[regi]).any())
                        picode->du1.recordUse(defRegIdx,ricode);
                    /* if defined, stop finding uses for this reg */
                    if ((ricode->du.def & duReg[regi]).any())
                        break;
                }
                /* Check if last definition of this register */
                if ((not (ticode->du.def & duReg[regi]).any()) and (this->liveOut & duReg[regi]).any())
                    picode->du.lastDefRegi |= duReg[regi];
            }
            else		/* only 1 instruction in this basic block */
            {
                /* Check if last definition of this register */
                if ((this->liveOut & duReg[regi]).any())
                    picode->du.lastDefRegi |= duReg[regi];
            }

            /* Find target icode for HLI_CALL icodes to procedures
             * that are functions.  The target icode is in the
             * next basic block (unoptimized code) or somewhere else
             * on optimized code. */
            if ((picode->hl()->opcode == HLI_CALL) &&
                    (picode->hl()->call.proc->flg & PROC_IS_FUNC))
            {
                tbb = this->edges[0].BBptr;
                for (ticode = tbb->begin(); ticode != tbb->end(); ticode++)
                {
                    if (ticode->type != HIGH_LEVEL)
                        continue;
                    /* if used, get icode index */
                    if ((ticode->du.use & duReg[regi]).any())
                        picode->du1.recordUse(defRegIdx,ticode);
                    /* if defined, stop finding uses for this reg */
                    if ((ticode->du.def & duReg[regi]).any())
                        break;
                }

                /* if not used in this basic block, check if the
                 * register is live out, if so, make it the last
                 * definition of this register */
                if ( picode->du1.used(defRegIdx) && (tbb->liveOut & duReg[regi]).any())
                    picode->du.lastDefRegi |= duReg[regi];
            }

            /* If not used within this bb or in successors of this
             * bb (ie. not in liveOut), then register is useless,
             * thus remove it.  Also check that this is not a return
             * from a library function (routines such as printf
             * return an integer, which is normally not taken into
             * account by the programmer). 	*/
            if (picode->valid() and not picode->du1.used(defRegIdx) and
                    (not (picode->du.lastDefRegi & duReg[regi]).any()) &&
                    (not ((picode->hl()->opcode == HLI_CALL) &&
                          (picode->hl()->call.proc->flg & PROC_ISLIB))))
            {
                if (! (this->liveOut & duReg[regi]).any())	/* not liveOut */
                {
                    res = picode->removeDefRegi (regi, defRegIdx+1,&Parent->localId);
                    if (res != true)
                    {
                        defRegIdx++;
                        continue;
                    }

                    /* Backpatch any uses of this instruction, within
                     * the same BB, if the instruction was invalidated */
                    for (auto back_patch_at = riICODE(picode.base()); back_patch_at != rend(); back_patch_at++)
                    {
                        back_patch_at->du1.remove(0,picode.base());
                    }
                }
                else		/* liveOut */
                    picode->du.lastDefRegi |= duReg[regi];
            }
            defRegIdx++;

            /* Check if all defined registers have been processed */
            if ((defRegIdx >= picode->du1.numRegsDef) || (defRegIdx == MAX_REGS_DEF))
                break;
        }
    }
}
/* Generates the du chain of each instruction in a basic block */
void Function::genDU1 ()
{
    /* Traverse tree in dfsLast order */
    assert(m_dfsLast.size()==numBBs);
    for(BB *pbb : m_dfsLast)
    {
        if (pbb->flg & INVALID_BB)
            continue;
        pbb->genDU1();
    }

}


/* Substitutes the rhs (or lhs if rhs not possible) of ticode for the rhs
 * of picode. */
void LOCAL_ID::forwardSubs (COND_EXPR *lhs, COND_EXPR *rhs, iICODE picode, iICODE ticode, int &numHlIcodes) const
{
    bool res;

    if (rhs == NULL)        /* In case expression popped is NULL */
        return;

    /* Insert on rhs of ticode, if possible */
    res = COND_EXPR::insertSubTreeReg (ticode->hl()->asgn.rhs,rhs, id_arr[lhs->expr.ident.idNode.regiIdx].id.regi, this);
    if (res)
    {
        picode->invalidate();
        numHlIcodes--;
    }
    else
    {
        /* Try to insert it on lhs of ticode*/
        res = COND_EXPR::insertSubTreeReg (ticode->hl()->asgn.lhs,rhs, id_arr[lhs->expr.ident.idNode.regiIdx].id.regi, this);
        if (res)
        {
            picode->invalidate();
            numHlIcodes--;
        }
    }
}


/* Substitutes the rhs (or lhs if rhs not possible) of ticode for the
 * expression exp given */
static void forwardSubsLong (int longIdx, COND_EXPR *_exp, iICODE picode, iICODE ticode, int *numHlIcodes)
{
    bool res;

    if (_exp == NULL)        /* In case expression popped is NULL */
        return;

    /* Insert on rhs of ticode, if possible */
    res = COND_EXPR::insertSubTreeLongReg (_exp, &ticode->hl()->asgn.rhs, longIdx);
    if (res)
    {
        picode->invalidate();
        (*numHlIcodes)--;
    }
    else
    {
        /* Try to insert it on lhs of ticode*/
        res = COND_EXPR::insertSubTreeLongReg (_exp, &ticode->hl()->asgn.lhs, longIdx);
        if (res)
        {
            picode->invalidate();
            (*numHlIcodes)--;
        }
    }
}


/* Returns whether the elements of the expression rhs are all x-clear from
 * instruction f up to instruction t.	*/
bool COND_EXPR::xClear (rICODE range_to_check, iICODE lastBBinst, const LOCAL_ID & locId)
{
    iICODE i;
    boolT res;
    uint8_t regi;

    switch (m_type)
    {
    case IDENTIFIER:
        if (expr.ident.idType == REGISTER)
        {
            regi= locId.id_arr[expr.ident.idNode.regiIdx].id.regi;
            range_to_check.advance_begin(1);
            auto all_valid_and_high_level_after_start = range_to_check | filtered(ICODE::TypeAndValidFilter<HIGH_LEVEL>());
            for (ICODE &i : all_valid_and_high_level_after_start)
                if ((i.du.def & duReg[regi]).any())
                    return false;
            if (all_valid_and_high_level_after_start.end().base() != lastBBinst)
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
        if(0==rhs())
            return false;
        res = rhs()->xClear ( range_to_check, lastBBinst, locId);
        if (res == false)
            return false;
        if(0==lhs())
            return false;
        return lhs()->xClear ( range_to_check, lastBBinst, locId);

    case NEGATION:
    case ADDRESSOF:
    case DEREFERENCE:
        if(0==expr.unaryExp)
            return false;
        return expr.unaryExp->xClear ( range_to_check, lastBBinst, locId);
    } /* eos */
    return false;
}
bool UnaryOperator::xClear(rICODE range_to_check, iICODE lastBBinst, const LOCAL_ID &locs)
{
    if(0==unaryExp)
        return false;
    return unaryExp->xClear ( range_to_check, lastBBinst, locs);
}

bool BinaryOperator::xClear(rICODE range_to_check, iICODE lastBBinst, const LOCAL_ID &locs)
{
    if(0==m_rhs)
        return false;
    if ( ! m_rhs->xClear (range_to_check, lastBBinst, locs) )
        return false;
    if(0==m_lhs)
        return false;
    return m_lhs->xClear (range_to_check, lastBBinst, locs);
}
/* Checks the type of the formal argument as against to the actual argument,
 * whenever possible, and then places the actual argument on the procedure's
 * argument list.	*/
/// @returns the type size of the stored Arg
static int processCArg (Function * pp, Function * pProc, ICODE * picode, int numArgs)
{
    COND_EXPR *_exp;
    bool res;

    /* if (numArgs == 0)
                return; */

    _exp = g_exp_stk.pop();
    if (pp->flg & PROC_ISLIB) /* library function */
    {
        if (pp->args.numArgs > 0)
            if (pp->flg & PROC_VARARG)
            {
                if (numArgs < pp->args.size())
                    adjustActArgType (_exp, pp->args[numArgs].type, pProc);
            }
            else
                adjustActArgType (_exp, pp->args[numArgs].type, pProc);
    }
    else			/* user function */
    {
        if (pp->args.numArgs > 0)
        {
            if(_exp==NULL)
            {
                fprintf(stderr,"Would try to adjustForArgType with null _exp\n");
            }
            else
                pp->args.adjustForArgType (numArgs, _exp->expType (pProc));
        }
    }
    res = picode->newStkArg (_exp, (llIcode)picode->ll()->getOpcode(), pProc);

    /* Do not update the size of k if the expression was a segment register
         * in a near call */
    if (res == false)
        return hlTypeSize (_exp, pProc);
    return 0; // be default we do not know the size of the argument
}

/** Eliminates extraneous intermediate icode instructions when finding
 * expressions.  Generates new hlIcodes in the form of expression trees.
 * For HLI_CALL hlIcodes, places the arguments in the argument list.    */
void LOCAL_ID::processTargetIcode(iICODE picode, int &numHlIcodes, iICODE ticode,bool isLong) const
{
    boolT res;
    HLTYPE &p_hl(*picode->hl());
    HLTYPE &t_hl(*ticode->hl());
    switch (t_hl.opcode)
    {
    case HLI_ASSIGN:
        if(isLong)
        {
            forwardSubsLong (p_hl.asgn.lhs->expr.ident.idNode.longIdx,
                             p_hl.asgn.rhs, picode,ticode,
                             &numHlIcodes);
        }
        else
            this->forwardSubs (p_hl.asgn.lhs, p_hl.asgn.rhs, picode, ticode, numHlIcodes);
        break;

    case HLI_JCOND:  case HLI_PUSH:  case HLI_RET:
        if(isLong)
        {
            res = COND_EXPR::insertSubTreeLongReg (
                        p_hl.asgn.rhs,
                        &t_hl.exp.v,
                        p_hl.asgn.lhs->expr.ident.idNode.longIdx);
        }
        else
        {
            res = COND_EXPR::insertSubTreeReg (
                        t_hl.exp.v,
                        p_hl.asgn.rhs,
                        id_arr[p_hl.asgn.lhs->expr.ident.idNode.regiIdx].id.regi,
                        this);
        }
        if (res)
        {
            picode->invalidate();
            numHlIcodes--;
        }
        break;

    case HLI_CALL:    /* register arguments */
        newRegArg ( picode, ticode);
        picode->invalidate();
        numHlIcodes--;
        break;
    }
}

void Function::processHliCall(COND_EXPR *_exp, iICODE picode)
{
    Function * pp;
    int cb, numArgs;
    boolT res;
    int k;
    pp = picode->hl()->call.proc;
    if (pp->flg & CALL_PASCAL)
    {
        cb = pp->cbParam;	/* fixed # arguments */
        k = 0;
        numArgs = 0;
        while(k<cb)
        {
            _exp = g_exp_stk.pop();
            if (pp->flg & PROC_ISLIB)	/* library function */
            {
                if (pp->args.numArgs > 0)
                    adjustActArgType(_exp, pp->args[numArgs].type, this);
                res = picode->newStkArg (_exp, (llIcode)picode->ll()->getOpcode(), this);
            }
            else			/* user function */
            {
                if (pp->args.numArgs >0)
                {
                    if(_exp==NULL)
                    {
                        fprintf(stderr,"Would try to adjustForArgType with null _exp\n");
                    }
                    pp->args.adjustForArgType (numArgs,_exp->expType (this));
                }
                res = picode->newStkArg (_exp,(llIcode)picode->ll()->getOpcode(), this);
            }
            if (res == false)
                k += hlTypeSize (_exp, this);
            numArgs++;
        }
    }
    else		/* CALL_C */
    {
        cb = picode->hl()->call.args->cb;
        numArgs = 0;
        k = 0;
        if (cb)
        {
            while ( k < cb )
            {
                k+=processCArg (pp, this, &(*picode), numArgs);
                numArgs++;
            }
        }
        else if ((cb == 0) && picode->ll()->testFlags(REST_STK))
        {
            while (! g_exp_stk.empty())
            {
                k+=processCArg (pp, this, &(*picode), numArgs);
                numArgs++;
            }
        }
    }
}


int BB::findBBExps(LOCAL_ID &locals,Function *fnc)
{
    bool res;

    ID *_retVal;         // function return value
    COND_EXPR *_exp,     // expression pointer - for HLI_POP and HLI_CALL    */
            *lhs;	// exp ptr for return value of a HLI_CALL		*/
    iICODE ticode;     // Target icode                             */
    HLTYPE *ti_hl=0;
    uint8_t regi;
    numHlIcodes = 0;
    // register(s) to be forward substituted	*/
    auto valid_and_highlevel = instructions | filtered(ICODE::TypeAndValidFilter<HIGH_LEVEL>());
    for (auto picode = valid_and_highlevel.begin(); picode != valid_and_highlevel.end(); picode++)
    {
//        if ((picode->type != HIGH_LEVEL) || ( ! picode->valid() ))
//            continue;
        HLTYPE &_icHl(*picode->hl());
        numHlIcodes++;
        if (picode->du1.numRegsDef == 1)    /* uint8_t/uint16_t regs */
        {
            /* Check for only one use of this register.  If this is
             * the last definition of the register in this BB, check
             * that it is not liveOut from this basic block */
            if (picode->du1.numUses(0)==1)
            {
                /* Check that this register is not liveOut, if it
                 * is the last definition of the register */
                regi = picode->du1.regi[0];

                /* Check if we can forward substitute this register */
                switch (_icHl.opcode)
                {
                    case HLI_ASSIGN:
                        /* Replace rhs of current icode into target
                         * icode expression */

                        ticode = picode->du1.idx[0].uses.front();
                        if ((picode->du.lastDefRegi & duReg[regi]).any() &&
                                ((ticode->hl()->opcode != HLI_CALL) &&
                                 (ticode->hl()->opcode != HLI_RET)))
                            continue;

                        if (_icHl.asgn.rhs->xClear (make_iterator_range(picode.base(),picode->du1.idx[0].uses[0]),
                                                    end(), locals))
                        {
                            locals.processTargetIcode(picode.base(), numHlIcodes, ticode,false);
                        }
                        break;

                    case HLI_POP:
                        ticode = picode->du1.idx[0].uses.front();
                        ti_hl = ticode->hl();
                        if ((picode->du.lastDefRegi & duReg[regi]).any() &&
                                ((ti_hl->opcode != HLI_CALL) &&
                                 (ti_hl->opcode != HLI_RET)))
                            continue;

                        _exp = g_exp_stk.pop();  /* pop last exp pushed */
                        switch (ticode->hl()->opcode) {
                            case HLI_ASSIGN:
                                locals.forwardSubs(_icHl.expr(), _exp, picode.base(), ticode, numHlIcodes);
                                break;

                            case HLI_JCOND: case HLI_PUSH: case HLI_RET:
                                res = COND_EXPR::insertSubTreeReg (ti_hl->exp.v,
                                                                   _exp,
                                                                   locals.id_arr[_icHl.expr()->expr.ident.idNode.regiIdx].id.regi,
                                                                   &locals);
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
                        ticode = picode->du1.idx[0].uses.front();
                        ti_hl = ticode->hl();
                        _retVal = &_icHl.call.proc->retVal;
                        switch (ti_hl->opcode)
                        {
                            case HLI_ASSIGN:
                                assert(ti_hl->asgn.rhs);
                                _exp = _icHl.call.toId();
                                res = COND_EXPR::insertSubTreeReg (ti_hl->asgn.rhs,_exp, _retVal->id.regi, &locals);
                                if (! res)
                                    COND_EXPR::insertSubTreeReg (ti_hl->asgn.lhs, _exp,_retVal->id.regi, &locals);
                                //TODO: HERE missing: 2 regs
                                picode->invalidate();
                                numHlIcodes--;
                                break;

                            case HLI_PUSH: case HLI_RET:
                                ti_hl->expr( _icHl.call.toId() );
                                picode->invalidate();
                                numHlIcodes--;
                                break;

                            case HLI_JCOND:
                                _exp = _icHl.call.toId();
                                res = COND_EXPR::insertSubTreeReg (ti_hl->exp.v, _exp, _retVal->id.regi, &locals);
                                if (res)	/* was substituted */
                                {
                                    picode->invalidate();
                                    numHlIcodes--;
                                }
                                else	/* cannot substitute function */
                                {
                                    //picode->loc_ip
                                    lhs = COND_EXPR::idID(_retVal,&locals,picode.base());
                                    picode->setAsgn(lhs, _exp);
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
            if ((picode->du1.numUses(0) == 1) and (picode->du1.numUses(1) == 1))
            {
                regi = picode->du1.regi[0]; //TODO: verify that regi actually should be assigned this

                switch (_icHl.opcode)
                {
                    case HLI_ASSIGN:
                        /* Replace rhs of current icode into target
                         * icode expression */
                        if (picode->du1.idx[0].uses[0] == picode->du1.idx[1].uses[0])
                        {
                            ticode = picode->du1.idx[0].uses.front();
                            if ((picode->du.lastDefRegi & duReg[regi]).any() &&
                                    ((ticode->hl()->opcode != HLI_CALL) &&
                                     (ticode->hl()->opcode != HLI_RET)))
                                continue;
                            locals.processTargetIcode(picode.base(), numHlIcodes, ticode,true);
                        }
                        break;

                    case HLI_POP:
                        if (picode->du1.idx[0].uses[0] == picode->du1.idx[1].uses[0])
                        {
                            ticode = picode->du1.idx[0].uses.front();
                            if ((picode->du.lastDefRegi & duReg[regi]).any() &&
                                    ((ticode->hl()->opcode != HLI_CALL) &&
                                     (ticode->hl()->opcode != HLI_RET)))
                                continue;

                            _exp = g_exp_stk.pop(); /* pop last exp pushed */
                            switch (ticode->hl()->opcode) {
                                case HLI_ASSIGN:
                                    forwardSubsLong (_icHl.expr()->expr.ident.idNode.longIdx,
                                                     _exp, picode.base(), ticode, &numHlIcodes);
                                    break;
                                case HLI_JCOND: case HLI_PUSH:
                                    res = COND_EXPR::insertSubTreeLongReg (_exp,
                                                                           &ticode->hl()->exp.v,
                                                                           _icHl.asgn.lhs->expr.ident.idNode.longIdx);
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
                        ticode = picode->du1.idx[0].uses.front();
                        switch (ticode->hl()->opcode)
                        {
                            case HLI_ASSIGN:
                                _exp = _icHl.call.toId();
                                ticode->hl()->asgn.lhs =
                                        COND_EXPR::idLong(&locals, DST, ticode,HIGH_FIRST, picode.base(), eDEF, ++iICODE(ticode));
                                ticode->hl()->asgn.rhs = _exp;
                                picode->invalidate();
                                numHlIcodes--;
                                break;

                            case HLI_PUSH:  case HLI_RET:
                                ticode->hl()->expr( _icHl.call.toId() );
                                picode->invalidate();
                                numHlIcodes--;
                                break;

                            case HLI_JCOND:
                                _exp = _icHl.call.toId();
                                _retVal = &picode->hl()->call.proc->retVal;
                                res = COND_EXPR::insertSubTreeLongReg (_exp,
                                                                       &ticode->hl()->exp.v,
                                                                       locals.newLongReg ( _retVal->type, _retVal->id.longId.h,
                                                                                            _retVal->id.longId.l, picode.base()));
                                if (res)	/* was substituted */
                                {
                                    picode->invalidate();
                                    numHlIcodes--;
                                }
                                else	/* cannot substitute function */
                                {
                                    lhs = locals.createId(_retVal,picode.base());
                                    picode->setAsgn(lhs, _exp);
                                }
                                break;
                        } /* eos */
                } /* eos */
            }
        }
        /* HLI_PUSH doesn't define any registers, only uses registers.
         * Push the associated expression to the register on the local
         * expression stack */
        else if (_icHl.opcode == HLI_PUSH)
        {
            g_exp_stk.processExpPush(numHlIcodes, picode.base());
        }
        else if(picode->du1.numRegsDef!=0)
            printf("Num def %d\n",picode->du1.numRegsDef);

        /* For HLI_CALL instructions that use arguments from the stack,
         * pop them from the expression stack and place them on the
         * procedure's argument list */
        if(_icHl.opcode == HLI_CALL)
        {
            if ( not _icHl.call.proc->hasRegArgs())
            {
                fnc->processHliCall(_exp, picode.base());
            }

            /* If we could not substitute the result of a function,
             * assign it to the corresponding registers */
            if ( not _icHl.call.proc->isLibrary() and (not picode->du1.used(0)) and (picode->du1.numRegsDef > 0))
            {
                _exp = COND_EXPR::idFunc (_icHl.call.proc, _icHl.call.args);
                lhs = COND_EXPR::idID (&_icHl.call.proc->retVal, &locals, picode.base());
                picode->setAsgn(lhs, _exp);
            }
        }
    }
    /* Store number of high-level icodes in current basic block */

}

void Function::findExps()
{
    //int i, numHlIcodes;
    //STKFRAME * args;  // pointer to arguments - for HLI_CALL          */

    /* Initialize expression stack */
    g_exp_stk.init();

    /* Traverse tree in dfsLast order */
//    for (i = 0; i < numBBs; i++)
    for(BB *pbb : m_dfsLast)
    {
        /* Process one BB */
//        pbb = m_dfsLast[i];
        if (not pbb->valid())
            continue;
        pbb->findBBExps( this->localId, this);

    }
}


/** Invokes procedures related with data flow analysis.  Works on a procedure
 * at a time basis.
 * Note: indirect recursion in liveRegAnalysis is possible. */
void Function::dataFlow(std::bitset<32> &_liveOut)
{
    bool isAx, isBx, isCx, isDx;
    int idx;

    /* Remove references to register variables */
    if (flg & SI_REGVAR)
        _liveOut &= maskDuReg[rSI];
    if (flg & DI_REGVAR)
        _liveOut &= maskDuReg[rDI];

    /* Function - return value register(s) */
    if (_liveOut.any())
    {
        flg |= PROC_IS_FUNC;
        isAx = _liveOut.test(rAX - rAX);
        isBx = _liveOut.test(rBX - rAX);
        isCx = _liveOut.test(rCX - rAX);
        isDx = _liveOut.test(rDX - rAX);
        bool isAL = !isAx && _liveOut.test(rAL - rAX);
        bool isAH = !isAx && _liveOut.test(rAH - rAX);
        bool isBL = !isBx && _liveOut.test(rBL - rAX);
        bool isBH = !isBx && _liveOut.test(rBH - rAX);
        bool isCL = !isCx && _liveOut.test(rCL - rAX);
        bool isCH = !isCx && _liveOut.test(rCH - rAX);
        bool isDL = !isDx && _liveOut.test(rDL - rAX);
        bool isDH = !isDx && _liveOut.test(rDH - rAX);
        if(isAL && isAH)
        {
            isAx = true;
            isAH=isAL=false;
        }
        if(isDL && isDH)
        {
            isDx = true;
            isDH=isDL=false;
        }
        if(isBL && isBH)
        {
            isBx = true;
            isBH=isBL=false;
        }
        if(isCL && isCH)
        {
            isCx = true;
            isCH=isCL=false;
        }

        if (isAx && isDx)       /* long or pointer */
        {
            retVal.type = TYPE_LONG_SIGN;
            retVal.loc = REG_FRAME;
            retVal.id.longId.h = rDX;
            retVal.id.longId.l = rAX;
            idx = localId.newLongReg(TYPE_LONG_SIGN, rDX, rAX, Icode.begin()/*0*/);
            localId.propLongId (rAX, rDX, "\0");
        }
        else if (isAx || isBx || isCx || isDx)	/* uint16_t */
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
        else if(isAL||isBL||isCL||isDL)
        {
            retVal.type = TYPE_BYTE_SIGN;
            retVal.loc = REG_FRAME;
            if (isAL)
                retVal.id.regi = rAL;
            else if (isBL)
                retVal.id.regi = rBL;
            else if (isCL)
                retVal.id.regi = rCL;
            else
                retVal.id.regi = rDL;
            idx = localId.newByteWordReg(TYPE_BYTE_SIGN,retVal.id.regi);

        }
    }

    /* Data flow analysis */
    liveAnal = true;
    elimCondCodes();
    genLiveKtes();
    liveRegAnalysis (_liveOut);   /* calls dataFlow() recursively */
    if (! (flg & PROC_ASM))		/* can generate C for pProc		*/
    {
        genDU1 ();			/* generate def/use level 1 chain */
        findExps (); 		/* forward substitution algorithm */
    }
}

