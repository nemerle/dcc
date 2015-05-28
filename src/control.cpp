/*********************************************************************
 * Description   : Performs control flow analysis on the CFG
 * (C) Cristina Cifuentes
 ********************************************************************/

#include "dcc.h"
#include <stdio.h>
#include <string.h>
#if __BORLAND__
#include <alloc.h>
#else
#include <malloc.h>
#endif

typedef struct list {
    Int         nodeIdx;    /* dfsLast index to the node */
    struct list *next;
} nodeList;


#define ancestor(a,b)	((a->dfsLastNum < b->dfsLastNum) && (a->dfsFirstNum < b->dfsFirstNum))
/* there is a path on the DFST from a to b if the a was first visited in a 
 * dfs, and a was later visited than b when doing the last visit of each 
 * node. */


static boolT isBackEdge (PBB p,PBB s)
/* Checks if the edge (p,s) is a back edge.  If node s was visited first 
 * during the dfs traversal (ie. s has a smaller dfsFirst number) or s == p,
 * then it is a backedge. 
 * Also incrementes the number of backedges entries to the header node. */
{
	if (p->dfsFirstNum >= s->dfsFirstNum)
	{
		s->numBackEdges++;
		return (TRUE);
	}
	return (FALSE);
}


static Int commonDom (Int currImmDom, Int predImmDom, PPROC pProc)
/* Finds the common dominator of the current immediate dominator
 * currImmDom and its predecessor's immediate dominator predImmDom  */ 
{
    if (currImmDom == NO_DOM)
       return (predImmDom);
    if (predImmDom == NO_DOM)   /* predecessor is the root */
       return (currImmDom);

    while ((currImmDom != NO_DOM) && (predImmDom != NO_DOM) &&
           (currImmDom != predImmDom))
    {
       if (currImmDom < predImmDom)
          predImmDom = pProc->dfsLast[predImmDom]->immedDom;
       else
          currImmDom = pProc->dfsLast[currImmDom]->immedDom;
    }
    return (currImmDom);
}


static void findImmedDom (PPROC pProc)
/* Finds the immediate dominator of each node in the graph pProc->cfg. 
 * Adapted version of the dominators algorithm by Hecht and Ullman; finds
 * immediate dominators only.  
 * Note: graph should be reducible */
{ PBB currNode;
  Int currIdx, j, predIdx;

    for (currIdx = 0; currIdx < pProc->numBBs; currIdx++) 
    {
		currNode = pProc->dfsLast[currIdx]; 
		if (currNode->flg & INVALID_BB)		/* Do not process invalid BBs */
			continue;

		for (j = 0; j < currNode->numInEdges; j++)
		{
			predIdx = currNode->inEdges[j]->dfsLastNum;
			if (predIdx < currIdx)
				currNode->immedDom = commonDom (currNode->immedDom, 
												predIdx, pProc);
		}
    }
}


static void insertList (nodeList **l, Int n)
/* Inserts the node n to the list l. */
{
	nodeList *newNode;

	newNode = allocStruc(nodeList);
	memset(newNode, 0, sizeof(nodeList));
    newNode->nodeIdx = n;
    newNode->next = *l;
    *l = newNode;
}


static boolT inList (nodeList *l, Int n)
/* Returns whether or not the node n (dfsLast numbering of a basic block) 
 * is on the list l. */
{
    while (l)
      if (l->nodeIdx == n)
         return (TRUE);
      else
         l = l->next;
    return (FALSE);
}


static void freeList (nodeList **l)
/* Frees space allocated by the list l.  */
{ nodeList *next;

    if (*l)
    {
       next = (*l)->next;
       free (*l);
       *l = next;
    }
    *l = NULL;
}


static boolT inInt(PBB n, queue *q) 
/* Returns whether the node n belongs to the queue list q. */ 
{ 
    while (q)
      if (q->node == n)
         return (TRUE);
      else
         q = q->next;
    return (FALSE);
}


static void findEndlessFollow (PPROC pProc, nodeList *loopNodes, PBB head)
/* Finds the follow of the endless loop headed at node head (if any).
 * The follow node is the closest node to the loop. */
{ nodeList *p;
  Int j, succ;

	head->loopFollow = MAX;
	p = loopNodes;
	while (p != NULL)
	{
		for (j = 0; j < pProc->dfsLast[p->nodeIdx]->numOutEdges; j++)
		{
			succ = pProc->dfsLast[p->nodeIdx]->edges[j].BBptr->dfsLastNum; 
			if ((! inList(loopNodes, succ)) && (succ < head->loopFollow))
				head->loopFollow = succ;
		}
		p = p->next;
	}
}


static void findNodesInLoop(PBB latchNode,PBB head,PPROC pProc,queue *intNodes) 
/* Flags nodes that belong to the loop determined by (latchNode, head) and 
 * determines the type of loop.                     */
{ Int i, headDfsNum, intNodeType, j;
  nodeList *loopNodes = NULL, p;
  Int immedDom,     		/* dfsLast index to immediate dominator */
	  thenDfs, elseDfs;		/* dsfLast index for THEN and ELSE nodes */
  PBB pbb;

    /* Flag nodes in loop headed by head (except header node) */
    headDfsNum = head->dfsLastNum;
    head->loopHead = headDfsNum;
    insertList (&loopNodes, headDfsNum);
    for (i = headDfsNum + 1; i < latchNode->dfsLastNum; i++)
    {
		if (pProc->dfsLast[i]->flg & INVALID_BB)	/* skip invalid BBs */
			continue;

        immedDom = pProc->dfsLast[i]->immedDom;
        if (inList (loopNodes, immedDom) && inInt(pProc->dfsLast[i], intNodes))
        {
           	insertList (&loopNodes, i);
           	if (pProc->dfsLast[i]->loopHead == NO_NODE)/*not in other loop*/
          	  	pProc->dfsLast[i]->loopHead = headDfsNum;
        }
    }
    latchNode->loopHead = headDfsNum;
    if (latchNode != head)
        insertList (&loopNodes, latchNode->dfsLastNum);

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
			pProc->Icode.SetLlFlag(latchNode->start + latchNode->length - 1,
				JX_LOOP);
          }
          else
          {
         	head->loopType = WHILE_TYPE;
            if (inList (loopNodes, head->edges[THEN].BBptr->dfsLastNum))
            	head->loopFollow = head->edges[ELSE].BBptr->dfsLastNum; 
            else
            	head->loopFollow = head->edges[THEN].BBptr->dfsLastNum; 
			pProc->Icode.SetLlFlag(head->start + head->length - 1, JX_LOOP);
          }
	   else /* head = anything besides 2-way, latch = 2-way */ 
       {
			head->loopType = REPEAT_TYPE;
			if (latchNode->edges[THEN].BBptr == head)
				head->loopFollow = latchNode->edges[ELSE].BBptr->dfsLastNum;
			else
				head->loopFollow = latchNode->edges[THEN].BBptr->dfsLastNum;
			pProc->Icode.SetLlFlag(latchNode->start + latchNode->length - 1,
				JX_LOOP);
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
				pbb = pProc->dfsLast[pbb->immedDom];	
			}
			if (pbb->dfsLastNum > head->dfsLastNum)
				pProc->dfsLast[head->loopFollow]->loopHead = NO_NODE;	/*****/
			pProc->Icode.SetLlFlag(head->start + head->length - 1, JX_LOOP);
		}
		else
		{
			head->loopType = ENDLESS_TYPE;
			findEndlessFollow (pProc, loopNodes, head);
		}

    freeList(&loopNodes);
}


static void findNodesInInt (queue **intNodes, Int level, interval *Ii)
/* Recursive procedure to find nodes that belong to the interval (ie. nodes 
 * from G1).                                */
{ queue *l;

    if (level == 1)
       for (l = Ii->nodes; l; l = l->next)
          appendQueue (intNodes, l->node);
    else
       for (l = Ii->nodes; l; l= l->next)
          findNodesInInt (intNodes, level - 1, l->node->correspInt);
}


static void structLoops(PPROC pProc, derSeq *derivedG)
/* Algorithm for structuring loops */
{ interval *Ii;
  PBB intHead,      	/* interval header node         	*/
      pred,     		/* predecessor node         		*/ 
      latchNode;    	/* latching node (in case of loops) */
  Int i,        		/* counter              			*/ 
      level = 0;    	/* derived sequence level       	*/
  interval *initInt;    /* initial interval         		*/
  queue *intNodes;  	/* list of interval nodes       	*/

    /* Structure loops */
    while (derivedG)    /* for all derived sequences Gi */
    {
       level++;
       Ii = derivedG->Ii;
       while (Ii)       /* for all intervals Ii of Gi */
       {
          latchNode = NULL;
          intNodes  = NULL;

          /* Find interval head (original BB node in G1) and create
           * list of nodes of interval Ii.              */
          initInt = Ii;
          for (i = 1; i < level; i++)
          	initInt = initInt->nodes->node->correspInt;
          intHead = initInt->nodes->node;

          /* Find nodes that belong to the interval (nodes from G1) */
          findNodesInInt (&intNodes, level, Ii);

          /* Find greatest enclosing back edge (if any) */
          for (i = 0; i < intHead->numInEdges; i++)
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
            	findNodesInLoop(latchNode, intHead, pProc, intNodes);
				latchNode->flg |= IS_LATCH_NODE;
         	}
          }

          /* Next interval */
          Ii = Ii->next;
       }

       /* Next derived sequence */
       derivedG = derivedG->next;
    }
}


static boolT successor (Int s, Int h, PPROC pProc)
/* Returns whether the BB indexed by s is a successor of the BB indexed by
 * h.  Note that h is a case node.                  */
{ Int i;
  PBB header;

    header = pProc->dfsLast[h];
    for (i = 0; i < header->numOutEdges; i++)
      if (header->edges[i].BBptr->dfsLastNum == s)
         return (TRUE);
    return (FALSE);
}


static void tagNodesInCase (PBB pBB, nodeList **l, Int head, Int tail)
/* Recursive procedure to tag nodes that belong to the case described by
 * the list l, head and tail (dfsLast index to first and exit node of the 
 * case).                               */
{ Int current,      /* index to current node */
      i;

    pBB->traversed = DFS_CASE;
    current = pBB->dfsLastNum;
    if ((current != tail) && (pBB->nodeType != MULTI_BRANCH) && 
        (inList (*l, pBB->immedDom)))
    {
       insertList (l, current);
       pBB->caseHead = head;
       for (i = 0; i < pBB->numOutEdges; i++)
          if (pBB->edges[i].BBptr->traversed != DFS_CASE)
             tagNodesInCase (pBB->edges[i].BBptr, l, head, tail);
    }
}


static void structCases(PPROC pProc)
/* Structures case statements.  This procedure is invoked only when pProc
 * has a case node.                         */
{ Int i, j;
  PBB caseHeader;       		/* case header node         */
  Int exitNode = NO_NODE;   	/* case exit node           */
  nodeList *caseNodes = NULL;   /* temporary: list of nodes in case */

    /* Linear scan of the nodes in reverse dfsLast order, searching for
     * case nodes                           */
	for (i = pProc->numBBs - 1; i >= 0; i--)
		if (pProc->dfsLast[i]->nodeType == MULTI_BRANCH)
		{
			caseHeader = pProc->dfsLast[i];

			/* Find descendant node which has as immediate predecessor 
			 * the current header node, and is not a successor.    */
			for (j = i + 2; j < pProc->numBBs; j++)
			{
				if ((!successor(j, i, pProc)) && 
										(pProc->dfsLast[j]->immedDom == i))
					if (exitNode == NO_NODE)
              			exitNode = j;
					else if (pProc->dfsLast[exitNode]->numInEdges <
                			 pProc->dfsLast[j]->numInEdges) 
              			exitNode = j;
         	}
			pProc->dfsLast[i]->caseTail = exitNode;
         
			/* Tag nodes that belong to the case by recording the
			 * header field with caseHeader.           */
			insertList (&caseNodes, i); 
			pProc->dfsLast[i]->caseHead = i;
			for (j = 0; j < caseHeader->numOutEdges; j++)
        		tagNodesInCase (caseHeader->edges[j].BBptr, &caseNodes, i,
								exitNode);
			if (exitNode != NO_NODE)
            	pProc->dfsLast[exitNode]->caseHead = i;
		}
}


static void flagNodes (nodeList **l, Int f, PPROC pProc)
/* Flags all nodes in the list l as having follow node f, and deletes all
 * nodes from the list.                         */
{  nodeList *p;

    p = *l;
    while (p)
    {
       pProc->dfsLast[p->nodeIdx]->ifFollow = f;
       p = p->next;
       free (*l);
       *l = p;
    }
    *l = NULL;
}


static void structIfs (PPROC pProc)
/* Structures if statements */
{  Int curr,    				/* Index for linear scan of nodes   	*/
       desc,    				/* Index for descendant         		*/
	   followInEdges,			/* Largest # in-edges so far 			*/
       follow;  				/* Possible follow node 				*/
   nodeList *domDesc = NULL,    /* List of nodes dominated by curr  	*/
        *unresolved = NULL, 	/* List of unresolved if nodes  		*/
        *l;         			/* Temporary list       				*/
   PBB currNode,    			/* Pointer to current node  			*/
		pbb;

	/* Linear scan of nodes in reverse dfsLast order */
    for (curr = pProc->numBBs - 1; curr >= 0; curr--)
    {
		currNode = pProc->dfsLast[curr];
		if (currNode->flg & INVALID_BB)		/* Do not process invalid BBs */
			continue;

		if ((currNode->nodeType == TWO_BRANCH) &&
			(! (pProc->Icode.GetLlFlag(currNode->start + currNode->length - 1)
				& JX_LOOP)))
		{
			followInEdges = 0;
			follow = 0;

			/* Find all nodes that have this node as immediate dominator */
			for (desc = curr+1; desc < pProc->numBBs; desc++)
			{
				if (pProc->dfsLast[desc]->immedDom == curr)  {
					insertList (&domDesc, desc);
					pbb = pProc->dfsLast[desc];
					if ((pbb->numInEdges - pbb->numBackEdges) >= followInEdges)
					{
						follow = desc;
						followInEdges = pbb->numInEdges - pbb->numBackEdges;
					}
				}
			}

			/* Determine follow according to number of descendants
			 * immediately dominated by this node  */
			if ((follow != 0) && (followInEdges > 1))
			{
				currNode->ifFollow = follow;
				if (unresolved)
					flagNodes (&unresolved, follow, pProc);
			}
			else
				insertList (&unresolved, curr);
		}
		freeList (&domDesc);
	}
}


void compoundCond (PPROC pproc)
/* Checks for compound conditions of basic blocks that have only 1 high 
 * level instruction.  Whenever these blocks are found, they are merged
 * into one block with the appropriate condition */
{ Int i, j, k, numOutEdges;
  PBB pbb, t, e, obb, pred;
  PICODE picode, ticode;
  COND_EXPR *exp;
 TYPEADR_TYPE *edges;
  boolT change;

  change = TRUE;
  while (change)
  {
	change = FALSE;

	/* Traverse nodes in postorder, this way, the header node of a
	 * compound condition is analysed first */
	for (i = 0; i < pproc->numBBs; i++)
	{
		pbb = pproc->dfsLast[i];
		if (pbb->flg & INVALID_BB)
			continue;

		if (pbb->nodeType == TWO_BRANCH)
		{
			t = pbb->edges[THEN].BBptr;
			e = pbb->edges[ELSE].BBptr;

			/* Check (X || Y) case */
			if ((t->nodeType == TWO_BRANCH) && (t->numHlIcodes == 1) && 
				(t->numInEdges == 1) && (t->edges[ELSE].BBptr == e)) 
			{
				obb = t->edges[THEN].BBptr;

				/* Construct compound DBL_OR expression */
				picode = pproc->Icode.GetIcode(pbb->start + pbb->length -1);
				ticode = pproc->Icode.GetIcode(t->start + t->length -1);
				exp = boolCondExp (picode->ic.hl.oper.exp,
								   ticode->ic.hl.oper.exp, DBL_OR);
				picode->ic.hl.oper.exp = exp;

				/* Replace in-edge to obb from t to pbb */
				for (j = 0; j < obb->numInEdges; j++)
					if (obb->inEdges[j] == t)
					{
						obb->inEdges[j] = pbb;
						break;
					}

				/* New THEN out-edge of pbb */
				pbb->edges[THEN].BBptr = obb;

				/* Remove in-edge t to e */
				for (j = 0; j < (e->numInEdges-1); j++)
					if (e->inEdges[j] == t)
					{
						memmove (&e->inEdges[j], &e->inEdges[j+1], 
								 (size_t)((e->numInEdges - j - 1) * sizeof(PBB)));
						break;
					}
				e->numInEdges--;	/* looses 1 arc */
				t->flg |= INVALID_BB;

				if (pbb->flg & IS_LATCH_NODE)
					pproc->dfsLast[t->dfsLastNum] = pbb;
				else 
					i--;		/* to repeat this analysis */

				change = TRUE;
			}

			/* Check (!X && Y) case */
			else if ((t->nodeType == TWO_BRANCH) && (t->numHlIcodes == 1) && 
				(t->numInEdges == 1) && (t->edges[THEN].BBptr == e)) 
			{
				obb = t->edges[ELSE].BBptr;

				/* Construct compound DBL_AND expression */
				picode = pproc->Icode.GetIcode(pbb->start + pbb->length -1);
				ticode = pproc->Icode.GetIcode(t->start + t->length -1);
				inverseCondOp (&picode->ic.hl.oper.exp);
				exp = boolCondExp (picode->ic.hl.oper.exp,
								   ticode->ic.hl.oper.exp, DBL_AND);
				picode->ic.hl.oper.exp = exp;

				/* Replace in-edge to obb from t to pbb */
				for (j = 0; j < obb->numInEdges; j++)
					if (obb->inEdges[j] == t)
					{
						obb->inEdges[j] = pbb;
						break;
					}

				/* New THEN and ELSE out-edges of pbb */
				pbb->edges[THEN].BBptr = e;
				pbb->edges[ELSE].BBptr = obb;
				
				/* Remove in-edge t to e */
				for (j = 0; j < (e->numInEdges-1); j++)
					if (e->inEdges[j] == t)
					{
						memmove (&e->inEdges[j], &e->inEdges[j+1], 
								 (size_t)((e->numInEdges - j - 1) * sizeof(PBB)));
						break;
					}
				e->numInEdges--;	/* looses 1 arc */
				t->flg |= INVALID_BB;

				if (pbb->flg & IS_LATCH_NODE)
					pproc->dfsLast[t->dfsLastNum] = pbb;
				else 
					i--;		/* to repeat this analysis */

				change = TRUE;
			}

			/* Check (X && Y) case */
			else if ((e->nodeType == TWO_BRANCH) && (e->numHlIcodes == 1) && 
					 (e->numInEdges == 1) && (e->edges[THEN].BBptr == t)) 
			{
				obb = e->edges[ELSE].BBptr;

				/* Construct compound DBL_AND expression */
				picode = pproc->Icode.GetIcode(pbb->start + pbb->length -1);
				ticode = pproc->Icode.GetIcode(t->start + t->length -1);
				exp = boolCondExp (picode->ic.hl.oper.exp,
								   ticode->ic.hl.oper.exp, DBL_AND);
				picode->ic.hl.oper.exp = exp;
				
				/* Replace in-edge to obb from e to pbb */
				for (j = 0; j < obb->numInEdges; j++)
					if (obb->inEdges[j] == e)
					{
						obb->inEdges[j] = pbb;
						break;
					}

				/* New ELSE out-edge of pbb */
				pbb->edges[ELSE].BBptr = obb;

				/* Remove in-edge e to t */
				for (j = 0; j < (t->numInEdges-1); j++)
					if (t->inEdges[j] == e)
					{
						memmove (&t->inEdges[j], &t->inEdges[j+1], 
								 (size_t)((t->numInEdges - j - 1) * sizeof(PBB)));
						break;
					}
				t->numInEdges--;	/* looses 1 arc */
				e->flg |= INVALID_BB;

				if (pbb->flg & IS_LATCH_NODE)
					pproc->dfsLast[e->dfsLastNum] = pbb;
				else 
					i--;		/* to repeat this analysis */

				change = TRUE;
			}

			/* Check (!X || Y) case */
			else if ((e->nodeType == TWO_BRANCH) && (e->numHlIcodes == 1) && 
					 (e->numInEdges == 1) && (e->edges[ELSE].BBptr == t)) 
			{
				obb = e->edges[THEN].BBptr;

				/* Construct compound DBL_OR expression */
				picode = pproc->Icode.GetIcode(pbb->start + pbb->length -1);
				ticode = pproc->Icode.GetIcode(t->start + t->length -1);
				inverseCondOp (&picode->ic.hl.oper.exp);
				exp = boolCondExp (picode->ic.hl.oper.exp,
								   ticode->ic.hl.oper.exp, DBL_OR);
				picode->ic.hl.oper.exp = exp;
				
				/* Replace in-edge to obb from e to pbb */
				for (j = 0; j < obb->numInEdges; j++)
					if (obb->inEdges[j] == e)
					{
						obb->inEdges[j] = pbb;
						break;
					}

				/* New THEN and ELSE out-edges of pbb */
				pbb->edges[THEN].BBptr = obb;
				pbb->edges[ELSE].BBptr = t;

				/* Remove in-edge e to t */
				for (j = 0; j < (t->numInEdges-1); j++)
					if (t->inEdges[j] == e)
					{
						memmove (&t->inEdges[j], &t->inEdges[j+1], 
								 (size_t)((t->numInEdges - j - 1) * sizeof(PBB)));
						break;
					}
				t->numInEdges--;	/* looses 1 arc */
				e->flg |= INVALID_BB;

				if (pbb->flg & IS_LATCH_NODE)
					pproc->dfsLast[e->dfsLastNum] = pbb;
				else 
					i--;		/* to repeat this analysis */

				change = TRUE;
			}
		}
	}
  }
}


void structure(PPROC pProc, derSeq *derivedG)
/* Structuring algorithm to find the structures of the graph pProc->cfg */
{
    /* Find immediate dominators of the graph */
    findImmedDom(pProc);
    if (pProc->hasCase)
       structCases(pProc); 
    structLoops(pProc, derivedG); 
    structIfs(pProc); 
}
