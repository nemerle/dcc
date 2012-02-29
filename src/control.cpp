/*********************************************************************
 * Description   : Performs control flow analysis on the CFG
 * (C) Cristina Cifuentes
 ********************************************************************/
#include <algorithm>
#include <list>
#include <cassert>
#include "dcc.h"
#include <stdio.h>
#include <string.h>
#if __BORLAND__
#include <alloc.h>
#else
#include <malloc.h>
#endif

//typedef struct list {
//    int         nodeIdx;
//    struct list *next;
//} nodeList;
typedef std::list<int> nodeList; /* dfsLast index to the node */

#define ancestor(a,b)	((a->dfsLastNum < b->dfsLastNum) && (a->dfsFirstNum < b->dfsFirstNum))
/* there is a path on the DFST from a to b if the a was first visited in a
 * dfs, and a was later visited than b when doing the last visit of each
 * node. */


/* Checks if the edge (p,s) is a back edge.  If node s was visited first
 * during the dfs traversal (ie. s has a smaller dfsFirst number) or s == p,
 * then it is a backedge.
 * Also incrementes the number of backedges entries to the header node. */
static boolT isBackEdge (BB * p,BB * s)
{
    if (p->dfsFirstNum >= s->dfsFirstNum)
    {
        s->numBackEdges++;
        return true;
    }
    return false;
}


/* Finds the common dominator of the current immediate dominator
 * currImmDom and its predecessor's immediate dominator predImmDom  */
static int commonDom (int currImmDom, int predImmDom, Function * pProc)
{
    if (currImmDom == NO_DOM)
        return (predImmDom);
    if (predImmDom == NO_DOM)   /* predecessor is the root */
        return (currImmDom);

    while ((currImmDom != NO_DOM) && (predImmDom != NO_DOM) &&
           (currImmDom != predImmDom))
    {
        if (currImmDom < predImmDom)
            predImmDom = pProc->m_dfsLast[predImmDom]->immedDom;
        else
            currImmDom = pProc->m_dfsLast[currImmDom]->immedDom;
    }
    return (currImmDom);
}


/* Finds the immediate dominator of each node in the graph pProc->cfg.
 * Adapted version of the dominators algorithm by Hecht and Ullman; finds
 * immediate dominators only.
 * Note: graph should be reducible */
void Function::findImmedDom ()
{
    BB * currNode;
    int currIdx, j, predIdx;

    for (currIdx = 0; currIdx < numBBs; currIdx++)
    {
        currNode = m_dfsLast[currIdx];
        if (currNode->flg & INVALID_BB)		/* Do not process invalid BBs */
            continue;
        for (BB * inedge : currNode->inEdges)
        {
            predIdx = inedge->dfsLastNum;
            if (predIdx < currIdx)
                currNode->immedDom = commonDom (currNode->immedDom, predIdx, this);
        }
    }
}


/* Inserts the node n to the list l. */
static void insertList (nodeList &l, int n)
{
    l.push_back(n);
}


/* Returns whether or not the node n (dfsLast numbering of a basic block)
 * is on the list l. */
static boolT inList (nodeList &l, int n)
{
    return std::find(l.begin(),l.end(),n)!=l.end();
}


/* Frees space allocated by the list l.  */
static void freeList (nodeList &l)
{
    l.clear();
}


/* Returns whether the node n belongs to the queue list q. */
static boolT inInt(BB * n, queue &q)
{
    return std::find(q.begin(),q.end(),n)!=q.end();
}


/* Finds the follow of the endless loop headed at node head (if any).
 * The follow node is the closest node to the loop. */
static void findEndlessFollow (Function * pProc, nodeList &loopNodes, BB * head)
{
    int j, succ;

    head->loopFollow = MAX;
    nodeList::iterator p = loopNodes.begin();
    for( ;p != loopNodes.end();++p)
    {
        for (j = 0; j < pProc->m_dfsLast[*p]->edges.size(); j++)
        {
            succ = pProc->m_dfsLast[*p]->edges[j].BBptr->dfsLastNum;
            if ((! inList(loopNodes, succ)) && (succ < head->loopFollow))
                head->loopFollow = succ;
        }
    }
}


//static void findNodesInLoop(BB * latchNode,BB * head,PPROC pProc,queue *intNodes)
/* Flags nodes that belong to the loop determined by (latchNode, head) and
 * determines the type of loop.                     */
static void findNodesInLoop(BB * latchNode,BB * head,Function * pProc,queue &intNodes)
{
    int i, headDfsNum, intNodeType;
    nodeList loopNodes;
    int immedDom,     		/* dfsLast index to immediate dominator */
        thenDfs, elseDfs;       /* dsfLast index for THEN and ELSE nodes */
    BB * pbb;

    /* Flag nodes in loop headed by head (except header node) */
    headDfsNum = head->dfsLastNum;
    head->loopHead = headDfsNum;
    insertList (loopNodes, headDfsNum);
    for (i = headDfsNum + 1; i < latchNode->dfsLastNum; i++)
    {
        if (pProc->m_dfsLast[i]->flg & INVALID_BB)	/* skip invalid BBs */
            continue;

        immedDom = pProc->m_dfsLast[i]->immedDom;
        if (inList (loopNodes, immedDom) && inInt(pProc->m_dfsLast[i], intNodes))
        {
            insertList (loopNodes, i);
            if (pProc->m_dfsLast[i]->loopHead == NO_NODE)/*not in other loop*/
                pProc->m_dfsLast[i]->loopHead = headDfsNum;
        }
    }
    latchNode->loopHead = headDfsNum;
    if (latchNode != head)
        insertList (loopNodes, latchNode->dfsLastNum);

    /* Determine type of loop and follow node */
    intNodeType = head->nodeType;
    if (latchNode->nodeType == TWO_BRANCH)
        if ((intNodeType == TWO_BRANCH) || (latchNode == head))
            if ((latchNode == head) ||
                (inList (loopNodes, head->edges[THEN].BBptr->dfsLastNum) &&
                 inList (loopNodes, head->edges[ELSE].BBptr->dfsLastNum)))
            {
                head->loopType = REPEAT_TYPE;
                if (latchNode->edges[0].BBptr == head)
                    head->loopFollow = latchNode->edges[ELSE].BBptr->dfsLastNum;
                else
                    head->loopFollow = latchNode->edges[THEN].BBptr->dfsLastNum;
                latchNode->back().ll()->SetLlFlag(JX_LOOP);
            }
            else
            {
                head->loopType = WHILE_TYPE;
                if (inList (loopNodes, head->edges[THEN].BBptr->dfsLastNum))
                    head->loopFollow = head->edges[ELSE].BBptr->dfsLastNum;
                else
                    head->loopFollow = head->edges[THEN].BBptr->dfsLastNum;
                head->back().ll()->SetLlFlag(JX_LOOP);
            }
        else /* head = anything besides 2-way, latch = 2-way */
        {
            head->loopType = REPEAT_TYPE;
            if (latchNode->edges[THEN].BBptr == head)
                head->loopFollow = latchNode->edges[ELSE].BBptr->dfsLastNum;
            else
                head->loopFollow = latchNode->edges[THEN].BBptr->dfsLastNum;
            latchNode->back().ll()->SetLlFlag(JX_LOOP);
        }
    else	/* latch = 1-way */
        if (latchNode->nodeType == LOOP_NODE)
        {
            head->loopType = REPEAT_TYPE;
            head->loopFollow = latchNode->edges[0].BBptr->dfsLastNum;
        }
        else if (intNodeType == TWO_BRANCH)
        {
            head->loopType = WHILE_TYPE;
            pbb = latchNode;
            thenDfs = head->edges[THEN].BBptr->dfsLastNum;
            elseDfs = head->edges[ELSE].BBptr->dfsLastNum;
            while (1)
            {
                if (pbb->dfsLastNum == thenDfs)
                {
                    head->loopFollow = elseDfs;
                    break;
                }
                else if (pbb->dfsLastNum == elseDfs)
                {
                    head->loopFollow = thenDfs;
                    break;
                }

                /* Check if couldn't find it, then it is a strangely formed
                                 * loop, so it is safer to consider it an endless loop */
                if (pbb->dfsLastNum <= head->dfsLastNum)
                {
                    head->loopType = ENDLESS_TYPE;
                    findEndlessFollow (pProc, loopNodes, head);
                    break;
                }
                pbb = pProc->m_dfsLast[pbb->immedDom];
            }
            if (pbb->dfsLastNum > head->dfsLastNum)
                pProc->m_dfsLast[head->loopFollow]->loopHead = NO_NODE;	/*****/
            head->back().ll()->SetLlFlag(JX_LOOP);
        }
        else
        {
            head->loopType = ENDLESS_TYPE;
            findEndlessFollow (pProc, loopNodes, head);
        }

    freeList(loopNodes);
}

//static void findNodesInInt (queue **intNodes, int level, interval *Ii)
/* Recursive procedure to find nodes that belong to the interval (ie. nodes
 * from G1).                                */
static void findNodesInInt (queue &intNodes, int level, interval *Ii)
{
    if (level == 1)
    {
        for(BB *en : Ii->nodes)
            appendQueue(intNodes,en);
    }
    else
        for(BB *en : Ii->nodes)
            findNodesInInt(intNodes,level-1,en->correspInt);
}


/* Algorithm for structuring loops */
void Function::structLoops(derSeq *derivedG)
{
    interval *Ii;
    BB * intHead,      	/* interval header node         	*/
            * pred,     /* predecessor node         		*/
            * latchNode;/* latching node (in case of loops) */
    int i,              /* counter              			*/
            level = 0;  /* derived sequence level       	*/
    interval *initInt;  /* initial interval         		*/
    queue intNodes;  	/* list of interval nodes       	*/

    /* Structure loops */
    /* for all derived sequences Gi */
    for(derSeq::iterator iter=derivedG->begin(); iter!=derivedG->end(); ++iter)
    {
        level++;
        Ii = iter->Ii;
        while (Ii)       /* for all intervals Ii of Gi */
        {
            latchNode = NULL;
          intNodes.clear();

            /* Find interval head (original BB node in G1) and create
           * list of nodes of interval Ii.              */
            initInt = Ii;
            for (i = 1; i < level; i++)
                initInt = (*initInt->nodes.begin())->correspInt;
            intHead = *initInt->nodes.begin();

            /* Find nodes that belong to the interval (nodes from G1) */
            findNodesInInt (intNodes, level, Ii);

            /* Find greatest enclosing back edge (if any) */
            for (i = 0; i < intHead->inEdges.size(); i++)
            {
                pred = intHead->inEdges[i];
                if (inInt(pred, intNodes) && isBackEdge(pred, intHead))
                    if (! latchNode)
                        latchNode = pred;
                    else
                    {
                        if (pred->dfsLastNum > latchNode->dfsLastNum)
                            latchNode = pred;
                    }
            }

            /* Find nodes in the loop and the type of loop */
            if (latchNode)
            {
                /* Check latching node is at the same nesting level of case
                 * statements (if any) and that the node doesn't belong to
                 * another loop.                   */
                if ((latchNode->caseHead == intHead->caseHead) &&
                    (latchNode->loopHead == NO_NODE))
                {
                    intHead->latchNode = latchNode->dfsLastNum;
                    findNodesInLoop(latchNode, intHead, this, intNodes);
                    latchNode->flg |= IS_LATCH_NODE;
                }
            }

            /* Next interval */
            Ii = Ii->next;
        }

        /* Next derived sequence */
    }
}


/* Returns whether the BB indexed by s is a successor of the BB indexed by
 * h.  Note that h is a case node.                  */
static boolT successor (int s, int h, Function * pProc)
{
    int i;
    BB * header;

    header = pProc->m_dfsLast[h];
    for (i = 0; i < header->edges.size(); i++)
        if (header->edges[i].BBptr->dfsLastNum == s)
            return true;
    return false;
}


/* Recursive procedure to tag nodes that belong to the case described by
 * the list l, head and tail (dfsLast index to first and exit node of the
 * case).                               */
static void tagNodesInCase (BB * pBB, nodeList &l, int head, int tail)
{ int current,      /* index to current node */
            i;

    pBB->traversed = DFS_CASE;
    current = pBB->dfsLastNum;
    if ((current != tail) && (pBB->nodeType != MULTI_BRANCH) && (inList (l, pBB->immedDom)))
    {
        insertList (l, current);
        pBB->caseHead = head;
        for (i = 0; i < pBB->edges.size(); i++)
            if (pBB->edges[i].BBptr->traversed != DFS_CASE)
                tagNodesInCase (pBB->edges[i].BBptr, l, head, tail);
    }
}


/* Structures case statements.  This procedure is invoked only when pProc
 * has a case node.                         */
void Function::structCases()
{
    int i, j;
    BB * caseHeader;       		/* case header node         */
    int exitNode = NO_NODE;   	/* case exit node           */
    nodeList caseNodes;   /* temporary: list of nodes in case */

    /* Linear scan of the nodes in reverse dfsLast order, searching for
     * case nodes                           */
    for (i = numBBs - 1; i >= 0; i--)
        if (m_dfsLast[i]->nodeType == MULTI_BRANCH)
        {
            caseHeader = m_dfsLast[i];

            /* Find descendant node which has as immediate predecessor
                         * the current header node, and is not a successor.    */
            for (j = i + 2; j < numBBs; j++)
            {
                if ((!successor(j, i, this)) &&
                    (m_dfsLast[j]->immedDom == i))
                    if (exitNode == NO_NODE)
                        exitNode = j;
                    else if (m_dfsLast[exitNode]->inEdges.size() < m_dfsLast[j]->inEdges.size())
                        exitNode = j;
            }
            m_dfsLast[i]->caseTail = exitNode;

            /* Tag nodes that belong to the case by recording the
                         * header field with caseHeader.           */
            insertList (caseNodes, i);
            m_dfsLast[i]->caseHead = i;
            for(TYPEADR_TYPE &pb : caseHeader->edges)
            {
                tagNodesInCase(pb.BBptr, caseNodes, i, exitNode);
            }
            //for (j = 0; j < caseHeader->edges[j]; j++)
            //    tagNodesInCase (caseHeader->edges[j].BBptr, caseNodes, i, exitNode);
            if (exitNode != NO_NODE)
                m_dfsLast[exitNode]->caseHead = i;
        }
}


/* Flags all nodes in the list l as having follow node f, and deletes all
 * nodes from the list.                         */
static void flagNodes (nodeList &l, int f, Function * pProc)
{
    nodeList::iterator p;

    p = l.begin();
    while (p!=l.end())
    {
        pProc->m_dfsLast[*p]->ifFollow = f;
        p = l.erase(p);
    }
}


/* Structures if statements */
void Function::structIfs ()
{
    int curr,    				/* Index for linear scan of nodes   	*/
            desc,    				/* Index for descendant         		*/
            followInEdges,			/* Largest # in-edges so far 			*/
            follow;  				/* Possible follow node 				*/
    nodeList domDesc,    /* List of nodes dominated by curr  	*/
            unresolved 	/* List of unresolved if nodes  		*/
            ;
    BB * currNode,    			/* Pointer to current node  			*/
            * pbb;

    /* Linear scan of nodes in reverse dfsLast order */
    for (curr = numBBs - 1; curr >= 0; curr--)
    {
        currNode = m_dfsLast[curr];
        if (currNode->flg & INVALID_BB)		/* Do not process invalid BBs */
            continue;

        if ((currNode->nodeType == TWO_BRANCH) && (!currNode->back().ll()->isLlFlag(JX_LOOP)))
        {
            followInEdges = 0;
            follow = 0;

            /* Find all nodes that have this node as immediate dominator */
            for (desc = curr+1; desc < numBBs; desc++)
            {
                if (m_dfsLast[desc]->immedDom == curr)
                {
                    insertList (domDesc, desc);
                    pbb = m_dfsLast[desc];
                    if ((pbb->inEdges.size() - pbb->numBackEdges) >= followInEdges)
                    {
                        follow = desc;
                        followInEdges = pbb->inEdges.size() - pbb->numBackEdges;
                    }
                }
            }

            /* Determine follow according to number of descendants
                         * immediately dominated by this node  */
            if ((follow != 0) && (followInEdges > 1))
            {
                currNode->ifFollow = follow;
                if (!unresolved.empty())
                    flagNodes (unresolved, follow, this);
            }
            else
                insertList (unresolved, curr);
        }
        freeList (domDesc);
    }
}


/* Checks for compound conditions of basic blocks that have only 1 high
 * level instruction.  Whenever these blocks are found, they are merged
 * into one block with the appropriate condition */
void Function::compoundCond()
{
    int i, j, k, numOutEdges;
    BB * pbb, * t, * e, * obb,* pred;
    ICODE * picode, * ticode;
    COND_EXPR *exp;
    TYPEADR_TYPE *edges;
    boolT change;

    change = TRUE;
    while (change)
    {
        change = FALSE;

        /* Traverse nodes in postorder, this way, the header node of a
         * compound condition is analysed first */
        for (i = 0; i < this->numBBs; i++)
        {
            pbb = this->m_dfsLast[i];
            if (pbb->flg & INVALID_BB)
                continue;

            if (pbb->nodeType != TWO_BRANCH)
                continue;

            t = pbb->edges[THEN].BBptr;
            e = pbb->edges[ELSE].BBptr;

            /* Check (X || Y) case */
            if ((t->nodeType == TWO_BRANCH) && (t->numHlIcodes == 1) &&
                    (t->inEdges.size() == 1) && (t->edges[ELSE].BBptr == e))
            {
                obb = t->edges[THEN].BBptr;

                /* Construct compound DBL_OR expression */
                picode = &pbb->back();
                ticode = &t->back();
                picode->hl()->expr(COND_EXPR::boolOp (picode->hl()->expr(), ticode->hl()->expr(), DBL_OR));

                /* Replace in-edge to obb from t to pbb */
                {
                    auto iter=find(obb->inEdges.begin(),obb->inEdges.end(),t);
                    if(iter!=obb->inEdges.end())
                        *iter = pbb;
                }

                /* New THEN out-edge of pbb */
                pbb->edges[THEN].BBptr = obb;

                /* Remove in-edge t to e */
                auto iter=std::find(e->inEdges.begin(),e->inEdges.end(),t);
                assert(iter!=e->inEdges.end());
                e->inEdges.erase(iter);
                t->flg |= INVALID_BB;

                if (pbb->flg & IS_LATCH_NODE)
                    this->m_dfsLast[t->dfsLastNum] = pbb;
                else
                    i--;		/* to repeat this analysis */

                change = true;
            }

            /* Check (!X && Y) case */
            else if ((t->nodeType == TWO_BRANCH) && (t->numHlIcodes == 1) &&
                     (t->inEdges.size() == 1) && (t->edges[THEN].BBptr == e))
            {
                obb = t->edges[ELSE].BBptr;

                /* Construct compound DBL_AND expression */
                picode = &pbb->back();
                ticode = &t->back();

                COND_EXPR *oldexpr=picode->hl()->expr();
                picode->hl()->expr(picode->hl()->expr()->inverse());
                delete oldexpr;

                picode->hl()->expr(COND_EXPR::boolOp (picode->hl()->expr(), ticode->hl()->expr(), DBL_AND));

                /* Replace in-edge to obb from t to pbb */
                auto iter=std::find(obb->inEdges.begin(),obb->inEdges.end(),t);
                assert(iter!=obb->inEdges.end());
                *iter=pbb;

                /* New THEN and ELSE out-edges of pbb */
                pbb->edges[THEN].BBptr = e;
                pbb->edges[ELSE].BBptr = obb;

                /* Remove in-edge t to e */
                iter=std::find(e->inEdges.begin(),e->inEdges.end(),t);
                assert(iter!=e->inEdges.end());
                e->inEdges.erase(iter); /* looses 1 arc */
                t->flg |= INVALID_BB;

                if (pbb->flg & IS_LATCH_NODE)
                    this->m_dfsLast[t->dfsLastNum] = pbb;
                else
                    i--;		/* to repeat this analysis */

                change = TRUE;
            }

            /* Check (X && Y) case */
            else if ((e->nodeType == TWO_BRANCH) && (e->numHlIcodes == 1) &&
                     (e->inEdges.size()==1) && (e->edges[THEN].BBptr == t))
            {
                obb = e->edges[ELSE].BBptr;

                /* Construct compound DBL_AND expression */
                picode = &pbb->back();
                ticode = &t->back();
                picode->hl()->expr(COND_EXPR::boolOp (picode->hl()->expr(),ticode->hl()->expr(), DBL_AND));

                /* Replace in-edge to obb from e to pbb */
                auto iter = std::find(obb->inEdges.begin(),obb->inEdges.end(),e);
                assert(iter!=obb->inEdges.end());
                *iter=pbb;
                /* New ELSE out-edge of pbb */
                pbb->edges[ELSE].BBptr = obb;

                /* Remove in-edge e to t */
                iter = std::find(t->inEdges.begin(),t->inEdges.end(),e);
                assert(iter!=t->inEdges.end());
                t->inEdges.erase(iter);
                e->flg |= INVALID_BB;

                if (pbb->flg & IS_LATCH_NODE)
                    this->m_dfsLast[e->dfsLastNum] = pbb;
                else
                    i--;		/* to repeat this analysis */

                change = TRUE;
            }

            /* Check (!X || Y) case */
            else if ((e->nodeType == TWO_BRANCH) && (e->numHlIcodes == 1) &&
                     (e->inEdges.size() == 1) && (e->edges[ELSE].BBptr == t))
            {
                obb = e->edges[THEN].BBptr;

                /* Construct compound DBL_OR expression */
                picode = &pbb->back();
                ticode = &t->back();
                COND_EXPR *oldexp=picode->hl()->expr();
                picode->hl()->expr(picode->hl()->expr()->inverse());
                delete oldexp;
                picode->hl()->expr(COND_EXPR::boolOp (picode->hl()->expr(), ticode->hl()->expr(), DBL_OR));
                //picode->hl()->expr() = exp;

                /* Replace in-edge to obb from e to pbb */
                auto iter = std::find(obb->inEdges.begin(),obb->inEdges.end(),e);
                assert(iter!=obb->inEdges.end());
                *iter=pbb;

                /* New THEN and ELSE out-edges of pbb */
                pbb->edges[THEN].BBptr = obb;
                pbb->edges[ELSE].BBptr = t;

                /* Remove in-edge e to t */
                iter = std::find(t->inEdges.begin(),t->inEdges.end(),e);
                assert(iter!=t->inEdges.end());
                t->inEdges.erase(iter);
                e->flg |= INVALID_BB;

                if (pbb->flg & IS_LATCH_NODE)
                    this->m_dfsLast[e->dfsLastNum] = pbb;
                else
                    i--;		/* to repeat this analysis */

                change = TRUE;
            }
        }
    }
}


/* Structuring algorithm to find the structures of the graph pProc->cfg */
void Function::structure(derSeq *derivedG)
{
    /* Find immediate dominators of the graph */
    findImmedDom();
    if (hasCase)
        structCases();
    structLoops(derivedG);
    structIfs();
}
