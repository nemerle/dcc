/*****************************************************************************
 *      dcc project Universal Decompilation Module
 *   This is supposedly a machine independant and language independant module
 *   that just plays with abstract cfg's and intervals and such like.
 * (C) Cristina Cifuentes
 ****************************************************************************/
#include <list>
#include <cassert>
#include <stdio.h>
#include "dcc.h"

static void displayCFG(Function * pProc);
static void displayDfs(BB * pBB);

/****************************************************************************
 * udm
 ****************************************************************************/
void udm(void)
{

    /* Build the control flow graph, find idioms, and convert low-level
     * icodes to high-level ones */
    for (auto iter = pProcList.rbegin(); iter!=pProcList.rend(); ++iter)
    {

        if (iter->flg & PROC_ISLIB)
            continue;         /* Ignore library functions */

        /* Create the basic control flow graph */
        iter->createCFG();
        if (option.VeryVerbose)
            iter->displayCFG();

        /* Remove redundancies and add in-edge information */
        iter->compressCFG();

        /* Print 2nd pass assembler listing */
        if (option.asm2)
            disassem(2, &(*iter));

        /* Idiom analysis and propagation of long type */
        iter->lowLevelAnalysis();

        /* Generate HIGH_LEVEL icodes whenever possible */
        iter->highLevelGen();
    }

    /* Data flow analysis - eliminate condition codes, extraneous registers
         * and intermediate instructions.  Find expressions by forward
         * substitution algorithm */
    pProcList.front().dataFlow (0);
    derSeq *derivedG=0;

    /* Control flow analysis - structuring algorithm */
    for (auto iter = pProcList.rbegin(); iter!=pProcList.rend(); ++iter)
    {

        if (iter->flg & PROC_ISLIB)
            continue;         /* Ignore library functions */

        /* Make cfg reducible and build derived sequences */
        derivedG=iter->checkReducibility();

        if (option.VeryVerbose)
            derivedG->display();

        /* Structure the graph */
        iter->structure(derivedG);

        /* Check for compound conditions */
        iter->compoundCond ();

        if (option.verbose) {
            printf("\nDepth first traversal - Proc %s\n", iter->name);
            iter->cfg.front()->displayDfs();
        }

        /* Free storage occupied by this procedure */
        freeDerivedSeq(*derivedG);
    }
}


static const char *const s_nodeType[] = {"branch", "if", "case", "fall", "return", "call",
                                 "loop", "repeat", "interval", "cycleHead",
                                 "caseHead", "terminate",
                                 "nowhere" };

static const char *const s_loopType[] = {"noLoop", "while", "repeat", "loop", "for"};


/****************************************************************************
 * displayCFG - Displays the Basic Block list
 ***************************************************************************/
void Function::displayCFG()
{
    Int i;
    BB * pBB;

    printf("\nBasic Block List - Proc %s", name);

    for (auto iter = cfg.begin(); iter!=cfg.end(); ++iter)
    {
        pBB = *iter;
        printf("\nnode type = %s, ", s_nodeType[pBB->nodeType]);
        printf("start = %ld, length = %ld, #out edges = %ld\n",
               pBB->start, pBB->length, pBB->numOutEdges);

        for (i = 0; i < pBB->numOutEdges; i++)
            printf(" outEdge[%2d] = %ld\n",i, pBB->edges[i].BBptr->start);
    }
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
        printf ("  inEdge[%ld] = %ld\n", i, inEdges[i]->start);

    /* Display out edges information */
    for (i = 0; i < numOutEdges; i++)
        if (nodeType == INTERVAL_NODE)
            printf(" outEdge[%ld] = %ld\n", i,
                   edges[i].BBptr->correspInt->numInt);
        else
            printf(" outEdge[%d] = %ld\n", i, edges[i].BBptr->start);
    printf("----\n");

    /* Recursive call on successors of current node */
    for (i = 0; i < numOutEdges; i++)
        if (edges[i].BBptr->traversed != DFS_DISP)
            edges[i].BBptr->displayDfs();
}
