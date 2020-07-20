#include "BasicBlock.h"

#include "msvc_fixes.h"
#include "Procedure.h"
#include "dcc.h"
#include "msvc_fixes.h"

#include <QtCore/QTextStream>
#include <cassert>
#include <string>
#include <boost/range/rbegin.hpp>
#include <boost/range/rend.hpp>

using namespace std;
using namespace boost;

BB *BB::Create(void */*ctx*/, const string &/*s*/, Function *parent, BB */*insertBefore*/)
{
    BB *pnewBB = new BB;
    pnewBB->Parent = parent;
    return pnewBB;
}
/**
 *  @arg start - basic block starts here, might be parent->Icode.end()
 *  @arg fin - last of basic block's instructions
*/
BB *BB::Create(const rCODE &r,eBBKind _nodeType, Function *parent)
{
    BB* pnewBB;
    pnewBB = new BB;
    pnewBB->nodeType = _nodeType;	/* Initialise */
    pnewBB->immedDom = NO_DOM;
    pnewBB->loopHead = pnewBB->caseHead = pnewBB->caseTail =
    pnewBB->latchNode= pnewBB->loopFollow = NO_NODE;
    pnewBB->instructions = r;
    /* Mark the basic block to which the icodes belong to, but only for
     * real code basic blocks (ie. not interval bbs) */
    if(parent)
    {
        int addr = pnewBB->begin()->loc_ip;
        //setInBB should automatically handle if our range is empty
        parent->Icode.SetInBB(pnewBB->instructions, pnewBB);

        assert(parent->m_ip_to_bb.find(addr)==parent->m_ip_to_bb.end());
        parent->m_ip_to_bb[addr] = pnewBB;
        parent->m_actual_cfg.push_back(pnewBB);
        pnewBB->Parent = parent;

    if ( r.begin() != parent->Icode.end() )		/* Only for code BB's */
        stats.numBBbef++;
    }
    return pnewBB;

}
BB *BB::CreateIntervalBB(Function *parent)
{
    iICODE endOfParent = parent->Icode.end();
    return Create(make_iterator_range(endOfParent,endOfParent),INTERVAL_NODE,nullptr);
}

static const char *const s_nodeType[] = {"branch", "if", "case", "fall", "return", "call",
                                 "loop", "repeat", "interval", "cycleHead",
                                 "caseHead", "terminate",
                                 "nowhere" };

static const char *const s_loopType[] = {"noLoop", "while", "repeat", "loop", "for"};


void BB::display()
{
    printf("\nnode type = %s, ", s_nodeType[nodeType]);
    printf("start = %d, length = %zd, #out edges = %zd\n", begin()->loc_ip, size(), edges.size());

    for (size_t i = 0; i < edges.size(); i++)
    {
        if(edges[i].BBptr==nullptr)
            printf(" outEdge[%2zd] = Unlinked out edge to %d\n",i, edges[i].ip);
        else
            printf(" outEdge[%2zd] = %d\n",i, edges[i].BBptr->begin()->loc_ip);
    }
}
/*****************************************************************************
 * displayDfs - Displays the CFG using a depth first traversal
 ****************************************************************************/
void BB::displayDfs()
{
    int i;
    assert(this);
    traversed = DFS_DISP;

    printf("node type = %s, ", s_nodeType[nodeType]);
    printf("start = %d, length = %zd, #in-edges = %zd, #out-edges = %zd\n",
           begin()->loc_ip, size(), inEdges.size(), edges.size());
    printf("dfsFirst = %d, dfsLast = %d, immed dom = %d\n",
           dfsFirstNum, dfsLastNum,
           immedDom == MAX ? -1 : immedDom);
    printf("loopType = %s, loopHead = %d, latchNode = %d, follow = %d\n",
           s_loopType[(int)loopType],
           loopHead == MAX ? -1 : loopHead,
           latchNode == MAX ? -1 : latchNode,
           loopFollow == MAX ? -1 : loopFollow);
    printf ("ifFollow = %d, caseHead = %d, caseTail = %d\n",
            ifFollow == MAX ? -1 : ifFollow,
            caseHead == MAX ? -1 : caseHead,
            caseTail == MAX ? -1 : caseTail);

    if (nodeType == INTERVAL_NODE)
        printf("corresponding interval = %d\n", correspInt->numInt);
    else
    {
        int edge_idx=0;
        for(BB *node : inEdges)
        {
            printf ("  inEdge[%d] = %d\n", edge_idx, node->begin()->loc_ip);
            edge_idx++;
        }
    }
    /* Display out edges information */
    i=0;
    for(TYPEADR_TYPE &edg : edges)
    {
        if (nodeType == INTERVAL_NODE)
            printf(" outEdge[%d] = %d\n", i, edg.BBptr->correspInt->numInt);
        else
            printf(" outEdge[%d] = %d\n", i, edg.BBptr->begin()->loc_ip);
        ++i;
    }
    printf("----\n");

    /* Recursive call on successors of current node */
    for(TYPEADR_TYPE &pb : edges)
    {
        if (pb.BBptr->traversed != DFS_DISP)
            pb.BBptr->displayDfs();
    }
}
/** Recursive procedure that writes the code for the given procedure, pointed
  to by pBB.
  \param indLevel indentation level - used for formatting.
  \param numLoc: last # assigned to local variables
*/
ICODE* BB::writeLoopHeader(int &indLevel, Function* pProc, int *numLoc, BB *&latch, bool &repCond)
{
    if(loopType == eNodeHeaderType::NO_TYPE)
        return nullptr;
    latch = pProc->m_dfsLast[this->latchNode];
    QString ostr_contents;
    QTextStream ostr(&ostr_contents);
    ICODE* picode;
    switch (loopType)
    {
    case eNodeHeaderType::WHILE_TYPE:
            picode = &this->back();

            /* Check for error in while condition */
            if (picode->hl()->opcode != HLI_JCOND)
                reportError (WHILE_FAIL);

            /* Check if condition is more than 1 HL instruction */
            if (numHlIcodes > 1)
            {
                /* Write the code for this basic block */
                writeBB(ostr,indLevel, pProc, numLoc);
                repCond = true;
            }

            /* Condition needs to be inverted if the loop body is along
             * the THEN path of the header node */
            if (edges[ELSE].BBptr->dfsLastNum == loopFollow)
            {
                picode->hlU()->replaceExpr(picode->hl()->expr()->inverse());
            }
            {
                QString e=picode->hl()->expr()->walkCondExpr (pProc, numLoc);
                ostr << "\n"<<indentStr(indLevel)<<"while ("<<e<<") {\n";
            }
            picode->invalidate();
            break;

    case eNodeHeaderType::REPEAT_TYPE:
            ostr << "\n"<<indentStr(indLevel)<<"do {\n";
            picode = &latch->back();
            picode->invalidate();
            break;

    case eNodeHeaderType::ENDLESS_TYPE:
            ostr << "\n"<<indentStr(indLevel)<<"for (;;) {\n";
            picode = &latch->back();
        break;
    }
    ostr.flush();
    cCode.appendCode(ostr_contents);
    stats.numHLIcode += 1;
    indLevel++;
    return picode;
}
bool BB::isEndOfPath(int latch_node_idx) const
{
    return nodeType == RETURN_NODE or nodeType == TERMINATE_NODE or
           nodeType == NOWHERE_NODE or dfsLastNum == latch_node_idx;
}
void BB::writeCode (int indLevel, Function * pProc , int *numLoc,int _latchNode, int _ifFollow)
{
    int follow;						/* ifFollow                 	*/
    BB * succ, *latch;				/* Successor and latching node 	*/
    ICODE * picode;					/* Pointer to HLI_JCOND instruction	*/
    QString l;                      /* Pointer to HLI_JCOND expression	*/
    bool emptyThen,					/* THEN clause is empty			*/
            repCond;				/* Repeat condition for while() */

    /* Check if this basic block should be analysed */
    if ((_ifFollow != UN_INIT) and (this == pProc->m_dfsLast[_ifFollow]))
        return;

    if (wasTraversedAtLevel(DFS_ALPHA))
        return;
    traversed = DFS_ALPHA;

    /* Check for start of loop */
    repCond = false;
    latch = nullptr;
       picode=writeLoopHeader(indLevel, pProc, numLoc, latch, repCond);

    /* Write the code for this basic block */
    if (repCond == false)
    {
        QString ostr_contents;
        QTextStream ostr(&ostr_contents);
        writeBB(ostr,indLevel, pProc, numLoc);
        ostr.flush();
        cCode.appendCode(ostr_contents);
    }

    /* Check for end of path */
    if (isEndOfPath(_latchNode))
        return;

    /* Check type of loop/node and process code */
    if ( loopType!=eNodeHeaderType::NO_TYPE )	/* there is a loop */
    {
        assert(latch);
        if (this != latch)		/* loop is over several bbs */
        {
            if (loopType == eNodeHeaderType::WHILE_TYPE)
            {
                succ = edges[THEN].BBptr;
                if (succ->dfsLastNum == loopFollow)
                    succ = edges[ELSE].BBptr;
            }
            else
                succ = edges[0].BBptr;
            if (succ->traversed != DFS_ALPHA)
                succ->writeCode (indLevel, pProc, numLoc, latch->dfsLastNum,_ifFollow);
            else	/* has been traversed so we need a goto */
                succ->front().ll()->emitGotoLabel (indLevel);
        }

        /* Loop epilogue: generate the loop trailer */
        indLevel--;
        if (loopType == eNodeHeaderType::WHILE_TYPE)
        {
            QString ostr_contents;
            QTextStream ostr(&ostr_contents);
            /* Check if there is need to repeat other statements involved
                         * in while condition, then, emit the loop trailer */
            if (repCond)
            {
                writeBB(ostr,indLevel+1, pProc, numLoc);
            }
            ostr <<indentStr(indLevel)<< "}	/* end of while */\n";
            ostr.flush();
            cCode.appendCode(ostr_contents);
        }
        else if (loopType == eNodeHeaderType::ENDLESS_TYPE)
            cCode.appendCode( "%s}	/* end of loop */\n",indentStr(indLevel));
        else if (loopType == eNodeHeaderType::REPEAT_TYPE)
        {
            QString e = "//*failed*//";
            if (picode->hl()->opcode != HLI_JCOND)
            {
                reportError (REPEAT_FAIL);
            }
            else
            {
                e=picode->hl()->expr()->walkCondExpr (pProc, numLoc);
            }
            cCode.appendCode( "%s} while (%s);\n", indentStr(indLevel),qPrintable(e));
        }

        /* Recurse on the loop follow */
        if (loopFollow != MAX)
        {
            succ = pProc->m_dfsLast[loopFollow];
            if (succ->traversed != DFS_ALPHA)
                succ->writeCode (indLevel, pProc, numLoc, _latchNode, _ifFollow);
            else		/* has been traversed so we need a goto */
                succ->front().ll()->emitGotoLabel (indLevel);
        }
    }

    else		/* no loop, process nodeType of the graph */
    {
        if (nodeType == TWO_BRANCH)		/* if-then[-else] */
        {
            stats.numHLIcode++;
            indLevel++;
            emptyThen = false;

            if (ifFollow != MAX)		/* there is a follow */
            {
                /* process the THEN part */
                follow = ifFollow;
                succ = edges[THEN].BBptr;
                if (succ->traversed != DFS_ALPHA)	/* not visited */
                {
                    if (succ->dfsLastNum != follow)	/* THEN part */
                    {
                        l = writeJcond ( *back().hl(), pProc, numLoc);
                        cCode.appendCode( "\n%s%s", indentStr(indLevel-1), qPrintable(l));
                        succ->writeCode (indLevel, pProc, numLoc, _latchNode,follow);
                    }
                    else		/* empty THEN part => negate ELSE part */
                    {
                        l = writeJcondInv ( *back().hl(), pProc, numLoc);
                        cCode.appendCode( "\n%s%s", indentStr(indLevel-1), qPrintable(l));
                        edges[ELSE].BBptr->writeCode (indLevel, pProc, numLoc, _latchNode, follow);
                        emptyThen = true;
                    }
                }
                else	/* already visited => emit label */
                    succ->front().ll()->emitGotoLabel(indLevel);

                /* process the ELSE part */
                succ = edges[ELSE].BBptr;
                if (succ->traversed != DFS_ALPHA)		/* not visited */
                {
                    if (succ->dfsLastNum != follow)		/* ELSE part */
                    {
                        cCode.appendCode( "%s}\n%selse {\n",
                                          indentStr(indLevel-1), indentStr(indLevel - 1));
                        succ->writeCode (indLevel, pProc, numLoc, _latchNode, follow);
                    }
                    /* else (empty ELSE part) */
                }
                else if (not emptyThen) 	/* already visited => emit label */
                {
                    cCode.appendCode( "%s}\n%selse {\n",
                                      indentStr(indLevel-1), indentStr(indLevel - 1));
                    succ->front().ll()->emitGotoLabel (indLevel);
                }
                cCode.appendCode( "%s}\n", indentStr(--indLevel));

                /* Continue with the follow */
                succ = pProc->m_dfsLast[follow];
                if (succ->traversed != DFS_ALPHA)
                    succ->writeCode (indLevel, pProc, numLoc, _latchNode,_ifFollow);
            }
            else		/* no follow => if..then..else */
            {
                l = writeJcond ( *back().hl(), pProc, numLoc);
                cCode.appendCode( "%s%s", indentStr(indLevel-1), qPrintable(l));
                edges[THEN].BBptr->writeCode (indLevel, pProc, numLoc, _latchNode, _ifFollow);
                cCode.appendCode( "%s}\n%selse {\n", indentStr(indLevel-1), indentStr(indLevel - 1));
                edges[ELSE].BBptr->writeCode (indLevel, pProc, numLoc, _latchNode, _ifFollow);
                cCode.appendCode( "%s}\n", indentStr(--indLevel));
            }
        }

        else 	/* fall, call, 1w */
        {
            succ = edges[0].BBptr;		/* fall-through edge */
            assert(succ->size()>0);
            if (succ->traversed != DFS_ALPHA)
            {
                succ->writeCode (indLevel, pProc,numLoc, _latchNode,_ifFollow);
            }
        }
    }
}
/* Writes the code for the current basic block.
 * Args: pBB: pointer to the current basic block.
 *		 Icode: pointer to the array of icodes for current procedure.
 *		 lev: indentation level - used for formatting.	*/
void BB::writeBB(QTextStream &ostr,int lev, Function * pProc, int *numLoc)
{
    /* Save the index into the code table in case there is a later goto
     * into this instruction (first instruction of the BB) */
    front().ll()->codeIdx = cCode.code.nextIdx();

    /* Generate code for each hlicode that is not a HLI_JCOND */

    for(ICODE &pHli : instructions)
    {
        if ((pHli.type == HIGH_LEVEL_ICODE) and ( pHli.valid() )) //TODO: use filtering range here.
        {
            QString line = pHli.hl()->write1HlIcode(pProc, numLoc);
            if (not line.isEmpty())
            {
                ostr<<indentStr(lev)<<line;
                stats.numHLIcode++;
            }
            if (option.verbose)
                pHli.writeDU();
        }
    }
}

iICODE BB::begin()
{
    return instructions.begin();
}

iICODE BB::end() const
{
    return instructions.end();
}
ICODE &BB::back()
{
    return instructions.back();
}

size_t BB::size()
{
    return std::distance(instructions.begin(),instructions.end());
}

ICODE &BB::front()
{
    return instructions.front();
}

riICODE BB::rbegin()
{
    return riICODE( instructions.end() );
}
riICODE BB::rend()
{
    return riICODE( instructions.begin() );
}
