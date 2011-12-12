#include <cassert>
#include <string>

#include "BasicBlock.h"
#include "Procedure.h"
#include "dcc.h"
using namespace std;
extern char *indent (Int indLevel);
BB *BB::Create(void *ctx, const string &s, Function *parent, BB *insertBefore)
{
    BB *pnewBB = new BB;
    pnewBB->Parent = parent;
    return pnewBB;
}

BB *BB::Create(Int start, Int ip, byte nodeType, Int numOutEdges, Function *parent)
{
    parent->cfg;
    BB* pnewBB;

    pnewBB = new BB;
    pnewBB->nodeType = nodeType;	/* Initialise */
    pnewBB->start = start;
    pnewBB->length = ip - start + 1;
    pnewBB->numOutEdges = (byte)numOutEdges;
    pnewBB->immedDom = NO_DOM;
    pnewBB->loopHead = pnewBB->caseHead = pnewBB->caseTail =
            pnewBB->latchNode= pnewBB->loopFollow = NO_NODE;

    if (numOutEdges)
        pnewBB->edges.resize(numOutEdges);

    /* Mark the basic block to which the icodes belong to, but only for
         * real code basic blocks (ie. not interval bbs) */
    if(parent)
    {
        if (start >= 0)
            parent->Icode.SetInBB(start, ip, pnewBB);
        parent->heldBBs.push_back(pnewBB);
        parent->cfg.push_back(pnewBB);
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
    printf("start = %ld, length = %ld, #out edges = %ld\n",
           start, length, numOutEdges);

    for (int i = 0; i < numOutEdges; i++)
        printf(" outEdge[%2d] = %ld\n",i, edges[i].BBptr->start);
}
/*****************************************************************************
 * displayDfs - Displays the CFG using a depth first traversal
 ****************************************************************************/
void BB::displayDfs()
{
    Int i;
    assert(this);
    traversed = DFS_DISP;

    printf("node type = %s, ", s_nodeType[nodeType]);
    printf("start = %ld, length = %ld, #in-edges = %ld, #out-edges = %ld\n",
           start, length, inEdges.size(), numOutEdges);
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
        for (i = 0; i < inEdges.size(); i++)
        printf ("  inEdge[%ld] = %ld\n", i, inEdges[i]->begin());

    /* Display out edges information */
    for (i = 0; i < numOutEdges; i++)
        if (nodeType == INTERVAL_NODE)
            printf(" outEdge[%ld] = %ld\n", i,
                   edges[i].BBptr->correspInt->numInt);
        else
            printf(" outEdge[%d] = %ld\n", i, edges[i].BBptr->begin());
    printf("----\n");

    /* Recursive call on successors of current node */
    for (i = 0; i < numOutEdges; i++)
        if (edges[i].BBptr->traversed != DFS_DISP)
            edges[i].BBptr->displayDfs();
}
/* Recursive procedure that writes the code for the given procedure, pointed
 * to by pBB.
 * Parameters:	pBB:	pointer to the cfg.
 *				Icode:	pointer to the Icode array for the cfg graph of the
 *						current procedure.
 *				indLevel: indentation level - used for formatting.
 *				numLoc: last # assigned to local variables 				*/
void BB::writeCode (Int indLevel, Function * pProc , Int *numLoc,Int latchNode, Int _ifFollow)
{
    Int follow,						/* ifFollow						*/
            _loopType, 					/* Type of loop, if any 		*/
            _nodeType;						/* Type of node 				*/
    BB * succ, *latch;					/* Successor and latching node 	*/
    ICODE * picode;					/* Pointer to HLI_JCOND instruction	*/
    char *l;							/* Pointer to HLI_JCOND expression	*/
    boolT emptyThen,					/* THEN clause is empty			*/
            repCond;					/* Repeat condition for while() */

    /* Check if this basic block should be analysed */
    if ((_ifFollow != UN_INIT) && (this == pProc->dfsLast[_ifFollow]))
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
        latch = pProc->dfsLast[this->latchNode];
        switch (_loopType)
        {
            case WHILE_TYPE:
                picode = &this->back();

                /* Check for error in while condition */
                if (picode->ic.hl.opcode != HLI_JCOND)
                    reportError (WHILE_FAIL);

                /* Check if condition is more than 1 HL instruction */
                if (numHlIcodes > 1)
                {
                    /* Write the code for this basic block */
                    writeBB(&pProc->Icode.front(), indLevel, pProc, numLoc);
                    repCond = TRUE;
                }

                /* Condition needs to be inverted if the loop body is along
                 * the THEN path of the header node */
                if (edges[ELSE].BBptr->dfsLastNum == loopFollow)
                    inverseCondOp (&picode->ic.hl.oper.exp);
                {
                    string e=walkCondExpr (picode->ic.hl.oper.exp, pProc, numLoc);
                    cCode.appendCode( "\n%swhile (%s) {\n", indent(indLevel),e.c_str());
                }
                picode->invalidate();
                break;

            case REPEAT_TYPE:
                cCode.appendCode( "\n%sdo {\n", indent(indLevel));
                picode = &latch->back();
                picode->invalidate();
                break;

            case ENDLESS_TYPE:
                cCode.appendCode( "\n%sfor (;;) {\n", indent(indLevel));
        }
        stats.numHLIcode += 1;
        indLevel++;
    }

    /* Write the code for this basic block */
    if (repCond == FALSE)
        writeBB (&pProc->Icode.front(), indLevel, pProc, numLoc);

    /* Check for end of path */
    _nodeType = nodeType;
    if (_nodeType == RETURN_NODE || _nodeType == TERMINATE_NODE ||
        _nodeType == NOWHERE_NODE || (dfsLastNum == latchNode))
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
                succ->front().emitGotoLabel (indLevel);
        }

        /* Loop epilogue: generate the loop trailer */
        indLevel--;
        if (_loopType == WHILE_TYPE)
        {
            /* Check if there is need to repeat other statements involved
                         * in while condition, then, emit the loop trailer */
            if (repCond)
                writeBB (&pProc->Icode.front(), indLevel+1, pProc, numLoc);
            cCode.appendCode( "%s}	/* end of while */\n",indent(indLevel));
        }
        else if (_loopType == ENDLESS_TYPE)
            cCode.appendCode( "%s}	/* end of loop */\n",indent(indLevel));
        else if (_loopType == REPEAT_TYPE)
        {
            if (picode->ic.hl.opcode != HLI_JCOND)
                reportError (REPEAT_FAIL);
            {
                string e=walkCondExpr (picode->ic.hl.oper.exp, pProc, numLoc);
                cCode.appendCode( "%s} while (%s);\n", indent(indLevel),e.c_str());
            }
        }

        /* Recurse on the loop follow */
        if (loopFollow != MAX)
        {
            succ = pProc->dfsLast[loopFollow];
            if (succ->traversed != DFS_ALPHA)
                succ->writeCode (indLevel, pProc, numLoc, latchNode, _ifFollow);
            else		/* has been traversed so we need a goto */
                succ->front().emitGotoLabel (indLevel);
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
                        l = writeJcond ( back().ic.hl, pProc, numLoc);
                        cCode.appendCode( "\n%s%s", indent(indLevel-1), l);
                        succ->writeCode (indLevel, pProc, numLoc, latchNode,follow);
                    }
                    else		/* empty THEN part => negate ELSE part */
                    {
                        l = writeJcondInv ( back().ic.hl, pProc, numLoc);
                        cCode.appendCode( "\n%s%s", indent(indLevel-1), l);
                        edges[ELSE].BBptr->writeCode (indLevel, pProc, numLoc, latchNode, follow);
                        emptyThen = TRUE;
                    }
                }
                else	/* already visited => emit label */
                    succ->front().emitGotoLabel(indLevel);

                /* process the ELSE part */
                succ = edges[ELSE].BBptr;
                if (succ->traversed != DFS_ALPHA)		/* not visited */
                {
                    if (succ->dfsLastNum != follow)		/* ELSE part */
                    {
                        cCode.appendCode( "%s}\n%selse {\n",
                                          indent(indLevel-1), indent(indLevel - 1));
                        succ->writeCode (indLevel, pProc, numLoc, latchNode, follow);
                    }
                    /* else (empty ELSE part) */
                }
                else if (! emptyThen) 	/* already visited => emit label */
                {
                    cCode.appendCode( "%s}\n%selse {\n",
                                      indent(indLevel-1), indent(indLevel - 1));
                    succ->front().emitGotoLabel (indLevel);
                }
                cCode.appendCode( "%s}\n", indent(--indLevel));

                /* Continue with the follow */
                succ = pProc->dfsLast[follow];
                if (succ->traversed != DFS_ALPHA)
                    succ->writeCode (indLevel, pProc, numLoc, latchNode,_ifFollow);
            }
            else		/* no follow => if..then..else */
            {
                l = writeJcond (
                        back().ic.hl, pProc, numLoc);
                cCode.appendCode( "%s%s", indent(indLevel-1), l);
                edges[THEN].BBptr->writeCode (indLevel, pProc, numLoc, latchNode, _ifFollow);
                cCode.appendCode( "%s}\n%selse {\n", indent(indLevel-1), indent(indLevel - 1));
                edges[ELSE].BBptr->writeCode (indLevel, pProc, numLoc, latchNode, _ifFollow);
                cCode.appendCode( "%s}\n", indent(--indLevel));
            }
        }

        else 	/* fall, call, 1w */
        {
            succ = edges[0].BBptr;		/* fall-through edge */
            if (succ->traversed != DFS_ALPHA)
                succ->writeCode (indLevel, pProc,numLoc, latchNode,_ifFollow);
        }
    }
}
/* Writes the code for the current basic block.
 * Args: pBB: pointer to the current basic block.
 *		 Icode: pointer to the array of icodes for current procedure.
 *		 lev: indentation level - used for formatting.	*/
void BB::writeBB(ICODE * hli, Int lev, Function * pProc, Int *numLoc)
{
    Int	i, last;
    char *line;           /* Pointer to the HIGH-LEVEL line   */

    /* Save the index into the code table in case there is a later goto
  * into this instruction (first instruction of the BB) */
    hli[start].codeIdx = nextBundleIdx (&cCode.code);

    /* Generate code for each hlicode that is not a HLI_JCOND */
    for (i = start, last = i + length; i < last; i++)
        if ((hli[i].type == HIGH_LEVEL) && (hli[i].invalid == FALSE))
        {
            line = write1HlIcode (hli[i].ic.hl, pProc, numLoc);
            if (line[0] != '\0')
            {
                cCode.appendCode( "%s%s", indent(lev), line);
                stats.numHLIcode++;
            }
            if (option.verbose)
                hli[i].writeDU(i);
        }
}
int BB::begin()
{
    return start;
}
int BB::rbegin()
{
    return start+length-1;
}
int BB::end()
{
    return start+length;
}
ICODE &BB::back()
{
    return Parent->Icode[rbegin()];
}

size_t BB::size()
{
    return length;
}

ICODE &BB::front()
{
    return Parent->Icode[start];
}
