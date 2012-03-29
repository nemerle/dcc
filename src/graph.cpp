/*****************************************************************************
 * 			dcc project CFG related functions
 * (C) Cristina Cifuentes
 ****************************************************************************/

#include "dcc.h"
#include <string.h>
#include <malloc.h>		/* For free() */
#include "graph.h"
#include "project.h"
extern Project g_proj;
//static BB *  rmJMP(Function * pProc, int marker, BB * pBB);
//static void mergeFallThrough(Function * pProc, BB * pBB);
//static void dfsNumbering(BB * pBB, std::vector<BB*> &dfsLast, int *first, int *last);

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
    int		i;
    BB *        psBB;
    BB *        pBB;
    iICODE 	pIcode = Icode.begin();
    iICODE      iStart = Icode.begin();
    stats.numBBbef = stats.numBBaft = 0;
    for (; pIcode!=Icode.end(); ++pIcode)
    {
        iICODE nextIcode = ++iICODE(pIcode);
        LLInst *ll = pIcode->ll();
        /* Stick a NOWHERE_NODE on the end if we terminate
                 * with anything other than a ret, jump or terminate */
        if (nextIcode == Icode.end() and
                (not ll->testFlags(TERMINATES)) and
                (not ll->match(iJMP)) and (not ll->match(iJMPF)) and
                (not ll->match(iRET)) and (not ll->match(iRETF)))
        {
            //pBB=BB::Create(start, ip, NOWHERE_NODE, 0, this);
            pBB=BB::Create(iStart, pIcode, NOWHERE_NODE, 0, this);

        }

        /* Only process icodes that have valid instructions */
        else if (not ll->testFlags(NO_CODE) )
        {
            switch (ll->getOpcode()) {
                case iJB:  case iJBE:  case iJAE:  case iJA:
                case iJL:  case iJLE:  case iJGE:  case iJG:
                case iJE:  case iJNE:  case iJS:   case iJNS:
                case iJO:  case iJNO:  case iJP:   case iJNP:
                case iJCXZ:
                    pBB = BB::Create(iStart, pIcode, TWO_BRANCH, 2, this);
CondJumps:
                    //start = ip + 1;
                    iStart = ++iICODE(pIcode);
                    pBB->edges[0].ip = (uint32_t)iStart->loc_ip;
                    /* This is for jumps off into nowhere */
                    if ( ll->testFlags(NO_LABEL) )
                    {
                        pBB->edges.pop_back();
                    }
                    else
                        pBB->edges[1].ip = ll->src().getImm2();
                    break;

                case iLOOP: case iLOOPE: case iLOOPNE:
                    //pBB = BB::Create(start, ip, LOOP_NODE, 2, this);
                    pBB = BB::Create(iStart, pIcode, LOOP_NODE, 2, this);
                    goto CondJumps;

                case iJMPF: case iJMP:
                    if (ll->testFlags(SWITCH))
                    {
                        //pBB = BB::Create(start, ip, MULTI_BRANCH, ll->caseTbl.numEntries, this);
                        pBB = BB::Create(iStart, pIcode, MULTI_BRANCH, ll->caseTbl2.size(), this);
                        for (size_t i = 0; i < ll->caseTbl2.size(); i++)
                            pBB->edges[i].ip = ll->caseTbl2[i];
                        hasCase = true;
                    }
                    else if ((ll->getFlag() & (I | NO_LABEL)) == I) //TODO: WHY NO_LABEL TESTIT
                    {
                        //pBB = BB::Create(start, ip, ONE_BRANCH, 1, this);
                        pBB = BB::Create(iStart, pIcode, ONE_BRANCH, 1, this);

                        pBB->edges[0].ip = ll->src().getImm2();
                    }
                    else
                        BB::Create(iStart, pIcode,  NOWHERE_NODE, 0, this);
                    iStart = ++iICODE(pIcode);
                    break;

                case iCALLF: case iCALL:
                {
                    Function * p = ll->src().proc.proc;
                    if (p)
                        i = ((p->flg) & TERMINATES) ? 0 : 1;
                    else
                        i = 1;
                    pBB = BB::Create(iStart, pIcode,  CALL_NODE, i, this);
                    iStart = ++iICODE(pIcode);//start = ip + 1;
                    if (i)
                        pBB->edges[0].ip = iStart->loc_ip;//(uint32_t)start;
                }
                    break;

                case iRET:  case iRETF:
                    //BB::Create(start, ip, RETURN_NODE, 0, this);
                    BB::Create(iStart, pIcode,  RETURN_NODE, 0, this);
                    iStart = ++iICODE(pIcode);
                    break;

                default:
                    /* Check for exit to DOS */
                    iICODE next1=++iICODE(pIcode);
                    if ( ll->testFlags(TERMINATES) )
                    {
                        pBB = BB::Create(iStart, pIcode,  TERMINATE_NODE, 0, this);
                        //pBB = BB::Create(start, ip, TERMINATE_NODE, 0, this);
                        iStart = ++iICODE(pIcode); // start = ip + 1;
                    }
                    /* Check for a fall through */
                    else if (next1 != Icode.end())
                    {
                        if (next1->ll()->testFlags(TARGET | CASE))
                        {
                            //pBB = BB::Create(start, ip, FALL_NODE, 1, this);
                            pBB = BB::Create(iStart, pIcode,  FALL_NODE, 1, this);
                            iStart = ++iICODE(pIcode); // start = ip + 1;
                            pBB->addOutEdge(iStart->loc_ip);
                            pBB->edges[0].ip = iStart->loc_ip;//(uint32_t)start;
                        }
                    }
                    break;
            }
        }
    }
    auto iter=heldBBs.begin();
    /* Convert list of BBs into a graph */
    for (; iter!=heldBBs.end(); ++iter)
    {
        pBB = *iter;
        for (size_t edeg_idx = 0; edeg_idx < pBB->edges.size(); edeg_idx++)
        {
            int32_t ip = pBB->edges[edeg_idx].ip;
            if (ip >= SYNTHESIZED_MIN)
            {
                fatalError (INVALID_SYNTHETIC_BB);
                return;
            }
            auto iter2=std::find_if(heldBBs.begin(),heldBBs.end(),
                                    [ip](BB *psBB)->bool {return psBB->begin()->loc_ip==ip;});
            if(iter2==heldBBs.end())
                fatalError(NO_BB, ip, name.c_str());
            psBB = *iter2;
            pBB->edges[edeg_idx].BBptr = psBB;
            psBB->inEdges.push_back((BB *)nullptr);
        }
    }
}

void Function::markImpure()
{
    PROG &prog(Project::get()->prog);
    for(ICODE &icod : Icode)
    {
        if ( not icod.ll()->testFlags(SYM_USE | SYM_DEF))
            continue;
        //assert that case tbl has less entries then symbol table ????
        //WARNING: Case entries are held in symbol table !
        assert(Project::get()->validSymIdx(icod.ll()->caseEntry));
        const SYM &psym(Project::get()->getSymByIdx(icod.ll()->caseEntry));
        for (int c = (int)psym.label; c < (int)psym.label+psym.size; c++)
        {
            if (BITMAP(c, BM_CODE))
            {
                icod.ll()->setFlags(IMPURE);
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
    {
        delete p;
    }
}


/*****************************************************************************
 * compressCFG - Remove redundancies and add in-edge information
 ****************************************************************************/
void Function::compressCFG()
{
    BB *pNxt;
    int	ip, first=0, last;

    /* First pass over BB list removes redundant jumps of the form
         * (Un)Conditional -> Unconditional jump  */
    for (BB *pBB : m_cfg)
    {
        if(pBB->inEdges.empty() || (pBB->nodeType != ONE_BRANCH && pBB->nodeType != TWO_BRANCH))
            continue;
        for (TYPEADR_TYPE &edgeRef : pBB->edges)
        {
            ip   = pBB->rbegin()->loc_ip;
            pNxt = edgeRef.BBptr->rmJMP(ip, edgeRef.BBptr);

            if (not pBB->edges.empty())   /* Might have been clobbered */
            {
                edgeRef.BBptr = pNxt;
                assert(pBB->back().loc_ip==ip);
                pBB->back().ll()->SetImmediateOp((uint32_t)pNxt->begin()->loc_ip);
                //Icode[ip].SetImmediateOp((uint32_t)pNxt->begin());
            }
        }
    }

    /* Next is a depth-first traversal merging any FALL_NODE or
     * ONE_BRANCH that fall through to a node with that as their only
     * in-edge. */
    m_cfg.front()->mergeFallThrough(Icode);

    /* Remove redundant BBs created by the above compressions
     * and allocate in-edge arrays as required. */
    stats.numBBaft = stats.numBBbef;

    for(auto iter=m_cfg.begin(); iter!=m_cfg.end(); ++iter)
    {
        BB * pBB = *iter;
        if (pBB->inEdges.empty())
        {
            if (iter == m_cfg.begin())	/* Init it misses out on */
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
    m_dfsLast.resize(numBBs,0); // = (BB **)allocMem(numBBs * sizeof(BB *))

    /* Now do a dfs numbering traversal and fill in the inEdges[] array */
    last = numBBs - 1;
    m_cfg.front()->dfsNumbering(m_dfsLast, &first, &last);
}


/****************************************************************************
 * rmJMP - If BB addressed is just a JMP it is replaced with its target
 ***************************************************************************/
BB *BB::rmJMP(int marker, BB * pBB)
{
    marker += (int)DFS_JMP;

    while (pBB->nodeType == ONE_BRANCH && pBB->size() == 1)
    {
        if (pBB->traversed != marker)
        {
            pBB->traversed = (eDFS)marker;
            pBB->inEdges.pop_back();
            if (not pBB->inEdges.empty())
            {
                pBB->edges[0].BBptr->inEdges.push_back((BB *)nullptr);
            }
            else
            {
                pBB->front().ll()->setFlags(NO_CODE);
                pBB->front().invalidate(); //pProc->Icode.SetLlInvalid(pBB->begin(), true);
            }

            pBB = pBB->edges[0].BBptr;
        }
        else
        {
            /* We are going around in circles */
            pBB->nodeType = NOWHERE_NODE;
            pBB->front().ll()->replaceSrc(LLOperand::CreateImm2(pBB->front().loc_ip));
            //pBB->front().ll()->src.immed.op = pBB->front().loc_ip;
            do {
                pBB = pBB->edges[0].BBptr;
                pBB->inEdges.pop_back(); // was --numInedges
                if (! pBB->inEdges.empty())
                {
                    pBB->front().ll()->setFlags(NO_CODE);
                    pBB->front().invalidate();
                    //                    pProc->Icode.setFlags(pBB->start, NO_CODE);
                    //                    pProc->Icode.SetLlInvalid(pBB->start, true);
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
            auto iter=std::find_if(this->end(),pChild->begin(),[](ICODE &c)
            {return not c.ll()->testFlags(NO_CODE);});

            if (iter != pChild->begin())
                break;
            back().ll()->setFlags(NO_CODE);
            back().invalidate();
            nodeType = FALL_NODE;
            //instructions.advance_end(-1); //TODO: causes creation of empty BB
        }
        /* If there's no other edges into child can merge */
        if (pChild->inEdges.size() != 1)
            break;

        nodeType = pChild->nodeType;
        instructions = boost::make_iterator_range(begin(),pChild->end());
        pChild->front().ll()->clrFlags(TARGET);
        edges.swap(pChild->edges);

        pChild->inEdges.clear();
        pChild->edges.clear();
    }
    traversed = DFS_MERGE;

    /* Process all out edges recursively */
    for (size_t i = 0; i < edges.size(); i++)
    {
        if (edges[i].BBptr->traversed != DFS_MERGE)
            edges[i].BBptr->mergeFallThrough(Icode);
    }
}


/*****************************************************************************
 * dfsNumbering - Numbers nodes during first and last visits and determine
 * in-edges
 ****************************************************************************/
void BB::dfsNumbering(std::vector<BB *> &dfsLast, int *first, int *last)
{
    BB *		pChild;
    traversed = DFS_NUM;
    dfsFirstNum = (*first)++;

    /* index is being used as an index to inEdges[]. */
    //    for (i = 0; i < edges.size(); i++)
    for(auto edge : edges)
    {
        pChild = edge.BBptr;
        pChild->inEdges[pChild->index++] = this;

        /* Is this the last visit? */
        if (pChild->index == int(pChild->inEdges.size()))
            pChild->index = UN_INIT;

        if (pChild->traversed != DFS_NUM)
            pChild->dfsNumbering(dfsLast, first, last);
    }
    dfsLastNum = *last;
    dfsLast[(*last)--] = this;
}
