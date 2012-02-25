/**************************************************************************
 * File : propLong.c
 * Purpose: propagate the value of long variables (local variables and
 *          registers) along the graph.  Structure the graph in this way.
 * (C) Cristina Cifuentes
 **************************************************************************/
#include <string.h>
#include <memory.h>
#include <cassert>
#include <algorithm>

#include "dcc.h"

/* Returns whether the given icode opcode is within the range of valid
 * high-level conditional jump icodes (iJB..iJG) */
static boolT isJCond (llIcode opcode)
{
    if ((opcode >= iJB) && (opcode <= iJG))
        return true;
    return false;
}


/* Returns whether the conditions for a 2-3 long variable are satisfied */
static boolT isLong23 (Int i, BB * pbb, Int *off, Int *arc)
{
    BB * t, * e, * obb2;

    if (pbb->nodeType != TWO_BRANCH)
        return false;
    t = pbb->edges[THEN].BBptr;
    e = pbb->edges[ELSE].BBptr;

    /* Check along the THEN path */
    if ((t->size() == 1) && (t->nodeType == TWO_BRANCH) && (t->inEdges.size() == 1))
    {
        obb2 = t->edges[THEN].BBptr;
        if ((obb2->size() == 2) && (obb2->nodeType == TWO_BRANCH) && (obb2->front().ic.ll.opcode == iCMP))
        {
            *off = obb2->front().loc_ip - i;
            *arc = THEN;
            return true;
        }
    }

    /* Check along the ELSE path  */
    else if ((e->size() == 1) && (e->nodeType == TWO_BRANCH) && (e->inEdges.size() == 1))
    {
        obb2 = e->edges[THEN].BBptr;
        if ((obb2->size() == 2) && (obb2->nodeType == TWO_BRANCH) &&  (obb2->front().ic.ll.opcode == iCMP))
        {
            *off = obb2->front().loc_ip - i;
            *arc = ELSE;
            return true;
        }
    }
    return false;
}
static boolT isLong23 (iICODE iter, BB * pbb, Int *off, Int *arc)
{
    BB * t, * e, * obb2;

    if (pbb->nodeType != TWO_BRANCH)
        return false;
    t = pbb->edges[THEN].BBptr;
    e = pbb->edges[ELSE].BBptr;

    /* Check along the THEN path */
    if ((t->size() == 1) && (t->nodeType == TWO_BRANCH) && (t->inEdges.size() == 1))
    {
        obb2 = t->edges[THEN].BBptr;
        if ((obb2->size() == 2) && (obb2->nodeType == TWO_BRANCH) && (obb2->front().ic.ll.opcode == iCMP))
        {
            *off = std::distance(iter,obb2->begin2());
            *arc = THEN;
            return true;
        }
    }

    /* Check along the ELSE path  */
    else if ((e->size() == 1) && (e->nodeType == TWO_BRANCH) && (e->inEdges.size() == 1))
    {
        obb2 = e->edges[THEN].BBptr;
        if ((obb2->size() == 2) && (obb2->nodeType == TWO_BRANCH) &&  (obb2->front().ic.ll.opcode == iCMP))
        {
            *off = std::distance(iter,obb2->begin2());//obb2->front().loc_ip - i;
            *arc = ELSE;
            return true;
        }
    }
    return false;
}


/* Returns whether the conditions for a 2-2 long variable are satisfied */
static boolT isLong22 (iICODE pIcode, iICODE pEnd, Int *off)
{
    iICODE next1 = pIcode+1;
    iICODE next2 = pIcode+2;
    iICODE next3 = pIcode+3;
    if(next3>=pEnd)
        return false;
    if (   (next2->ic.ll.opcode == iCMP) &&
           (isJCond (next1->ic.ll.opcode)) &&
           (isJCond (next3->ic.ll.opcode)))
    {
        *off = 2;
        return true;
    }
    return false;
}


/** Creates a long conditional <=, >=, <, or > at (pIcode+1).
 * Removes excess nodes from the graph by flagging them, and updates
 * the new edges for the remaining nodes.
 * @return number of ICODEs to skip

*/
static int longJCond23 (COND_EXPR *rhs, COND_EXPR *lhs, iICODE pIcode, Int arc, Int off)
{
    BB * pbb, * obb1, * obb2, * tbb;
    int skipped_insn=0;
    if (arc == THEN)
    {
        /* Find intermediate basic blocks and target block */
        pbb = pIcode->inBB;
        obb1 = pbb->edges[THEN].BBptr;
        obb2 = obb1->edges[THEN].BBptr;
        tbb = obb2->edges[THEN].BBptr;

        /* Modify out edge of header basic block */
        pbb->edges[THEN].BBptr = tbb;

        /* Modify in edges of target basic block */
        auto newlast=std::remove_if(tbb->inEdges.begin(),tbb->inEdges.end(),
                                    [obb1,obb2](BB *b) -> bool {
                                    return (b==obb1) || (b==obb2); });
        tbb->inEdges.erase(newlast,tbb->inEdges.end());
        tbb->inEdges.push_back(pbb); /* looses 2 arcs, gains 1 arc */

        /* Modify in edges of the ELSE basic block */
        tbb = pbb->edges[ELSE].BBptr;
        auto iter=std::find(tbb->inEdges.begin(),tbb->inEdges.end(),obb2);
        assert(iter!=tbb->inEdges.end());
        tbb->inEdges.erase(iter); /* looses 1 arc */
        /* Update icode index */
        skipped_insn = 5;
    }
    else  /* ELSE arc */
    {
        /* Find intermediate basic blocks and target block */
        pbb = pIcode->inBB;
        obb1 = pbb->edges[ELSE].BBptr;
        obb2 = obb1->edges[THEN].BBptr;
        tbb = obb2->edges[THEN].BBptr;

        /* Modify in edges of target basic block */
        auto iter=std::find(tbb->inEdges.begin(),tbb->inEdges.end(),obb2);
        assert(iter!=tbb->inEdges.end());
        tbb->inEdges.erase(iter); /* looses 1 arc */

        /* Modify in edges of the ELSE basic block */
        tbb = obb2->edges[ELSE].BBptr;
        auto newlast=std::remove_if(tbb->inEdges.begin(),tbb->inEdges.end(),
                                    [obb1,obb2](BB *b) -> bool { return (b==obb1) || (b==obb2); });
        tbb->inEdges.erase(newlast,tbb->inEdges.end());
        tbb->inEdges.push_back(pbb); /* looses 2 arcs, gains 1 arc */

        /* Modify out edge of header basic block */
        pbb->edges[ELSE].BBptr = tbb;

        /* Update icode index */
        skipped_insn = 2;
    }

    /* Create new HLI_JCOND and condition */
    lhs = COND_EXPR::boolOp (lhs, rhs, condOpJCond[(pIcode+off+1)->ic.ll.opcode-iJB]);
    (pIcode+1)->setJCond(lhs);
    (pIcode+1)->copyDU(*pIcode, eUSE, eUSE);
    (pIcode+1)->du.use |= (pIcode+off)->du.use;

    /* Update statistics */
    obb1->flg |= INVALID_BB;
    obb2->flg |= INVALID_BB;
    stats.numBBaft -= 2;

    pIcode->invalidate();
    obb1->front().invalidate();
    obb2->front().invalidate();
    (obb2->begin2()+1)->invalidate();
    return skipped_insn;
}


/** Creates a long conditional equality or inequality at (pIcode+1).
 * Removes excess nodes from the graph by flagging them, and updates
 * the new edges for the remaining nodes.
 * @return number of ICODE's to skip
*/
static int longJCond22 (COND_EXPR *rhs, COND_EXPR *lhs, iICODE pIcode)
{
    Int j;
    BB * pbb, * obb1, * tbb;
    iICODE next1=pIcode+1;
    iICODE next2=pIcode+2;
    iICODE next3=pIcode+3;
    /* Form conditional expression */
    lhs = COND_EXPR::boolOp (lhs, rhs, condOpJCond[(pIcode+3)->ic.ll.opcode - iJB]);
    next1->setJCond(lhs);
    next1->copyDU (*pIcode, eUSE, eUSE);
    next1->du.use |= next2->du.use;

    /* Adjust outEdges[0] to the new target basic block */
    pbb = pIcode->inBB;
    if (pbb->back().loc_ip == next1->loc_ip)
    {
        /* Find intermediate and target basic blocks */
        obb1 = pbb->edges[THEN].BBptr;
        tbb = obb1->edges[THEN].BBptr;

        /* Modify THEN out edge of header basic block */
        pbb->edges[THEN].BBptr = tbb;

        /* Modify in edges of target basic block */
        auto iter=std::find(tbb->inEdges.begin(),tbb->inEdges.end(),obb1);
        assert(iter!=tbb->inEdges.end());
        tbb->inEdges.erase(iter);

        if ((pIcode+3)->ic.ll.opcode != iJE)
            tbb->inEdges.push_back(pbb); /* iJNE => replace arc */

        /* Modify ELSE out edge of header basic block */
        tbb = obb1->edges[ELSE].BBptr;
        pbb->edges[ELSE].BBptr = tbb;

        iter=std::find(tbb->inEdges.begin(),tbb->inEdges.end(),obb1);
        assert(iter!=tbb->inEdges.end());
        tbb->inEdges.erase(iter);
        if ((pIcode+3)->ic.ll.opcode == iJE)	/* replace */
            tbb->inEdges.push_back(pbb);

        /* Update statistics */
        obb1->flg |= INVALID_BB;
        stats.numBBaft--;
    }

    pIcode->invalidate();
    next2->invalidate();
    next3->invalidate();
    return 4;
}

/* Propagates TYPE_LONG_(UN)SIGN icode information to the current pIcode
 * Pointer.
 * Arguments: i     : index into the local identifier table
 *            pLocId: ptr to the long local identifier
 *            pProc : ptr to current procedure's record.        */
void Function::propLongStk (Int i, ID *pLocId)
{
    Int idx, off, arc;
    COND_EXPR *lhs, *rhs;     /* Pointers to left and right hand expression */
    iICODE pIcode, pEnd;

    /* Check all icodes for offHi:offLo */
    pEnd = Icode.end();
    for (idx = 0; idx < (this->Icode.size() - 1); idx++)
    {
        pIcode = Icode.begin()+idx;
        if ((pIcode->type == HIGH_LEVEL) || (pIcode->invalid == TRUE))
            continue;

        if (pIcode->ic.ll.opcode == (pIcode+1)->ic.ll.opcode)
        {
            switch (pIcode->ic.ll.opcode)
            {
            case iMOV:
                if (checkLongEq (pLocId->id.longStkId, pIcode, i, this,
                                 &rhs, &lhs, 1) == TRUE)
                {
                    pIcode->setAsgn(lhs, rhs);
                    (pIcode+1)->invalidate();
                    idx++;
                }
                break;

            case iAND: case iOR: case iXOR:
                if (checkLongEq (pLocId->id.longStkId, pIcode, i, this,
                                 &rhs, &lhs, 1) == TRUE)
                {
                    switch (pIcode->ic.ll.opcode)
                    {
                    case iAND: 	rhs = COND_EXPR::boolOp (lhs, rhs, AND);
                        break;
                    case iOR: 	rhs = COND_EXPR::boolOp (lhs, rhs, OR);
                        break;
                    case iXOR: 	rhs = COND_EXPR::boolOp (lhs, rhs, XOR);
                        break;
                    }
                    pIcode->setAsgn(lhs, rhs);
                    (pIcode+1)->invalidate();
                    idx++;
                }
                break;

            case iPUSH:
                if (checkLongEq (pLocId->id.longStkId, pIcode, i, this,
                                 &rhs, &lhs, 1) == TRUE)
                {
                    pIcode->setUnary( HLI_PUSH, lhs);
                    (pIcode+1)->invalidate();
                    idx++;
                }
                break;
            } /*eos*/
        }

        /* Check long conditional (i.e. 2 CMPs and 3 branches */
        else if ((pIcode->ic.ll.opcode == iCMP) && (isLong23 (idx, pIcode->inBB, &off, &arc)))
        {
            if ( checkLongEq (pLocId->id.longStkId, pIcode, i, this, &rhs, &lhs, off) )
            {
                idx += longJCond23 (rhs, lhs, pIcode, arc, off); //
            }
        }

        /* Check for long conditional equality or inequality.  This requires
                 * 2 CMPs and 2 branches */
        else if ((pIcode->ic.ll.opcode == iCMP) && isLong22 (pIcode, pEnd, &off))
        {
            if ( checkLongEq (pLocId->id.longStkId, pIcode, i, this,&rhs, &lhs, off) )
            {
                idx += longJCond22 (rhs, lhs, pIcode); // maybe this should have -1 to offset loop autoincrement?
            }
        }
    }
}
int Function::checkBackwarLongDefs(int loc_ident_idx, const ID &pLocId, iICODE beg,Assignment &asgn)
{
    LLOperand * pmH,* pmL;
    iICODE pIcode;
    riICODE rev(beg);
    bool forced_finish=false;
    for (; not forced_finish and rev!=Icode.rend();rev++) //idx = pLocId_idx - 1; idx > 0 ; idx--
    {
        pIcode = (rev+1).base();//Icode.begin()+(idx-1);
        if ((pIcode->type == HIGH_LEVEL) || (pIcode->invalid == TRUE))
            continue;

        if (pIcode->ic.ll.opcode != (pIcode+1)->ic.ll.opcode)
            continue;
        switch (pIcode->ic.ll.opcode)
        {
        case iMOV:
            pmH = &pIcode->ic.ll.dst;
            pmL = &(pIcode+1)->ic.ll.dst;
            if ((pLocId.id.longId.h == pmH->regi) && (pLocId.id.longId.l == pmL->regi))
            {
                asgn.lhs = COND_EXPR::idLongIdx (loc_ident_idx);
                this->localId.id_arr[loc_ident_idx].idx.push_back(pIcode);//idx-1//insert
                pIcode->setRegDU( pmL->regi, eDEF);
                asgn.rhs = COND_EXPR::idLong (&this->localId, SRC, pIcode, HIGH_FIRST, pIcode/*idx*/, eUSE, 1);
                pIcode->setAsgn(asgn.lhs, asgn.rhs);
                (pIcode+1)->invalidate();
                forced_finish=true; /* to exit the loop */
            }
            break;

        case iPOP:
            pmH = &(pIcode+1)->ic.ll.dst;
            pmL = &pIcode->ic.ll.dst;
            if ((pLocId.id.longId.h == pmH->regi) && (pLocId.id.longId.l == pmL->regi))
            {
                asgn.lhs = COND_EXPR::idLongIdx (loc_ident_idx);
                pIcode->setRegDU( pmH->regi, eDEF);
                pIcode->setUnary(HLI_POP, asgn.lhs);
                (pIcode+1)->invalidate();
                forced_finish=true;        /* to exit the loop */
            }
            break;

            //                /**** others missing ***/

        case iAND: case iOR: case iXOR:
            pmL = &pIcode->ic.ll.dst;
            pmH = &(pIcode+1)->ic.ll.dst;
            if ((pLocId.id.longId.h == pmH->regi) && (pLocId.id.longId.l == pmL->regi))
            {
                asgn.lhs = COND_EXPR::idLongIdx (loc_ident_idx);
                pIcode->setRegDU( pmH->regi, USE_DEF);
                asgn.rhs = COND_EXPR::idLong (&this->localId, SRC, pIcode, LOW_FIRST, pIcode/*idx*/, eUSE, 1);
                switch (pIcode->ic.ll.opcode) {
                case iAND: asgn.rhs = COND_EXPR::boolOp (asgn.lhs, asgn.rhs, AND);
                    break;
                case iOR:
                    asgn.rhs = COND_EXPR::boolOp (asgn.lhs, asgn.rhs, OR);
                    break;
                case iXOR: asgn.rhs = COND_EXPR::boolOp (asgn.lhs, asgn.rhs, XOR);
                    break;
                } /* eos */
                pIcode->setAsgn(asgn.lhs, asgn.rhs);
                (pIcode+1)->invalidate();
                forced_finish=true;        /* to exit the loop */
            }
            break;
        } /* eos */
    }
    return rev!=Icode.rend();
}
int Function::checkForwardLongDefs(int loc_ident_idx, const ID &pLocId, iICODE beg,Assignment &asgn)
{
    bool forced_finish=false;
    auto pEnd=Icode.end();
    for (auto pIcode=beg; not forced_finish and ((pIcode+1)!=Icode.end()); ++pIcode)
    {
        iICODE next1(pIcode+1);
        LLOperand * pmH,* pmL;            /* Pointers to dst LOW_LEVEL icodes */
        int off,arc;

        if ((pIcode->type == HIGH_LEVEL) || (pIcode->invalid == TRUE))
            continue;

        if (pIcode->ic.ll.opcode == next1->ic.ll.opcode)
            switch (pIcode->ic.ll.opcode)
            {
            case iMOV:
                if ((pLocId.id.longId.h == pIcode->ic.ll.src.regi) &&
                        (pLocId.id.longId.l == next1->ic.ll.src.regi))
                {
                    asgn.rhs = COND_EXPR::idLongIdx (loc_ident_idx);
                    pIcode->setRegDU( next1->ic.ll.src.regi, eUSE);
                    asgn.lhs = COND_EXPR::idLong (&this->localId, DST, pIcode,HIGH_FIRST, pIcode, eDEF, 1);
                    pIcode->setAsgn(asgn.lhs, asgn.rhs);
                    next1->invalidate();
                    forced_finish =true; /* to exit the loop */
                }
                break;

            case iPUSH:
                if ((pLocId.id.longId.h == pIcode->ic.ll.src.regi) &&
                        (pLocId.id.longId.l == next1->ic.ll.src.regi))
                {
                    asgn.rhs = COND_EXPR::idLongIdx (loc_ident_idx);
                    pIcode->setRegDU( next1->ic.ll.src.regi, eUSE);
                    pIcode->setUnary(HLI_PUSH, asgn.lhs);
                    next1->invalidate();
                }
                forced_finish =true; /* to exit the loop */
                break;

                /*** others missing ****/

            case iAND: case iOR: case iXOR:
                pmL = &pIcode->ic.ll.dst;
                pmH = &next1->ic.ll.dst;
                if ((pLocId.id.longId.h == pmH->regi) &&
                        (pLocId.id.longId.l == pmL->regi))
                {
                    asgn.lhs = COND_EXPR::idLongIdx (loc_ident_idx);
                    pIcode->setRegDU( pmH->regi, USE_DEF);
                    asgn.rhs = COND_EXPR::idLong (&this->localId, SRC, pIcode,
                                                  LOW_FIRST, pIcode/*idx*/, eUSE, 1);
                    switch (pIcode->ic.ll.opcode) {
                    case iAND: asgn.rhs = COND_EXPR::boolOp (asgn.lhs, asgn.rhs, AND);
                        break;
                    case iOR:  asgn.rhs = COND_EXPR::boolOp (asgn.lhs, asgn.rhs, OR);
                        break;
                    case iXOR: asgn.rhs = COND_EXPR::boolOp (asgn.lhs, asgn.rhs, XOR);
                        break;
                    }
                    pIcode->setAsgn(asgn.lhs, asgn.rhs);
                    (pIcode+1)->invalidate();
                    // ftw loop restart ????
                    //idx = 0;
                    // maybe this should end the loop instead
                    forced_finish =true; /* to exit the loop */
                }
                break;
            } /* eos */

        /* Check long conditional (i.e. 2 CMPs and 3 branches */
        else if ((pIcode->ic.ll.opcode == iCMP) && (isLong23 (pIcode, pIcode->inBB, &off, &arc)))
        {
            if (checkLongRegEq (pLocId.id.longId, pIcode, loc_ident_idx, this,
                                asgn.rhs, asgn.lhs, off) == TRUE)
            {
                pIcode += longJCond23 (asgn.rhs, asgn.lhs, pIcode, arc, off);
            }
        }

        /* Check for long conditional equality or inequality.  This requires
             * 2 CMPs and 2 branches */
        else if ((pIcode->ic.ll.opcode == iCMP) &&
                 (isLong22 (pIcode, pEnd, &off)))
        {
            if (checkLongRegEq (pLocId.id.longId, pIcode, loc_ident_idx, this,
                                asgn.rhs, asgn.lhs, off) == TRUE)
            {
                pIcode += longJCond22 (asgn.rhs, asgn.lhs, pIcode);
            }
        }

        /* Check for OR regH, regL
             *			 JX lab
             *		=> HLI_JCOND (regH:regL X 0) lab
             * This is better code than HLI_JCOND (HI(regH:regL) | LO(regH:regL)) */
        else if ((pIcode->ic.ll.opcode == iOR) && (next1 < pEnd) &&
                 (isJCond (next1->ic.ll.opcode)))
        {
            if ((pIcode->ic.ll.dst.regi == pLocId.id.longId.h) &&
                    (pIcode->ic.ll.src.regi == pLocId.id.longId.l))
            {
                asgn.lhs = COND_EXPR::idLongIdx (loc_ident_idx);

                asgn.rhs = COND_EXPR::idKte (0, 4);	/* long 0 */
                asgn.lhs = COND_EXPR::boolOp (asgn.lhs, asgn.rhs, condOpJCond[next1->ic.ll.opcode - iJB]);
                next1->setJCond(asgn.lhs);
                next1->copyDU(*pIcode, eUSE, eUSE);
                pIcode->invalidate();
            }
        }

    } /* end for */
}

/* Finds the definition of the long register pointed to by pLocId, and
 * transforms that instruction into a HIGH_LEVEL icode instruction.
 * Arguments: i     : index into the local identifier table
 *            pLocId: ptr to the long local identifier
 *            pProc : ptr to current procedure's record.        */
void Function::propLongReg (Int loc_ident_idx, const ID *pLocId)
{
    Int idx;
    iICODE pEnd;

    /* Process all definitions/uses of long registers at an icode position */
    pEnd = this->Icode.end();
    const IDX_ARRAY &lidx(pLocId->idx);
    //    for (int pLocId_idx : pLocId->idx)
    // WARNING: this loop modifies the iterated-over container.
    for (int j = 0; j < pLocId->idx.size(); j++)
    {
        auto idx_iter=lidx.begin();
        std::advance(idx_iter,j);
        //assert(*idx_iter==lidx.z[j]);
        int pLocId_idx=std::distance(Icode.begin(),*idx_iter);
        Assignment asgn;
        /* Check backwards for a definition of this long register */
        if (checkBackwarLongDefs(loc_ident_idx,*pLocId,*idx_iter,asgn))
            continue;
        /* If no definition backwards, check forward for a use of this long reg */
        checkForwardLongDefs(loc_ident_idx,*pLocId,*idx_iter,asgn);
    } /* end for */
}


/* Propagates the long global address across all LOW_LEVEL icodes.
 * Transforms some LOW_LEVEL icodes into HIGH_LEVEL     */
void Function::propLongGlb (Int i, ID *pLocId)
{
    printf("WARN: Function::propLongGlb not implemented");
}


/* Propagated identifier information, thus converting some LOW_LEVEL icodes
 * into HIGH_LEVEL icodes.  */
void Function::propLong()
{
    Int i;
    ID *pLocId;           /* Pointer to current local identifier */

    for (i = 0; i < localId.csym(); i++)
    {
        pLocId = &localId.id_arr[i];
        if ((pLocId->type==TYPE_LONG_SIGN) || (pLocId->type==TYPE_LONG_UNSIGN))
        {
            switch (pLocId->loc)
            {
            case STK_FRAME:
                propLongStk (i, pLocId);
                break;
            case REG_FRAME:
                propLongReg (i, pLocId);
                break;
            case GLB_FRAME:
                propLongGlb (i, pLocId);
                break;
            }
        }
    }
}

