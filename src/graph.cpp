/*****************************************************************************
 * dcc project CFG related functions
 * (C) Cristina Cifuentes
 ****************************************************************************/

#include "graph.h"

#include "msvc_fixes.h"
#include "dcc.h"
#include "project.h"

#include <boost/range/rbegin.hpp>
#include <boost/range/rend.hpp>
#include <boost/range/adaptors.hpp>
#include <string.h>

using namespace std;
using namespace boost;
//static BB *  rmJMP(Function * pProc, int marker, BB * pBB);
//static void mergeFallThrough(Function * pProc, BB * pBB);
//static void dfsNumbering(BB * pBB, std::vector<BB*> &dfsLast, int *first, int *last);

void Function::addOutEdgesForConditionalJump(BB * pBB,int next_ip, LLInst *ll)
{
    pBB->addOutEdge(next_ip);
    /* This is checking for jumps off into nowhere */
    if ( not ll->testFlags(NO_LABEL) )
        pBB->addOutEdge(ll->src().getImm2());
}

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

    BB *        psBB;
    BB *        pBB;
    iICODE  pIcode = Icode.begin();

    stats.numBBbef = stats.numBBaft = 0;
    rICODE  current_range=make_iterator_range(pIcode,++iICODE(pIcode));
    for (; pIcode!=Icode.end(); ++pIcode,current_range.advance_end(1))
    {
        iICODE nextIcode = ++iICODE(pIcode);
        pBB = nullptr;

        LLInst *ll = pIcode->ll();
        /* Only process icodes that have valid instructions */
        if(ll->testFlags(NO_CODE))
            continue;
        /* Stick a NOWHERE_NODE on the end if we terminate
         * with anything other than a ret, jump or terminate */
        if (nextIcode == Icode.end() and
                (not ll->testFlags(TERMINATES)) and
                (not ll->match(iJMP)) and (not ll->match(iJMPF)) and
                (not ll->match(iRET)) and (not ll->match(iRETF)))
        {
            pBB=BB::Create(current_range, NOWHERE_NODE, this);
        }
        else
            switch (ll->getOpcode()) {
            case iJB:  case iJBE:  case iJAE:  case iJA:
            case iJL:  case iJLE:  case iJGE:  case iJG:
            case iJE:  case iJNE:  case iJS:   case iJNS:
            case iJO:  case iJNO:  case iJP:   case iJNP:
            case iJCXZ:
                pBB = BB::Create(current_range, TWO_BRANCH, this);
                addOutEdgesForConditionalJump(pBB,nextIcode->loc_ip, ll);
                break;

            case iLOOP: case iLOOPE: case iLOOPNE:
                pBB = BB::Create(current_range, LOOP_NODE, this);
                addOutEdgesForConditionalJump(pBB,nextIcode->loc_ip, ll);
                break;

            case iJMPF: case iJMP:
                if (ll->testFlags(SWITCH))
                {
                    pBB = BB::Create(current_range, MULTI_BRANCH, this);
                    for (auto & elem : ll->caseTbl2)
                        pBB->addOutEdge(elem);
                    hasCase = true;
                }
                else if ((ll->getFlag() & (I | NO_LABEL)) == I) //TODO: WHY NO_LABEL TESTIT
                {
                    pBB = BB::Create(current_range, ONE_BRANCH, this);
                    pBB->addOutEdge(ll->src().getImm2());
                }
                else
                    pBB = BB::Create(current_range,  NOWHERE_NODE, this);
                break;

            case iCALLF: case iCALL:
            {
                Function * p = ll->src().proc.proc;
                pBB = BB::Create(current_range,  CALL_NODE, this);
                if (p and not ((p->flg) & TERMINATES) )
                    pBB->addOutEdge(nextIcode->loc_ip);
                break;
            }

            case iRET:  case iRETF:
                pBB = BB::Create(current_range,  RETURN_NODE, this);
                break;

            default:
                /* Check for exit to DOS */
                if ( ll->testFlags(TERMINATES) )
                {
                    pBB = BB::Create(current_range,  TERMINATE_NODE, this);
                }
                /* Check for a fall through */
                else if (nextIcode != Icode.end())
                {
                    if (nextIcode->ll()->testFlags(TARGET | CASE))
                    {
                        pBB = BB::Create(current_range,  FALL_NODE, this);
                        pBB->addOutEdge(nextIcode->loc_ip);
                    }
                }
                break;
            }
        if(pBB!=nullptr) // created a new Basic block
        {
            // restart the range
            // end iterator will be updated by expression in for statement
            current_range=make_iterator_range(nextIcode,nextIcode);
        }
        if (nextIcode == Icode.end())
            break;
    }
    for (auto pr : m_ip_to_bb)
    {
        BB* pBB=pr.second;
        for (auto & elem : pBB->edges)
        {
            int32_t ip = elem.ip;
            if (ip >= SYNTHESIZED_MIN)
            {
                fatalError (INVALID_SYNTHETIC_BB);
                return;
            }
            auto iter2=m_ip_to_bb.find(ip);
            if(iter2==m_ip_to_bb.end())
                fatalError(NO_BB, ip, qPrintable(name));
            psBB = iter2->second;
            elem.BBptr = psBB;
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
 * freeCFG - Deallocates a cfg
 ****************************************************************************/
void Function::freeCFG()
{
    for(auto p : m_ip_to_bb)
    {
        delete p.second;
    }
    m_ip_to_bb.clear();
}


/*****************************************************************************
 * compressCFG - Remove redundancies and add in-edge information
 ****************************************************************************/
void Function::compressCFG()
{
    BB *pNxt;
    int ip, first=0, last;

    /* First pass over BB list removes redundant jumps of the form
         * (Un)Conditional -> Unconditional jump  */
    for (BB *pBB : m_actual_cfg) //m_cfg
    {
        if(pBB->inEdges.empty() or (pBB->nodeType != ONE_BRANCH and pBB->nodeType != TWO_BRANCH))
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
    m_actual_cfg.front()->mergeFallThrough(Icode);

    /* Remove redundant BBs created by the above compressions
     * and allocate in-edge arrays as required. */
    stats.numBBaft = stats.numBBbef;
    bool entry_node=true;
    for(BB *pBB : m_actual_cfg)
    {
        if (pBB->inEdges.empty())
        {
            if (entry_node)     /* Init it misses out on */
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
        entry_node=false;
    }

    /* Allocate storage for dfsLast[] array */
    numBBs = stats.numBBaft;
    m_dfsLast.resize(numBBs,nullptr); // = (BB **)allocMem(numBBs * sizeof(BB *))

    /* Now do a dfs numbering traversal and fill in the inEdges[] array */
    last = numBBs - 1;
    m_actual_cfg.front()->dfsNumbering(m_dfsLast, &first, &last);
}


/****************************************************************************
 * rmJMP - If BB addressed is just a JMP it is replaced with its target
 ***************************************************************************/
BB *BB::rmJMP(int marker, BB * pBB)
{
    marker += (int)DFS_JMP;

    while (pBB->nodeType == ONE_BRANCH and pBB->size() == 1)
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
                if (not pBB->inEdges.empty())
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
    BB *    pChild;
    if (nullptr==this)
    {
        printf("mergeFallThrough on empty BB!\n");
    }
    while (nodeType == FALL_NODE or nodeType == ONE_BRANCH)
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
    for (auto & elem : edges)
    {
        if (elem.BBptr->traversed != DFS_MERGE)
            elem.BBptr->mergeFallThrough(Icode);
    }
}


/*****************************************************************************
 * dfsNumbering - Numbers nodes during first and last visits and determine
 * in-edges
 ****************************************************************************/
void BB::dfsNumbering(std::vector<BB *> &dfsLast, int *first, int *last)
{
    BB * pChild;
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
