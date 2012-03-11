#include <cassert>
#include <string>

#include "BasicBlock.h"
#include "Procedure.h"
#include "dcc.h"
using namespace std;

BB *BB::Create(void *ctx, const string &s, Function *parent, BB *insertBefore)
{
    BB *pnewBB = new BB;
    pnewBB->Parent = parent;
    return pnewBB;
}

BB *BB::Create(int start, int ip, uint8_t _nodeType, int numOutEdges, Function *parent)
{
    BB* pnewBB;

    pnewBB = new BB;
    pnewBB->nodeType = _nodeType;	/* Initialise */
    pnewBB->immedDom = NO_DOM;
    pnewBB->loopHead = pnewBB->caseHead = pnewBB->caseTail =
            pnewBB->latchNode= pnewBB->loopFollow = NO_NODE;
    pnewBB->range_start = parent->Icode.begin();
    pnewBB->range_end = parent->Icode.begin();
    if(start!=-1)
    {
        advance(pnewBB->range_start,start);
        advance(pnewBB->range_end,ip+1);
    }
    else
    {
        pnewBB->range_end = parent->Icode.end();
        pnewBB->range_end = parent->Icode.end();
    }

    if (numOutEdges)
        pnewBB->edges.resize(numOutEdges);

    /* Mark the basic block to which the icodes belong to, but only for
         * real code basic blocks (ie. not interval bbs) */
    if(parent)
    {
        if (start >= 0)
            parent->Icode.SetInBB(start, ip, pnewBB);
        parent->heldBBs.push_back(pnewBB);
        parent->m_cfg.push_back(pnewBB);
        pnewBB->Parent = parent;
    }
    if (start != -1)		/* Only for code BB's */
        stats.numBBbef++;
    return pnewBB;
}

static const char *const s_nodeType[] = {"branch", "if", "case", "fall", "return", "call",
                                 "loop", "repeat", "interval", "cycleHead",
                                 "caseHead", "terminate",
                                 "nowhere" };

static const char *const s_loopType[] = {"noLoop", "while", "repeat", "loop", "for"};


void BB::display()
{
    printf("\nnode type = %s, ", s_nodeType[nodeType]);
    printf("start = %ld, length = %ld, #out edges = %ld\n", begin()->loc_ip, size(), edges.size());

    for (size_t i = 0; i < edges.size(); i++)
        printf(" outEdge[%2d] = %ld\n",i, edges[i].BBptr->begin()->loc_ip);
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
    printf("start = %ld, length = %ld, #in-edges = %ld, #out-edges = %ld\n",
           begin()->loc_ip, size(), inEdges.size(), edges.size());
    printf("dfsFirst = %ld, dfsLast = %ld, immed dom = %ld\n",
           dfsFirstNum, dfsLastNum,
           immedDom == MAX ? -1 : immedDom);
    printf("loopType = %s, loopHead = %ld, latchNode = %ld, follow = %ld\n",
           s_loopType[loopType],
           loopHead == MAX ? -1 : loopHead,
           latchNode == MAX ? -1 : latchNode,
           loopFollow == MAX ? -1 : loopFollow);
    printf ("ifFollow = %ld, caseHead = %ld, caseTail = %ld\n",
            ifFollow == MAX ? -1 : ifFollow,
            caseHead == MAX ? -1 : caseHead,
            caseTail == MAX ? -1 : caseTail);

    if (nodeType == INTERVAL_NODE)
        printf("corresponding interval = %ld\n", correspInt->numInt);
    else
    {
        int edge_idx=0;
        for(BB *node : inEdges)
        {
            printf ("  inEdge[%ld] = %ld\n", edge_idx, node->begin()->loc_ip);
            edge_idx++;
        }
    }
    /* Display out edges information */
    i=0;
    for(TYPEADR_TYPE &edg : edges)
    {
        if (nodeType == INTERVAL_NODE)
            printf(" outEdge[%ld] = %ld\n", i, edg.BBptr->correspInt->numInt);
        else
            printf(" outEdge[%d] = %ld\n", i, edg.BBptr->begin()->loc_ip);
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
/* Recursive procedure that writes the code for the given procedure, pointed
 * to by pBB.
 * Parameters:	pBB:	pointer to the cfg.
 *				Icode:	pointer to the Icode array for the cfg graph of the
 *						current procedure.
 *				indLevel: indentation level - used for formatting.
 *				numLoc: last # assigned to local variables 				*/
void BB::writeCode (int indLevel, Function * pProc , int *numLoc,int _latchNode, int _ifFollow)
{
    int follow,						/* ifFollow                 	*/
        _loopType, 					/* Type of loop, if any         */
        _nodeType;                                      /* Type of node                 */
    BB * succ, *latch;					/* Successor and latching node 	*/
    ICODE * picode;					/* Pointer to HLI_JCOND instruction	*/
    char *l;                                            /* Pointer to HLI_JCOND expression	*/
    boolT emptyThen,					/* THEN clause is empty			*/
            repCond;					/* Repeat condition for while() */

    /* Check if this basic block should be analysed */
    if ((_ifFollow != UN_INIT) && (this == pProc->m_dfsLast[_ifFollow]))
        return;

    if (traversed == DFS_ALPHA)
        return;
    traversed = DFS_ALPHA;

    /* Check for start of loop */
    repCond = FALSE;
    latch = NULL;
    _loopType = loopType;
    if (_loopType)
    {
        latch = pProc->m_dfsLast[this->latchNode];
        switch (_loopType)
        {
            case WHILE_TYPE:
                picode = &this->back();

                /* Check for error in while condition */
                if (picode->hl()->opcode != HLI_JCOND)
                    reportError (WHILE_FAIL);

                /* Check if condition is more than 1 HL instruction */
                if (numHlIcodes > 1)
                {
                    /* Write the code for this basic block */
                    writeBB(indLevel, pProc, numLoc);
                    repCond = true;
                }

                /* Condition needs to be inverted if the loop body is along
                 * the THEN path of the header node */
                if (edges[ELSE].BBptr->dfsLastNum == loopFollow)
                {
                    COND_EXPR *old_expr=picode->hl()->expr();
                    string e=walkCondExpr (old_expr, pProc, numLoc);
                    picode->hl()->expr(picode->hl()->expr()->inverse());
                    delete old_expr;
                }
                {
                    string e=walkCondExpr (picode->hl()->expr(), pProc, numLoc);
                    cCode.appendCode( "\n%swhile (%s) {\n", indentStr(indLevel),e.c_str());
                }
                picode->invalidate();
                break;

            case REPEAT_TYPE:
                cCode.appendCode( "\n%sdo {\n", indentStr(indLevel));
                picode = &latch->back();
                picode->invalidate();
                break;

            case ENDLESS_TYPE:
                cCode.appendCode( "\n%sfor (;;) {\n", indentStr(indLevel));
        }
        stats.numHLIcode += 1;
        indLevel++;
    }

    /* Write the code for this basic block */
    if (repCond == FALSE)
        writeBB (indLevel, pProc, numLoc);

    /* Check for end of path */
    _nodeType = nodeType;
    if (_nodeType == RETURN_NODE || _nodeType == TERMINATE_NODE ||
        _nodeType == NOWHERE_NODE || (dfsLastNum == _latchNode))
        return;

    /* Check type of loop/node and process code */
    if (_loopType)	/* there is a loop */
    {
        assert(latch);
        if (this != latch)		/* loop is over several bbs */
        {
            if (_loopType == WHILE_TYPE)
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
        if (_loopType == WHILE_TYPE)
        {
            /* Check if there is need to repeat other statements involved
                         * in while condition, then, emit the loop trailer */
            if (repCond)
                writeBB (indLevel+1, pProc, numLoc);
            cCode.appendCode( "%s}	/* end of while */\n",indentStr(indLevel));
        }
        else if (_loopType == ENDLESS_TYPE)
            cCode.appendCode( "%s}	/* end of loop */\n",indentStr(indLevel));
        else if (_loopType == REPEAT_TYPE)
        {
            if (picode->hl()->opcode != HLI_JCOND)
                reportError (REPEAT_FAIL);
            {
                string e=walkCondExpr (picode->hl()->expr(), pProc, numLoc);
                cCode.appendCode( "%s} while (%s);\n", indentStr(indLevel),e.c_str());
            }
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
        if (_nodeType == TWO_BRANCH)		/* if-then[-else] */
        {
            stats.numHLIcode++;
            indLevel++;
            emptyThen = FALSE;

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
                        cCode.appendCode( "\n%s%s", indentStr(indLevel-1), l);
                        succ->writeCode (indLevel, pProc, numLoc, _latchNode,follow);
                    }
                    else		/* empty THEN part => negate ELSE part */
                    {
                        l = writeJcondInv ( *back().hl(), pProc, numLoc);
                        cCode.appendCode( "\n%s%s", indentStr(indLevel-1), l);
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
                else if (! emptyThen) 	/* already visited => emit label */
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
                cCode.appendCode( "%s%s", indentStr(indLevel-1), l);
                edges[THEN].BBptr->writeCode (indLevel, pProc, numLoc, _latchNode, _ifFollow);
                cCode.appendCode( "%s}\n%selse {\n", indentStr(indLevel-1), indentStr(indLevel - 1));
                edges[ELSE].BBptr->writeCode (indLevel, pProc, numLoc, _latchNode, _ifFollow);
                cCode.appendCode( "%s}\n", indentStr(--indLevel));
            }
        }

        else 	/* fall, call, 1w */
        {
            succ = edges[0].BBptr;		/* fall-through edge */
            if (succ->traversed != DFS_ALPHA)
                succ->writeCode (indLevel, pProc,numLoc, _latchNode,_ifFollow);
        }
    }
}
/* Writes the code for the current basic block.
 * Args: pBB: pointer to the current basic block.
 *		 Icode: pointer to the array of icodes for current procedure.
 *		 lev: indentation level - used for formatting.	*/
void BB::writeBB(int lev, Function * pProc, int *numLoc)
{
    /* Save the index into the code table in case there is a later goto
  * into this instruction (first instruction of the BB) */
    front().ll()->codeIdx = nextBundleIdx (&cCode.code);
    //hli[start].codeIdx = nextBundleIdx (&cCode.code);
    //for (i = start, last = i + length; i < last; i++)

    /* Generate code for each hlicode that is not a HLI_JCOND */

    for(ICODE &pHli : *this)
    {
        if ((pHli.type == HIGH_LEVEL) && ( pHli.valid() )) //TODO: use filtering range here.
        {
            std::string line = pHli.hl()->write1HlIcode(pProc, numLoc);
            if (!line.empty())
            {
                cCode.appendCode( "%s%s", indentStr(lev), line.c_str());
                stats.numHLIcode++;
            }
            if (option.verbose)
                pHli.writeDU();
        }
    }
}

iICODE BB::begin()
{
    return range_start;
}

iICODE BB::end() const
{
    return range_end;
}
ICODE &BB::back()
{
    return *rbegin();
}

size_t BB::size()
{
    return distance(range_start,range_end);
}

ICODE &BB::front()
{
    return *begin();
}

riICODE BB::rbegin()
{
    return riICODE(end());
}
riICODE BB::rend()
{
    return riICODE(begin());
}
