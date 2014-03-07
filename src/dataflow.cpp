/*****************************************************************************
 * Project: dcc
 * File:    dataflow.c
 * Purpose: Data flow analysis module.
 * (C) Cristina Cifuentes
 ****************************************************************************/
#include <stdint.h>
#include <cstring>
#include <iostream>
#include <iomanip>
#include <cstdio>
#include <boost/range.hpp>
#include <boost/range/adaptors.hpp>
#include <boost/range/algorithm.hpp>
#include <boost/assign.hpp>

#include "dcc.h"
#include "project.h"
using namespace boost;
using namespace boost::adaptors;
struct ExpStack
{
    Function *func;
    typedef std::list<Expr *> EXP_STK;
    EXP_STK expStk;      /* local expression stack */

    void        init(Function *f);
    void        push(Expr *);
    Expr *      pop();
    Expr *      top() const {
        if(!expStk.empty())
            return expStk.back();
        return nullptr;
    }
    int         numElem();
    bool       empty();
    void processExpPush(int &numHlIcodes, ICODE &picode)
    {
        RegisterNode *rn = dynamic_cast<RegisterNode *>(picode.hlU()->expr());
        if(rn)
            assert(rn->m_syms==&func->localId);
        push(picode.hlU()->expr());
        picode.invalidate();
        numHlIcodes--;
    }

};
/***************************************************************************
 * Expression stack functions
 **************************************************************************/

/* Reinitalizes the expression stack (expStk) to NULL, by freeing all the
 * space allocated (if any).        */
void ExpStack::init(Function *f)
{
    func=f;
    expStk.clear();
}

/* Pushes the given expression onto the local stack (expStk). */
void ExpStack::push(Expr *expr)
{
    expStk.push_back(expr);
}


/* Returns the element on the top of the local expression stack (expStk),
 * and deallocates the space allocated by this node.
 * If there are no elements on the stack, returns NULL. */
Expr *ExpStack::pop()
{
    if(expStk.empty())
        return nullptr;
    Expr *topExp = expStk.back();
    expStk.pop_back();
    return topExp;
}

/* Returns the number of elements available in the expression stack */
int ExpStack::numElem()
{
    return expStk.size();
}

/* Returns whether the expression stack is empty or not */
bool ExpStack::empty()
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
static Expr *srcIdent (const LLInst &ll_insn, Function * pProc, iICODE i, ICODE & duIcode, operDu du)
{
    const LLOperand * src_op = ll_insn.get(SRC);
    if (src_op->isImmediate())   /* immediate operand ll_insn.testFlags(I)*/
    {
        //if (ll_insn.testFlags(B))
        return new Constant(src_op->getImm2(), src_op->byteWidth());
    }
    // otherwise
    return AstIdent::id (ll_insn, SRC, pProc, i, duIcode, du);
}


/* Returns the destination operand */
static Expr *dstIdent (const LLInst & ll_insn, Function * pProc, iICODE i, ICODE & duIcode, operDu du)
{
    Expr *n;
    n = AstIdent::id (ll_insn, DST, pProc, i, duIcode, du);
    /** Is it needed? (pIcode->ll()->flg) & NO_SRC_B **/
    return (n);
}
/* Eliminates all condition codes and generates new hlIcode instructions */
void Function::elimCondCodes ()
{
    //    int i;

    uint8_t use;           /* Used flags bit vector                  */
    uint8_t def;           /* Defined flags bit vector               */
    bool notSup;       /* Use/def combination not supported      */
    Expr *rhs;     /* Source operand                         */
    Expr *lhs;     /* Destination operand                    */
    BinaryOperator *_expr;   /* Boolean expression                     */
    //BB * pBB;           /* Pointer to BBs in dfs last ordering    */
    riICODE useAt;      /* Instruction that used flag    */
    riICODE defAt;      /* Instruction that defined flag */
    //lhs=rhs=_expr=0;
    auto valid_reversed_bbs = (m_dfsLast | reversed | filtered(BB::ValidFunctor()) );
    for( BB * pBB : valid_reversed_bbs)
    {
        //auto reversed_instructions = pBB->range() | reversed;
        for (useAt = pBB->rbegin(); useAt != pBB->rend(); useAt++)
        {
            ICODE &useIcode(*useAt);
            llIcode useAtOp = llIcode(useAt->ll()->getOpcode());
            use = useAt->ll()->flagDU.u;
            if ((useAt->type != LOW_LEVEL) || ( ! useAt->valid() ) || ( 0 == use ))
                continue;
            /* Find definition within the same basic block */
            defAt=useAt;
            ++defAt;
            for (; defAt != pBB->rend(); defAt++)
            {
                ICODE &defIcode(*defAt);
                def = defAt->ll()->flagDU.d;
                if ((use & def) != use)
                    continue;
                notSup = false;
                LLOperand *dest_ll = defIcode.ll()->get(DST);
                LLOperand *src_ll = defIcode.ll()->get(SRC);
                if ((useAtOp >= iJB) && (useAtOp <= iJNS))
                {
                    iICODE befDefAt = (++riICODE(defAt)).base();
                    switch (defIcode.ll()->getOpcode())
                    {
                    case iCMP:
                        rhs = srcIdent (*defIcode.ll(), this, befDefAt,*useAt, eUSE);
                        lhs = dstIdent (*defIcode.ll(), this, befDefAt,*useAt, eUSE);
                        break;

                    case iOR:
                        lhs = defIcode.hl()->asgn.lhs()->clone();
                        useAt->copyDU(*defAt, eUSE, eDEF);
                        //if (defAt->ll()->testFlags(B))
                        rhs = new Constant(0, dest_ll->byteWidth());
                        break;

                    case iTEST:
                        rhs = srcIdent (*defIcode.ll(),this, befDefAt,*useAt, eUSE);
                        lhs = dstIdent (*defIcode.ll(),this, befDefAt,*useAt, eUSE);
                        lhs = BinaryOperator::And(lhs, rhs);
                        //                            if (defAt->ll()->testFlags(B))
                        rhs = new Constant(0, dest_ll->byteWidth());
                        break;
                    case iINC:
                    case iDEC: //WARNING: verbatim copy from iOR needs fixing ?
                        lhs = defIcode.hl()->asgn.lhs()->clone();
                        useAt->copyDU(*defAt, eUSE, eDEF);
                        rhs = new Constant(0, dest_ll->byteWidth());
                        break;
                    default:
                        notSup = true;
                        std::cout << hex<<defIcode.loc_ip;
                        reportError (JX_NOT_DEF, defIcode.ll()->getOpcode());
                        flg |= PROC_ASM;		/* generate asm */
                    }
                    if (! notSup)
                    {
                        assert(lhs);
                        assert(rhs);
                        _expr = BinaryOperator::Create(condOpJCond[useAtOp-iJB],lhs,rhs);
                        useAt->setJCond(_expr);
                    }
                }

                else if (useAtOp == iJCXZ)
                {
                    //NOTICE: was rCX, 0
                    lhs = new RegisterNode(LLOperand(rCX, 0 ), &localId);
                    useAt->setRegDU (rCX, eUSE);
                    rhs = new Constant(0, 2);
                    _expr = BinaryOperator::Create(EQUAL,lhs,rhs);
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
                    _expr = dynamic_cast<BinaryOperator *>(_prev.hl()->expr()->clone());
                    assert(_expr);
                    _expr->changeBoolOp (condOpJCond[useAtOp-iJB]);
                    useAt->copyDU(_prev, eUSE, eUSE);
                    useAt->setJCond(_expr);
                }
            }
            /* Error - definition not found for use of a cond code */
            else if (defAt == pBB->rend())
            {
                reportError(DEF_NOT_FOUND,useAtOp);
            }
        }
    }
}


/** Generates the LiveUse() and Def() sets for each basic block in the graph.
  \note these sets are constant and could have been constructed during
        the construction of the graph, but since the code hasn't been
        analyzed yet for idioms, the procedure preamble misleads the
        analysis (eg: push si, would include si in LiveUse; although it
        is not really meant to be a register that is used before defined). */
void Function::genLiveKtes ()
{
    BB * pbb;
    LivenessSet liveUse, def;

    for (size_t i = 0; i < numBBs; i++)
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
                liveUse |= (insn.du.use - def);
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
void Function::liveRegAnalysis (LivenessSet &in_liveOut)
{
    using namespace boost::adaptors;
    using namespace boost::assign;
    //BB * pbb=0;             /* pointer to current basic block   */
    Function * pcallee;     /* invoked subroutine               */
    //ICODE  *ticode        /* icode that invokes a subroutine  */
    ;
    LivenessSet prevLiveOut,	/* previous live out 				*/
            prevLiveIn;		/* previous live in					*/
    bool change;			/* is there change in the live sets?*/

    /* liveOut for this procedure */
    liveOut = in_liveOut;

    change = true;
    while (change)
    {
        /* Process nodes in reverse postorder order */
        change = false;
        auto valid_reversed_bbs = (m_dfsLast | reversed | filtered(BB::ValidFunctor()) );
        for( BB * pbb : valid_reversed_bbs) // for each valid pbb in reversed dfs order
        {

            /* Get current liveIn() and liveOut() sets */
            prevLiveIn  = pbb->liveIn;
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
                        picode->hlU()->expr(AstIdent::idID(&retVal, &localId, (++pbb->rbegin()).base()));
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
                            pbb->liveOut.reset();
                    }

                    if ((! (pcallee->flg & PROC_ISLIB)) || ( pbb->liveOut.any() ))
                    {
                        switch (pcallee->retVal.type) {
                        case TYPE_LONG_SIGN: case TYPE_LONG_UNSIGN:
                            ticode.du1.setDef(rAX).addDef(rDX);
                            //TODO: use Calling convention to properly set regs here
                            break;
                        case TYPE_WORD_SIGN: case TYPE_WORD_UNSIGN:
                        case TYPE_BYTE_SIGN: case TYPE_BYTE_UNSIGN:
                            ticode.du1.setDef(rAX);
                            break;
                        default:
                            ticode.du1 = ICODE::DU1(); // was .numRegsDef = 0
                            //fprintf(stderr,"Function::liveRegAnalysis : Unknown return type %d, assume 0\n",pcallee->retVal.type);
                        } /*eos*/

                        /* Propagate def/use results to calling icode */
                        ticode.du.use = pcallee->liveIn;
                        ticode.du.def = pcallee->liveOut;
                    }
                }
            }

            /* liveIn(b) = liveUse(b) U (liveOut(b) - def(b) */
            pbb->liveIn = LivenessSet(pbb->liveUse + (pbb->liveOut - pbb->def));

            /* Check if live sets have been modified */
            if ((prevLiveIn != pbb->liveIn) || (prevLiveOut != pbb->liveOut))
                change = true;
        }
    }
    BB *pbb = m_dfsLast.front();
    /* Propagate liveIn(b) to procedure header */
    if (pbb->liveIn.any())   /* uses registers */
        liveIn = pbb->liveIn;

    /* Remove any references to register variables */
    if (flg & SI_REGVAR)
    {
        liveIn.clrReg(rSI);
        pbb->liveIn.clrReg(rSI);
    }
    if (flg & DI_REGVAR)
    {
        liveIn.clrReg(rDI);
        pbb->liveIn.clrReg(rDI);
    }
}

/* Check remaining instructions of the BB for all uses
 * of register regi, before any definitions of the
 * register */
bool BB::FindUseBeforeDef(eReg regi, int defRegIdx, iICODE start_at)
{
    if ((regi == rDI) && (flg & DI_REGVAR))
        return true;
    if ((regi == rSI) && (flg & SI_REGVAR))
        return true;
    if (distance(start_at,end())>1) /* several instructions */
    {
        iICODE ticode=end();
        // Only check uses of HIGH_LEVEL icodes
        auto hl_range=make_iterator_range(start_at,end()) | filtered(ICODE::select_high_level);
        auto checked_icode=hl_range.begin();
        ++checked_icode;
        for (; checked_icode != hl_range.end(); ++checked_icode)
        {
            ICODE &ic(*checked_icode);
            /* if used, get icode index */
            if ( ic.du.use.testRegAndSubregs(regi) )
                start_at->du1.recordUse(defRegIdx,checked_icode.base());
            /* if defined, stop finding uses for this reg */
            if (ic.du.def.testRegAndSubregs(regi))
            {
                ticode=checked_icode.base();
                break;
            }
        }
        if(ticode==end())
            ticode=(++riICODE(rbegin())).base();

        /* Check if last definition of this register */
        if (not ticode->du.def.testRegAndSubregs(regi) and liveOut.testRegAndSubregs(regi) )
            start_at->du.lastDefRegi.addReg(regi);
    }
    else		/* only 1 instruction in this basic block */
    {
        /* Check if last definition of this register */
        if ( liveOut.testRegAndSubregs(regi) )
            start_at->du.lastDefRegi.addReg(regi);
    }
    return false;
}
/* Find target icode for HLI_CALL icodes to procedures
 * that are functions.  The target icode is in the
 * next basic block (unoptimized code) or somewhere else
 * on optimized code. */
void BB::ProcessUseDefForFunc(eReg regi, int defRegIdx, ICODE &picode)
{
    if (!((picode.hl()->opcode == HLI_CALL) && (picode.hl()->call.proc->flg & PROC_IS_FUNC)))
        return;

    BB *tbb = this->edges[0].BBptr;
    auto target_instructions = tbb->instructions | filtered(ICODE::select_high_level);
    for (auto iter=target_instructions.begin(); iter!=target_instructions.end(); ++iter)
    {
        /* if used, get icode index */
        if ( iter->du.use.testRegAndSubregs(regi) )
            picode.du1.recordUse(defRegIdx,iter.base());
        /* if defined, stop finding uses for this reg */
        if (iter->du.def.testRegAndSubregs(regi))
            break;
    }

    /* if not used in this basic block, check if the
     * register is live out, if so, make it the last
     * definition of this register */
    if ( picode.du1.used(defRegIdx) && tbb->liveOut.testRegAndSubregs(regi))
        picode.du.lastDefRegi.addReg(regi);
}

/* If not used within this bb or in successors of this
 * bb (ie. not in liveOut), then register is useless,
 * thus remove it.  Also check that this is not a return
 * from a library function (routines such as printf
 * return an integer, which is normally not taken into
 * account by the programmer). 	*/
void BB::RemoveUnusedDefs(eReg regi, int defRegIdx, iICODE picode)
{
    if (picode->valid() and not picode->du1.used(defRegIdx) and
            (not picode->du.lastDefRegi.testRegAndSubregs(regi)) &&
            (not ((picode->hl()->opcode == HLI_CALL) &&
                  (picode->hl()->call.proc->flg & PROC_ISLIB))))
    {
        if (! (this->liveOut.testRegAndSubregs(regi)))	/* not liveOut */
        {
            bool res = picode->removeDefRegi (regi, defRegIdx+1,&Parent->localId);
            if (res == true)
            {

                /* Backpatch any uses of this instruction, within
                 * the same BB, if the instruction was invalidated */
                rICODE the_rest(begin(),picode);
                for ( ICODE &back_patch_at : the_rest|reversed)
                {
                    back_patch_at.du1.remove(0,picode);
                }
            }
        }
        else		/* liveOut */
            picode->du.lastDefRegi.addReg(regi);
    }
}

void BB::genDU1()
{
    /* Process each register definition of a HIGH_LEVEL icode instruction.
     * Note that register variables should not be considered registers.
     */
    assert(nullptr!=Parent);
    auto all_high_levels =  instructions | filtered(ICODE::select_high_level);
    for (auto picode=all_high_levels.begin(); picode!=all_high_levels.end(); ++picode)
    {
        ICODE &ic = *picode;
        int defRegIdx = 0;
        // foreach defined register
        for (int k = rAX; k < INDEX_BX_SI; k++)
        {
            if (not ic.du.def.testReg(k))
                continue;
            eReg regi = (eReg)(k);      /* Register that was defined */
            picode->du1.regi[defRegIdx] = regi;

            if(FindUseBeforeDef(regi,defRegIdx, picode.base()))
                continue;

            ProcessUseDefForFunc(regi, defRegIdx,ic);
            RemoveUnusedDefs(regi, defRegIdx, picode.base());

            defRegIdx++;

            /* Check if all defined registers have been processed */
            if ((defRegIdx >= picode->du1.getNumRegsDef()) || (defRegIdx == MAX_REGS_DEF))
                break;
        }
    }
}
/* Generates the du chain of each instruction in a basic block */
void Function::genDU1 ()
{
    /* Traverse tree in dfsLast order */
    assert(m_dfsLast.size()==numBBs);
    for(BB *pbb : m_dfsLast | filtered(BB::ValidFunctor()))
    {
        pbb->genDU1();
    }

}

/* Substitutes the rhs (or lhs if rhs not possible) of ticode for the rhs of picode. */
void LOCAL_ID::forwardSubs (Expr *lhs, Expr *rhs, iICODE picode, iICODE ticode, int &numHlIcodes) const
{
    bool res;
    UnaryOperator *lhs_unary;
    while( (lhs_unary = dynamic_cast<UnaryOperator *>(lhs)) )
    {
        if(dynamic_cast<AstIdent *>(lhs_unary))
            break;
        lhs = lhs_unary->unaryExp;
    }
    RegisterNode * lhs_reg=dynamic_cast<RegisterNode *>(lhs_unary);
    assert(lhs_reg);
    if (rhs == nullptr)        /* In case expression popped is NULL */
        return;

    /* Insert on rhs of ticode, if possible */
    res = Expr::insertSubTreeReg (ticode->hlU()->asgn.rhs,rhs, id_arr[lhs_reg->regiIdx].id.regi, this);
    if (res)
    {
        picode->invalidate();
        numHlIcodes--;
    }
    else
    {
        /* Try to insert it on lhs of ticode*/
        RegisterNode *op = dynamic_cast<RegisterNode *>(ticode->hlU()->asgn.m_lhs);
        if(op)
        {
            eReg inserted = id_arr[lhs_reg->regiIdx].id.regi;
            eReg lhsReg =   id_arr[op->regiIdx].id.regi;
            if((lhsReg==inserted)||Machine_X86::isSubRegisterOf(lhsReg,inserted))
            {
                // Do not replace ax = XYZ; given ax = H << P;  with H << P  =
                return;
            }
        }
        res = Expr::insertSubTreeReg (ticode->hlU()->asgn.m_lhs,rhs, id_arr[lhs_reg->regiIdx].id.regi, this);
        if (res)
        {
            picode->invalidate();
            numHlIcodes--;
        }
    }
}


/* Substitutes the rhs (or lhs if rhs not possible) of ticode for the expression exp given */
static void forwardSubsLong (int longIdx, Expr *_exp, iICODE picode, iICODE ticode, int *numHlIcodes)
{
    bool res;

    if (_exp == nullptr)        /* In case expression popped is NULL */
        return;

    /* Insert on rhs of ticode, if possible */
    res = Expr::insertSubTreeLongReg (_exp, ticode->hlU()->asgn.rhs, longIdx);
    if (res)
    {
        picode->invalidate();
        (*numHlIcodes)--;
    }
    else
    {
        /* Try to insert it on lhs of ticode*/
        res = Expr::insertSubTreeLongReg (_exp, ticode->hlU()->asgn.m_lhs, longIdx);
        if (res)
        {
            picode->invalidate();
            (*numHlIcodes)--;
        }
    }
}

/* Returns whether the elements of the expression rhs are all x-clear from
 * instruction f up to instruction t.	*/
bool UnaryOperator::xClear(rICODE range_to_check, iICODE lastBBinst, const LOCAL_ID &locs)
{
    if(nullptr==unaryExp)
        return false;
    return unaryExp->xClear ( range_to_check, lastBBinst, locs);
}

bool BinaryOperator::xClear(rICODE range_to_check, iICODE lastBBinst, const LOCAL_ID &locs)
{
    if(nullptr==m_rhs)
        return false;
    if ( ! m_rhs->xClear (range_to_check, lastBBinst, locs) )
        return false;
    if(nullptr==m_lhs)
        return false;
    return m_lhs->xClear (range_to_check, lastBBinst, locs);
}
bool AstIdent::xClear(rICODE range_to_check, iICODE lastBBinst, const LOCAL_ID &locId)
{
    if (ident.idType != REGISTER)
        return true;
    assert(false);
    return false;
}
/** Checks the type of the formal argument as against to the actual argument,
  whenever possible, and then places the actual argument on the procedure's
  argument list.
   @returns the type size of the stored Arg
*/
int C_CallingConvention::processCArg (Function * callee, Function * pProc, ICODE * picode, size_t numArgs)
{
    Expr *_exp;
    bool res;
    int size_of_arg=0;
    PROG &prog(Project::get()->prog);


    /* if (numArgs == 0)
                return; */
    assert(pProc==g_exp_stk.func);
    _exp = g_exp_stk.pop();
    if (callee->flg & PROC_ISLIB) /* library function */
    {
        if (callee->args.numArgs > 0)
        {
            if (callee->getFunctionType()->isVarArg())
            {
                if (numArgs < callee->args.size()) {
                    if(_exp==nullptr)
                        fprintf(stderr,"Would try to adjustForArgType with null _exp\n");
                    else
                        _exp = pProc->adjustActArgType (_exp, callee->args[numArgs].type);
                }
            }
            else {
                if(numArgs<callee->args.size()) {
                    if(prog.addressingMode=='l') {
                        if((callee->args[numArgs].type==TYPE_STR)||(callee->args[numArgs].type==TYPE_PTR)) {
                            RegisterNode *rn = dynamic_cast<RegisterNode *>(g_exp_stk.top());
                            AstIdent *idn = dynamic_cast<AstIdent *>(g_exp_stk.top());
                            if(rn) {
                                const ID &_id(pProc->localId.id_arr[rn->regiIdx]);
                                assert(&pProc->localId==rn->m_syms);
                                if(_id.id.regi==rDS) {
                                    g_exp_stk.pop(); // pop segment
                                    size_of_arg += 2;
                                }
                            } else if(idn) {
                                Expr *tmp1 = new Constant(2,1);
                                Expr *tmp2 = BinaryOperator::createSHL(_exp,tmp1);
                                _exp = BinaryOperator::CreateAdd(g_exp_stk.top(),tmp2);
                                g_exp_stk.pop(); // pop segment
                                size_of_arg += 2;
                            }

                        }
                    }
                    _exp = pProc->adjustActArgType (_exp, callee->args[numArgs].type);
                } else {
                    fprintf(stderr,"processCArg tried to query non existent arg\n");
                }
            }
        }
    }
    else			/* user function */
    {
        if (callee->args.numArgs > 0)
        {
            if(_exp==nullptr)
                fprintf(stderr,"Would try to adjustForArgType with null _exp\n");
            else
                callee->args.adjustForArgType (numArgs, _exp->expType (pProc));
        }
    }
    res = picode->newStkArg (_exp, (llIcode)picode->ll()->getOpcode(), pProc);
    /* Do not update the size of k if the expression was a segment register
         * in a near call */
    if (res == false)
    {
        if(_exp==nullptr)
            return 2+size_of_arg;
        return _exp->hlTypeSize (pProc)+size_of_arg;
    }
    return 0; // be default we do not know the size of the argument
}

/** Eliminates extraneous intermediate icode instructions when finding
 * expressions.  Generates new hlIcodes in the form of expression trees.
 * For HLI_CALL hlIcodes, places the arguments in the argument list.    */
void LOCAL_ID::processTargetIcode(iICODE picode, int &numHlIcodes, iICODE ticode,bool isLong) const
{
    bool res;
    HLTYPE &p_hl(*picode->hlU());
    HLTYPE &t_hl(*ticode->hlU());

    AstIdent *lhs_ident = dynamic_cast<AstIdent *>(p_hl.asgn.lhs());
    switch (t_hl.opcode)
    {
    case HLI_ASSIGN:
        assert(lhs_ident);
        if(isLong)
        {
            forwardSubsLong (lhs_ident->ident.idNode.longIdx,
                             p_hl.asgn.rhs, picode,ticode,
                             &numHlIcodes);
        }
        else
            this->forwardSubs (lhs_ident, p_hl.asgn.rhs, picode, ticode, numHlIcodes);
        break;

    case HLI_JCOND:  case HLI_PUSH:  case HLI_RET:
        if(isLong)
        {
            assert(lhs_ident);
            res = Expr::insertSubTreeLongReg (
                        p_hl.asgn.rhs,
                        t_hl.exp.v,
                        lhs_ident->ident.idNode.longIdx);
        }
        else
        {
            RegisterNode *lhs_reg = dynamic_cast<RegisterNode *>(p_hl.asgn.lhs());
            assert(lhs_reg);
            res = Expr::insertSubTreeReg (
                        t_hl.exp.v,
                        p_hl.asgn.rhs,
                        id_arr[lhs_reg->regiIdx].id.regi,
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
    default:
        fprintf(stderr,"unhandled LOCAL_ID::processTargetIcode opcode %d\n",t_hl.opcode);

    }
}
void C_CallingConvention::processHLI(Function *func,Expr *_exp, iICODE picode) {
    Function * pp;
    int cb, numArgs;
    int k;
    pp = picode->hl()->call.proc;
    cb = picode->hl()->call.args->cb;
    numArgs = 0;
    k = 0;
    if (cb)
    {
        while ( k < cb )
        {
            k+=processCArg (pp, func, &(*picode), numArgs);
            numArgs++;
        }
    }
    else if ((cb == 0) && picode->ll()->testFlags(REST_STK))
    {
        while (! g_exp_stk.empty())
        {
            k+=processCArg (pp, func, &(*picode), numArgs);
            numArgs++;
        }
    }
}

void Pascal_CallingConvention::processHLI(Function *func,Expr *_exp, iICODE picode) {
    Function * pp;
    int cb, numArgs;
    bool res;
    int k;
    pp = picode->hl()->call.proc;

    cb = pp->cbParam;	/* fixed # arguments */
    k = 0;
    numArgs = 0;
    while(k<cb)
    {
        _exp = g_exp_stk.pop();
        if (pp->flg & PROC_ISLIB)	/* library function */
        {
            if (pp->args.numArgs > 0)
                _exp = func->adjustActArgType(_exp, pp->args[numArgs].type);
            res = picode->newStkArg (_exp, (llIcode)picode->ll()->getOpcode(), func);
        }
        else			/* user function */
        {
            if (pp->args.numArgs >0)
            {
                if(_exp==nullptr)
                {
                    fprintf(stderr,"Would try to adjustForArgType with null _exp\n");
                }
                pp->args.adjustForArgType (numArgs,_exp->expType (func));
            }
            res = picode->newStkArg (_exp,(llIcode)picode->ll()->getOpcode(), func);
        }
        if (res == false)
            k += _exp->hlTypeSize (func);
        numArgs++;
    }
}

void Function::processHliCall(Expr *_exp, iICODE picode)
{
    Function * pp = picode->hl()->call.proc;
    pp->callingConv()->processHLI(this,_exp,picode);
}


void BB::findBBExps(LOCAL_ID &locals,Function *fnc)
{
    bool res;

    ID *_retVal;         // function return value
    Expr *_exp;     // expression pointer - for HLI_POP and HLI_CALL    */
    //Expr *lhs;	// exp ptr for return value of a HLI_CALL		*/
    iICODE ticode;     // Target icode                             */
    HLTYPE *ti_hl=nullptr;
    uint8_t regi;
    numHlIcodes = 0;
    assert(&fnc->localId==&locals);
    // register(s) to be forward substituted	*/
    auto valid_and_highlevel = instructions | filtered(ICODE::TypeAndValidFilter<HIGH_LEVEL>());
    for (auto picode = valid_and_highlevel.begin(); picode != valid_and_highlevel.end(); picode++)
    {
        HLTYPE &_icHl(*picode->hlU());
        numHlIcodes++;
        if (picode->du1.getNumRegsDef() == 1)    /* uint8_t/uint16_t regs */
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
                    if ((picode->du.lastDefRegi.testRegAndSubregs(regi)) &&
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
                    // TODO: sometimes picode->du1.idx[0].uses.front() points to next basic block ?
                    // pop X
                    // lab1:
                    //   call F() <- somehow this is marked as user of POP ?
                    ticode = picode->du1.idx[0].uses.front();
                    ti_hl = ticode->hlU();
                    if ((picode->du.lastDefRegi.testRegAndSubregs(regi)) &&
                            ((ti_hl->opcode != HLI_CALL) &&
                             (ti_hl->opcode != HLI_RET)))
                        continue;

                    _exp = g_exp_stk.pop();  /* pop last exp pushed */
                    switch (ticode->hl()->opcode) {
                    case HLI_ASSIGN:
                        locals.forwardSubs(_icHl.expr(), _exp, picode.base(), ticode, numHlIcodes);
                        break;

                    case HLI_JCOND: case HLI_PUSH: case HLI_RET:
                    {
                        RegisterNode *v = dynamic_cast<RegisterNode *>(_icHl.expr());
                        assert(v);
                        res = Expr::insertSubTreeReg (ti_hl->exp.v,
                                                      _exp,
                                                      locals.id_arr[v->regiIdx].id.regi,
                                &locals);
                        if (res)
                        {
                            picode->invalidate();
                            numHlIcodes--;
                        }
                    }
                        break;

                        /****case HLI_CALL:    // register arguments
                        newRegArg (pProc, picode, ticode);
                        picode->invalidate();
                        numHlIcodes--;
                        break;	*/
                    default:
                        fprintf(stderr,"unhandled BB::findBBExps target opcode %d\n",ticode->hl()->opcode);

                    } // eos
                    break;

                case HLI_CALL:
                    ticode = picode->du1.idx[0].uses.front();
                    ti_hl = ticode->hlU();
                    _retVal = &_icHl.call.proc->retVal;
                    switch (ti_hl->opcode)
                    {
                    case HLI_ASSIGN:
                        assert(ti_hl->asgn.rhs);
                        _exp = _icHl.call.toAst();
                        res = Expr::insertSubTreeReg (ti_hl->asgn.rhs,_exp, _retVal->id.regi, &locals);
                        if (! res)
                            Expr::insertSubTreeReg (ti_hl->asgn.m_lhs, _exp,_retVal->id.regi, &locals);
                        //TODO: HERE missing: 2 regs
                        picode->invalidate();
                        numHlIcodes--;
                        break;

                    case HLI_PUSH: case HLI_RET:
                        ti_hl->expr( _icHl.call.toAst() );
                        picode->invalidate();
                        numHlIcodes--;
                        break;

                    case HLI_JCOND:
                        _exp = _icHl.call.toAst();
                        res = Expr::insertSubTreeReg (ti_hl->exp.v, _exp, _retVal->id.regi, &locals);
                        if (res)	/* was substituted */
                        {
                            picode->invalidate();
                            numHlIcodes--;
                        }
                        else	/* cannot substitute function */
                        {
                            auto lhs = AstIdent::idID(_retVal,&locals,picode.base());
                            picode->setAsgn(lhs, _exp);
                        }
                        break;
                    default:
                        fprintf(stderr,"unhandled BB::findBBExps HLI_CALL target opcode %d\n",ti_hl->opcode);
                    } /* eos */
                    break;
                default:
                    fprintf(stderr,"BB::findBBExps Unhandled HLI %d\n",_icHl.opcode);
                } /* eos */
            }
        }

        else if (picode->du1.getNumRegsDef() == 2)   /* long regs */
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
                        if ((picode->du.lastDefRegi.testRegAndSubregs(regi)) &&
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
                        if ((picode->du.lastDefRegi.testRegAndSubregs(regi)) &&
                                ((ticode->hl()->opcode != HLI_CALL) &&
                                 (ticode->hl()->opcode != HLI_RET)))
                            continue;

                        _exp = g_exp_stk.pop(); /* pop last exp pushed */
                        switch (ticode->hl()->opcode) {
                        case HLI_ASSIGN:
                            forwardSubsLong (dynamic_cast<AstIdent *>(_icHl.expr())->ident.idNode.longIdx,
                                             _exp, picode.base(), ticode, &numHlIcodes);
                            break;
                        case HLI_JCOND: case HLI_PUSH:
                            res = Expr::insertSubTreeLongReg (_exp,
                                                              ticode->hlU()->exp.v,
                                                              dynamic_cast<AstIdent *>(_icHl.asgn.lhs())->ident.idNode.longIdx);
                            if (res)
                            {
                                picode->invalidate();
                                numHlIcodes--;
                            }
                            break;
                        case HLI_CALL:	/*** missing ***/
                            break;
                        default:
                            fprintf(stderr,"BB::findBBExps Unhandled target op %d\n",ticode->hl()->opcode);
                        } /* eos */
                    }
                    break;

                case HLI_CALL:    /* check for function return */
                    ticode = picode->du1.idx[0].uses.front();
                    switch (ticode->hl()->opcode)
                    {
                    case HLI_ASSIGN:
                        _exp = _icHl.call.toAst();
                        ticode->hlU()->asgn.lhs(
                                    AstIdent::Long(&locals, DST,
                                                   ticode,HIGH_FIRST, picode.base(),
                                                   eDEF, *(++iICODE(ticode))->ll()));
                        ticode->hlU()->asgn.rhs = _exp;
                        picode->invalidate();
                        numHlIcodes--;
                        break;

                    case HLI_PUSH:
                    case HLI_RET:
                        ticode->hlU()->expr( _icHl.call.toAst() );
                        picode->invalidate();
                        numHlIcodes--;
                        break;

                    case HLI_JCOND:
                        _exp = _icHl.call.toAst();
                        _retVal = &picode->hl()->call.proc->retVal;
                        res = Expr::insertSubTreeLongReg (_exp,
                                                          ticode->hlU()->exp.v,
                                                          locals.newLongReg ( _retVal->type, _retVal->longId(), picode.base()));
                        if (res)	/* was substituted */
                        {
                            picode->invalidate();
                            numHlIcodes--;
                        }
                        else	/* cannot substitute function */
                        {
                            auto lhs = locals.createId(_retVal,picode.base());
                            picode->setAsgn(lhs, _exp);
                        }
                        break;
                    default:
                        fprintf(stderr,"BB::findBBExps Unhandled target op %d\n",ticode->hl()->opcode);
                    } /* eos */
                    break;
                default:
                    fprintf(stderr,"BB::findBBExps Unhandled HLI %d\n",_icHl.opcode);

                } /* eos */
            }
        }
        /* HLI_PUSH doesn't define any registers, only uses registers.
         * Push the associated expression to the register on the local
         * expression stack */
        else if (_icHl.opcode == HLI_PUSH)
        {
            g_exp_stk.processExpPush(numHlIcodes, *picode);
        }
        else if(picode->du1.getNumRegsDef()!=0)
            printf("Num def %d\n",picode->du1.getNumRegsDef());

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
            if ( not _icHl.call.proc->isLibrary() and (not picode->du1.used(0)) and (picode->du1.getNumRegsDef() > 0))
            {
                _exp = new FuncNode(_icHl.call.proc, _icHl.call.args);
                auto lhs = AstIdent::idID (&_icHl.call.proc->retVal, &locals, picode.base());
                picode->setAsgn(lhs, _exp);
            }
        }
    }
    /* Store number of high-level icodes in current basic block */

}

void Function::findExps()
{
    /* Initialize expression stack */
    g_exp_stk.init(this);
    /* Traverse tree in dfsLast order */
    for(BB *pbb : m_dfsLast | filtered(BB::ValidFunctor()))
    {
        /* Process one valid BB */
        pbb->findBBExps( this->localId, this);
    }
}

void Function::preprocessReturnDU(LivenessSet &_liveOut)
{
    if (_liveOut.any())
    {
        //        int idx;
        bool isAx, isBx, isCx, isDx;
        eReg bad_regs[] = {rES,rCS,rDS,rSS};
        constexpr const char * names[] ={"ES","CS","DS","SS"};
        for(int i=0; i<4; ++i)
            if(_liveOut.testReg(bad_regs[i]))
            {
                fprintf(stderr,"LivenessSet probably screwed up, %s register as an liveOut in preprocessReturnDU\n",names[i]);
                _liveOut.clrReg(bad_regs[i]);
                if(not _liveOut.any())
                    return;
            }
        flg |= PROC_IS_FUNC;
        isAx = _liveOut.testReg(rAX);
        isBx = _liveOut.testReg(rBX);
        isCx = _liveOut.testReg(rCX);
        isDx = _liveOut.testReg(rDX);
        bool isAL = !isAx && _liveOut.testReg(rAL);
        bool isAH = !isAx && _liveOut.testReg(rAH);
        bool isBL = !isBx && _liveOut.testReg(rBL);
        bool isBH = !isBx && _liveOut.testReg(rBH);
        bool isCL = !isCx && _liveOut.testReg(rCL);
        bool isCH = !isCx && _liveOut.testReg(rCH);
        bool isDL = !isDx && _liveOut.testReg(rDL);
        bool isDH = !isDx && _liveOut.testReg(rDH);
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
            retVal.longId() = LONGID_TYPE(rDX,rAX);
            /*idx = */localId.newLongReg(TYPE_LONG_SIGN, LONGID_TYPE(rDX,rAX), Icode.begin());
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
            /*idx = */localId.newByteWordReg(TYPE_WORD_SIGN,retVal.id.regi);
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
            /*idx = */localId.newByteWordReg(TYPE_BYTE_SIGN,retVal.id.regi);
        }
        else if(isAH||isBH||isCH||isDH)
        {
            retVal.type = TYPE_BYTE_SIGN;
            retVal.loc = REG_FRAME;
            if (isAH)
                retVal.id.regi = rAH;
            else if (isBH)
                retVal.id.regi = rBH;
            else if (isCH)
                retVal.id.regi = rCH;
            else
                retVal.id.regi = rDH;
            /*idx = */localId.newByteWordReg(TYPE_BYTE_SIGN,retVal.id.regi);
        }
    }
}
/** Invokes procedures related with data flow analysis.
 * Works on a procedure at a time basis.
 \note indirect recursion in liveRegAnalysis is possible. */
void Function::dataFlow(LivenessSet &_liveOut)
{

    /* Remove references to register variables */
    if (flg & SI_REGVAR)
        _liveOut.clrReg(rSI);
    if (flg & DI_REGVAR)
        _liveOut.clrReg(rDI);

    /* Function - return value register(s) */
    preprocessReturnDU(_liveOut);

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
