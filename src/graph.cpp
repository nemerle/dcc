/*****************************************************************************
 * 			dcc project CFG related functions
 * (C) Cristina Cifuentes
 ****************************************************************************/

#include "dcc.h"
#include <string.h>
#if __BORLAND__
#include <alloc.h>
#else
#include <malloc.h>		/* For free() */
#endif
#include "graph.h"

//static BB *  rmJMP(Function * pProc, Int marker, BB * pBB);
static void mergeFallThrough(Function * pProc, BB * pBB);
static void dfsNumbering(BB * pBB, std::vector<BB*> &dfsLast, Int *first, Int *last);

/*****************************************************************************
 * createCFG - Create the basic control flow graph
 ****************************************************************************/
void Function::createCFG()
{
    /* Splits Icode associated with the procedure into Basic Blocks.
     * The links between BBs represent the control flow graph of the
     * procedure.
     * A Basic Block is defined to end on one of the following instructions:
     * 1) Conditional and unconditional jumps
     * 2) CALL(F)
     * 3) RET(F)
     * 4) On the instruction before a join (a flagged TARGET)
     * 5) Repeated string instructions
     * 6) End of procedure
     */
    Int		i;
    Int		ip, start;
    BB *        psBB;
    BB *        pBB;
    iICODE 	pIcode = Icode.begin();

    stats.numBBbef = stats.numBBaft = 0;
    for (ip = start = 0; pIcode!=Icode.end(); ip++, pIcode++)
    {
        /* Stick a NOWHERE_NODE on the end if we terminate
                 * with anything other than a ret, jump or terminate */
        if (ip + 1 == Icode.size() &&
                ! (pIcode->ic.ll.flg & TERMINATES) &&
                pIcode->ic.ll.opcode != iJMP && pIcode->ic.ll.opcode != iJMPF &&
                pIcode->ic.ll.opcode != iRET && pIcode->ic.ll.opcode != iRETF)
        {
            pBB=BB::Create(start, ip, NOWHERE_NODE, 0, this);
        }

        /* Only process icodes that have valid instructions */
        else if ((pIcode->ic.ll.flg & NO_CODE) != NO_CODE)
        {
            switch (pIcode->ic.ll.opcode) {
                case iJB:  case iJBE:  case iJAE:  case iJA:
                case iJL:  case iJLE:  case iJGE:  case iJG:
                case iJE:  case iJNE:  case iJS:   case iJNS:
                case iJO:  case iJNO:  case iJP:   case iJNP:
                case iJCXZ:
                    pBB = BB::Create(start, ip, TWO_BRANCH, 2, this);
CondJumps:
                    start = ip + 1;
                    pBB->edges[0].ip = (dword)start;
                    /* This is for jumps off into nowhere */
                    if (pIcode->ic.ll.flg & NO_LABEL)
                    {
                        pBB->edges.pop_back();
                    }
                    else
                        pBB->edges[1].ip = pIcode->ic.ll.src.op();
                    break;

                case iLOOP: case iLOOPE: case iLOOPNE:
                    pBB = BB::Create(start, ip, LOOP_NODE, 2, this);
                    goto CondJumps;

                case iJMPF: case iJMP:
                    if (pIcode->ic.ll.flg & SWITCH)
                    {
                        pBB = BB::Create(start, ip, MULTI_BRANCH, pIcode->ic.ll.caseTbl.numEntries, this);
                        for (i = 0; i < pIcode->ic.ll.caseTbl.numEntries; i++)
                            pBB->edges[i].ip = pIcode->ic.ll.caseTbl.entries[i];
                        hasCase = TRUE;
                    }
                    else if ((pIcode->ic.ll.flg & (I | NO_LABEL)) == I)
                    {
                        pBB = BB::Create(start, ip, ONE_BRANCH, 1, this);
                        pBB->edges[0].ip = pIcode->ic.ll.src.op();
                    }
                    else
                        BB::Create(start, ip, NOWHERE_NODE, 0, this);
                    start = ip + 1;
                    break;

                case iCALLF: case iCALL:
                {
                    Function * p = pIcode->ic.ll.src.proc.proc;
                    if (p)
                        i = ((p->flg) & TERMINATES) ? 0 : 1;
                    else
                        i = 1;
                    pBB = BB::Create(start, ip, CALL_NODE, i, this);
                    start = ip + 1;
                    if (i)
                        pBB->edges[0].ip = (dword)start;
                }
                    break;

                case iRET:  case iRETF:
                    BB::Create(start, ip, RETURN_NODE, 0, this);
                    start = ip + 1;
                    break;

                default:
                    /* Check for exit to DOS */
                    if (pIcode->ic.ll.flg & TERMINATES)
                    {
                        pBB = BB::Create(start, ip, TERMINATE_NODE, 0, this);
                        start = ip + 1;
                    }
                    /* Check for a fall through */
                    else if (Icode[ip + 1].ic.ll.flg & (TARGET | CASE))
                    {
                        pBB = BB::Create(start, ip, FALL_NODE, 1, this);
                        start = ip + 1;
                        pBB->edges[0].ip = (dword)start;
                    }
                    break;
            }
        }
    }
    std::vector<BB *>::iterator iter=heldBBs.begin();
    /* Convert list of BBs into a graph */
    for (; iter!=heldBBs.end(); ++iter)
    {
        pBB = *iter;
        for (i = 0; i < pBB->edges.size(); i++)
        {
            ip = pBB->edges[i].ip;
            if (ip >= SYNTHESIZED_MIN)
            {
                fatalError (INVALID_SYNTHETIC_BB);
                return ;
            }
            auto iter2=std::find_if(heldBBs.begin(),heldBBs.end(),
                         [ip](BB *psBB)->bool {return psBB->begin()==ip;});
            if(iter2==heldBBs.end())
                fatalError(NO_BB, ip, name.c_str());
            psBB = *iter2;
            pBB->edges[i].BBptr = psBB;
            psBB->inEdges.push_back(0);
        }
    }
}

void Function::markImpure()
{
    SYM * psym;
    for(ICODE &icod : Icode)
    {
        if ( not icod.isLlFlag(SYM_USE | SYM_DEF))
            continue;
        psym = &symtab[icod.ic.ll.caseTbl.numEntries];
        for (int c = (Int)psym->label; c < (Int)psym->label+psym->size; c++)
        {
            if (BITMAP(c, BM_CODE))
            {
                icod.SetLlFlag(IMPURE);
                flg |= IMPURE;
                break;
            }
        }
    }

}



/*****************************************************************************
 * newBB - Allocate new BB and link to end of list
 *****************************************************************************/

/*****************************************************************************
 * freeCFG - Deallocates a cfg
 ****************************************************************************/
void Function::freeCFG()
{
    for(BB *p : heldBBs)
        delete p;
}


/*****************************************************************************
 * compressCFG - Remove redundancies and add in-edge information
 ****************************************************************************/
void Function::compressCFG()
{
    BB * pBB, *pNxt;
    Int	ip, first=0, last, i;

    /* First pass over BB list removes redundant jumps of the form
         * (Un)Conditional -> Unconditional jump  */
    std::vector<BB*>::iterator iter=cfg.begin();
    for (;iter!=cfg.end(); ++iter)
    {
        pBB = *iter;
        if(pBB->inEdges.empty() || (pBB->nodeType != ONE_BRANCH && pBB->nodeType != TWO_BRANCH))
            continue;
        for (i = 0; i < pBB->edges.size(); i++)
        {
            ip   = pBB->rbegin();
            pNxt = pBB->edges[i].BBptr->rmJMP(ip, pBB->edges[i].BBptr);

            if (not pBB->edges.empty())   /* Might have been clobbered */
            {
                pBB->edges[i].BBptr = pNxt;
                Icode[ip].SetImmediateOp((dword)pNxt->begin());
            }
        }
    }

    /* Next is a depth-first traversal merging any FALL_NODE or
     * ONE_BRANCH that fall through to a node with that as their only
     * in-edge. */
    this->cfg.front()->mergeFallThrough(Icode);

    /* Remove redundant BBs created by the above compressions
     * and allocate in-edge arrays as required. */
    stats.numBBaft = stats.numBBbef;

    for(auto iter=cfg.begin(); iter!=cfg.end(); ++iter)
    {
        pBB = *iter;
        if (pBB->inEdges.empty())
        {
            if (iter == cfg.begin())	/* Init it misses out on */
                pBB->index = UN_INIT;
            else
            {
                delete pBB;
                stats.numBBaft--;
            }
        }
        else
        {
            pBB->inEdgeCount = pBB->inEdges.size();
        }
    }

    /* Allocate storage for dfsLast[] array */
    numBBs = stats.numBBaft;
    dfsLast.resize(numBBs,0); // = (BB **)allocMem(numBBs * sizeof(BB *))

    /* Now do a dfs numbering traversal and fill in the inEdges[] array */
    last = numBBs - 1;
    cfg.front()->dfsNumbering(dfsLast, &first, &last);
}


/****************************************************************************
 * rmJMP - If BB addressed is just a JMP it is replaced with its target
 ***************************************************************************/
BB *BB::rmJMP(Int marker, BB * pBB)
{
    marker += DFS_JMP;

    while (pBB->nodeType == ONE_BRANCH && pBB->size() == 1)
    {
        if (pBB->traversed != marker)
        {
            pBB->traversed = marker;
            pBB->inEdges.pop_back();
            if (not pBB->inEdges.empty())
            {
                pBB->edges[0].BBptr->inEdges.push_back(0);
            }
            else
            {
                pBB->front().SetLlFlag(NO_CODE);
                pBB->front().invalidate(); //pProc->Icode.SetLlInvalid(pBB->begin(), TRUE);
            }

            pBB = pBB->edges[0].BBptr;
        }
        else
        {
            /* We are going around in circles */
            pBB->nodeType = NOWHERE_NODE;
            pBB->front().ic.ll.src.SetImmediateOp(pBB->front().loc_ip);
            //pBB->front().ic.ll.src.immed.op = pBB->front().loc_ip;
            do {
                pBB = pBB->edges[0].BBptr;
                pBB->inEdges.pop_back(); // was --numInedges
                if (! pBB->inEdges.empty())
                {
                    pBB->front().SetLlFlag(NO_CODE);
                    pBB->front().invalidate();
//                    pProc->Icode.SetLlFlag(pBB->start, NO_CODE);
//                    pProc->Icode.SetLlInvalid(pBB->start, TRUE);
                }
            } while (pBB->nodeType != NOWHERE_NODE);

            pBB->edges.clear();
        }
    }
    return pBB;
}


/*****************************************************************************
 * mergeFallThrough
 ****************************************************************************/
void BB::mergeFallThrough( CIcodeRec &Icode)
{
    BB *	pChild;
    Int	i;

    if (!this)
    {
        printf("mergeFallThrough on empty BB!\n");
    }
    while (nodeType == FALL_NODE || nodeType == ONE_BRANCH)
    {
        pChild = edges[0].BBptr;
        /* Jump to next instruction can always be removed */
        if (nodeType == ONE_BRANCH)
        {
            assert(Parent==pChild->Parent);
            if(back().loc_ip>pChild->front().loc_ip) // back edege
                break;
            auto iter=std::find_if(this->end2(),pChild->begin2(),[](ICODE &c)
                {return not c.isLlFlag(NO_CODE);});

            if (iter != pChild->begin2())
                break;
            back().SetLlFlag(NO_CODE);
            back().invalidate();
            nodeType = FALL_NODE;
            length--;

        }
        /* If there's no other edges into child can merge */
        if (pChild->inEdges.size() != 1)
            break;

        nodeType = pChild->nodeType;
        length = (pChild->start - start) + pChild->length ;
        pChild->front().ClrLlFlag(TARGET);
        edges.swap(pChild->edges);

        pChild->inEdges.clear();
        pChild->edges.clear();
    }
    traversed = DFS_MERGE;

    /* Process all out edges recursively */
    for (i = 0; i < edges.size(); i++)
        if (edges[i].BBptr->traversed != DFS_MERGE)
            edges[i].BBptr->mergeFallThrough(Icode);
}


/*****************************************************************************
 * dfsNumbering - Numbers nodes during first and last visits and determine
 * in-edges
 ****************************************************************************/
void BB::dfsNumbering(std::vector<BB *> &dfsLast, Int *first, Int *last)
{
    BB *		pChild;
    traversed = DFS_NUM;
    dfsFirstNum = (*first)++;

    /* index is being used as an index to inEdges[]. */
//    for (i = 0; i < edges.size(); i++)
    for (auto edge : edges)
    {
        pChild = edge.BBptr;
        pChild->inEdges[pChild->index++] = this;

        /* Is this the last visit? */
        if (pChild->index == pChild->inEdges.size())
            pChild->index = UN_INIT;

        if (pChild->traversed != DFS_NUM)
            pChild->dfsNumbering(dfsLast, first, last);
    }
    dfsLastNum = *last;
    dfsLast[(*last)--] = this;
}
