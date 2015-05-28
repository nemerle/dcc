/********************************************************************
 *  Checks for reducibility of a graph by intervals, and
 *  constructs an equivalent reducible graph if one is not found.
 * (C) Cristina Cifuentes
 ********************************************************************/

#include "dcc.h"
#include <stdio.h>
#ifdef __BORLAND__
#include <alloc.h>
#else
#include <malloc.h>		/* For free() */
#endif
#include <string.h>

static Int      numInt;     /* Number of intervals      */


#define nonEmpty(q)     (q != NULL)
/* Returns whether the queue q is empty or not */

#define trivialGraph(G)     (G->numOutEdges == 0)
/* Returns whether the graph is a trivial graph or not */


static BB *firstOfQueue (queue **Q)
/* Returns the first element in the queue Q, and removes this element
 * from the list.  Q is not an empty queue.                 */
{ queue *elim;
  BB *first;

    elim  = *Q;         /* Pointer to first node */
    first = (*Q)->node;     /* First element */
    *Q    = (*Q)->next;     /* Pointer to next node */
    free (elim);            /* Free storage */
    return (first);
}


queue *appendQueue (queue **Q, BB *node)
/* Appends pointer to node at the end of the queue Q if node is not present
 * in this queue.  Returns the queue node just appended.        */
{ queue *pq, *l;

    pq = allocStruc(queue);
    pq->node = node;
    pq->next = NULL;

    if (Q)
        if (*Q)
        {
           for (l = *Q; l->next && l->node != node; l = l->next)
              ;
           if (l->node != node)
              l->next = pq;
        }
        else        /* (*Q) == NULL */
           *Q = pq;
    return (pq);
}


static BB *firstOfInt (interval *pI)
/* Returns the next unprocessed node of the interval list (pointed to by
 * pI->currNode).  Removes this element logically from the list, by updating
 * the currNode pointer to the next unprocessed element.  */
{ queue *pq;

    pq = pI->currNode;
    if (pq == NULL)
       return (NULL);
    pI->currNode = pq->next;
    return (pq->node);
}


static queue *appendNodeInt (queue *pqH, BB *node, interval *pI)
/* Appends node node to the end of the interval list I, updates currNode
 * if necessary, and removes the node from the header list H if it is 
 * there.  The interval header information is placed in the field 
 * node->inInterval.
 * Note: nodes are added to the interval list in interval order (which
 * topsorts the dominance relation).                    */
{ queue *pq,        /* Pointer to current node of the list      */
         *prev;     /* Pointer to previous node in the list     */

    /* Append node if it is not already in the interval list */
    pq = appendQueue (&pI->nodes, node);

    /* Update currNode if necessary */
    if (pI->currNode == NULL)
       pI->currNode = pq;

    /* Check header list for occurrence of node, if found, remove it 
     * and decrement number of out-edges from this interval.    */
    if (node->beenOnH && pqH)
    {
       prev = pqH;
       for (pq = prev; pq && pq->node != node; pq = pq->next)
          prev = pq;
       if (pq == prev)
       {
          pqH = pqH->next;
          pI->numOutEdges -= (byte)pq->node->numInEdges - 1;
       }
       else if (pq)
       {
          prev->next = pq->next;
          pI->numOutEdges -= (byte)pq->node->numInEdges - 1;
       }
    }

    /* Update interval header information for this basic block */
    node->inInterval = pI;

    return (pqH);
}


static void findIntervals (derSeq *derivedGi)
/* Finds the intervals of graph derivedGi->Gi and places them in the list 
 * of intervals derivedGi->Ii.
 * Algorithm by M.S.Hecht.                      */
{  interval *pI,        /* Interval being processed         */ 
        *J;         /* ^ last interval in derivedGi->Ii */
   BB *h,           /* Node being processed         */
      *header,          /* Current interval's header node   */
      *succ;            /* Successor basic block        */
   Int i;           /* Counter              */
   queue *H;            /* Queue of possible header nodes   */
   boolT first = TRUE;       /* First pass through the loop      */

    H = appendQueue (NULL, derivedGi->Gi);  /* H = {first node of G} */
    derivedGi->Gi->beenOnH = TRUE;
    derivedGi->Gi->reachingInt = allocStruc(BB);    /* ^ empty BB */
	memset (derivedGi->Gi->reachingInt, 0, sizeof(BB));

    /* Process header nodes list H */
    while (nonEmpty (H))
    {
       header = firstOfQueue (&H);
	   pI = allocStruc(interval);
	   memset (pI, 0, sizeof(interval));
       pI->numInt = (byte)numInt++;
       if (first)               /* ^ to first interval  */
          derivedGi->Ii = J = pI;
       H = appendNodeInt (H, header, pI);   /* pI(header) = {header} */

       /* Process all nodes in the current interval list */
       while ((h = firstOfInt (pI)) != NULL)
       { 
         /* Check all immediate successors of h */
         for (i = 0; i < h->numOutEdges; i++)
         {
               succ = h->edges[i].BBptr;
               succ->inEdgeCount--;

               if (succ->reachingInt == NULL)   /* first visit */
               {
              succ->reachingInt = header;
              if (succ->inEdgeCount == 0) 
                     H = appendNodeInt (H, succ, pI);
              else if (! succ->beenOnH) /* out edge */
              {
                 appendQueue (&H, succ);
                 succ->beenOnH = TRUE;
                 pI->numOutEdges++;
              }
               }
               else     /* node has been visited before */
              if (succ->inEdgeCount == 0)
              {
                 if (succ->reachingInt == header || 
                 succ->inInterval == pI) /* same interval */
                 {
                    if (succ != header)
                                H = appendNodeInt (H, succ, pI);
                 }
                 else            /* out edge */
                pI->numOutEdges++;
              }
              else if (succ != header && succ->beenOnH)
                   pI->numOutEdges++;
            }
       }

       /* Link interval I to list of intervals */
       if (! first)
       {
          J->next = pI;
          J = pI;
       }
       else     /* first interval */
          first = FALSE;
    }
}


static void displayIntervals (interval *pI)
/* Displays the intervals of the graph Gi.              */
{ queue *nodePtr;

    while (pI)
    {
       nodePtr = pI->nodes;
       printf ("  Interval #: %ld\t#OutEdges: %ld\n", 
           pI->numInt, pI->numOutEdges);

       while (nodePtr)
       {
          if (nodePtr->node->correspInt == NULL)    /* real BBs */
             printf ("    Node: %ld\n", nodePtr->node->start);
          else              /* BBs represent intervals */
         printf ("   Node (corresp int): %d\n",
             nodePtr->node->correspInt->numInt);
          nodePtr = nodePtr->next;
       }
       pI = pI->next;
    }
}


static derSeq *newDerivedSeq()
/* Allocates space for a new derSeq node. */
{ derSeq *pder;

	pder = allocStruc(derSeq);
	memset (pder, 0, sizeof(derSeq));
    return (pder);
}


static void freeQueue (queue **q)
/* Frees the storage allocated for the queue q*/
{ queue *queuePtr;

    for (queuePtr = *q; queuePtr; queuePtr = *q)
    {
       *q = (*q)->next;
       free (queuePtr);
    }
}


static void freeInterval (interval **pI)
/* Frees the storage allocated for the interval pI */
{ interval *Iptr;

    while (*pI)
    {
       freeQueue (&((*pI)->nodes));
       Iptr = *pI;
       *pI = (*pI)->next;
       free (Iptr);
    }
}


void freeDerivedSeq(derSeq *derivedG)
/* Frees the storage allocated by the derived sequence structure, except 
 * for the original graph cfg (derivedG->Gi).               */
{ derSeq *derivedGi;

    while (derivedG)
    {
       freeInterval (&(derivedG->Ii));
       if (derivedG->Gi->nodeType == INTERVAL_NODE)
        freeCFG (derivedG->Gi);
       derivedGi = derivedG;
       derivedG  = derivedG->next;
       free (derivedGi);
    }
}


static boolT nextOrderGraph (derSeq *derivedGi)
/* Finds the next order graph of derivedGi->Gi according to its intervals 
 * (derivedGi->Ii), and places it in derivedGi->next->Gi.       */
{ interval *Ii;     /* Interval being processed         */
  BB *BBnode,       /* New basic block of intervals         */
     *curr,     /* BB being checked for out edges       */
     *succ,     /* Successor node               */
     derInt;
  queue *listIi;    /* List of intervals                */
  Int i,        /* Index to outEdges array          */
      j;        /* Index to successors              */
  boolT   sameGraph; /* Boolean, isomorphic graphs           */

    /* Process Gi's intervals */
    derivedGi->next = newDerivedSeq();
    Ii = derivedGi->Ii;
    sameGraph = TRUE;
    derInt.next = NULL;
    BBnode = &derInt;

    while (Ii) {
       i = 0;
       BBnode = newBB (BBnode, -1, -1, INTERVAL_NODE, Ii->numOutEdges, NULL);
       BBnode->correspInt = Ii;
       listIi = Ii->nodes;

       /* Check for more than 1 interval */
       if (sameGraph && listIi->next)
          sameGraph = FALSE;

       /* Find out edges */
       if (BBnode->numOutEdges > 0)
          while (listIi) {
         curr = listIi->node;
         for (j = 0; j < curr->numOutEdges; j++) {
             succ = curr->edges[j].BBptr;
             if (succ->inInterval != curr->inInterval)
                BBnode->edges[i++].intPtr = succ->inInterval;
         }
             listIi = listIi->next;
          }

       /* Next interval */
       Ii = Ii->next;
    }

    /* Convert list of pointers to intervals into a real graph.
     * Determines the number of in edges to each new BB, and places it
     * in numInEdges and inEdgeCount for later interval processing. */
    curr = derivedGi->next->Gi = derInt.next;
    while (curr) {
       for (i = 0; i < curr->numOutEdges; i++) {
           BBnode = derivedGi->next->Gi;    /* BB of an interval */
           while (BBnode && curr->edges[i].intPtr != BBnode->correspInt)
          BBnode = BBnode->next;
           if (BBnode) {
          curr->edges[i].BBptr = BBnode;
          BBnode->numInEdges++;
          BBnode->inEdgeCount++;
           }
           else
          fatalError (INVALID_INT_BB);
       }
       curr = curr->next;
    }
    return (boolT)(! sameGraph);
}



static byte findDerivedSeq (derSeq *derivedGi)
/* Finds the derived sequence of the graph derivedG->Gi (ie. cfg).
 * Constructs the n-th order graph and places all the intermediate graphs
 * in the derivedG list sequence.                   */
{  BB *Gi;      /* Current derived sequence graph       */

    Gi = derivedGi->Gi;
    while (! trivialGraph (Gi))
    {
         /* Find the intervals of Gi and place them in derivedGi->Ii */
         findIntervals (derivedGi);

         /* Create Gi+1 and check if it is equivalent to Gi */
         if (! nextOrderGraph (derivedGi))
            break; 

         derivedGi = derivedGi->next;
         Gi = derivedGi->Gi;
         stats.nOrder++;
    }

    if (! trivialGraph (Gi))
    {
        freeDerivedSeq(derivedGi->next);    /* remove Gi+1 */
        derivedGi->next = NULL;
        return FALSE;
    }
    findIntervals (derivedGi);
    return TRUE;
}


static void nodeSplitting (BB *G)
/* Converts the irreducible graph G into an equivalent reducible one, by
 * means of node splitting.  */
{

}



void displayDerivedSeq(derSeq *derGi)
/* Displays the derived sequence and intervals of the graph G */
{
    Int n = 1;      /* Derived sequence number */
    printf ("\nDerived Sequence Intervals\n");
    while (derGi)
    {
       printf ("\nIntervals for G%lX\n", n++);
       displayIntervals (derGi->Ii);
       derGi = derGi->next;
    }
}


void checkReducibility (PPROC pProc, derSeq **derivedG)
/* Checks whether the control flow graph, cfg, is reducible or not.
 * If it is not reducible, it is converted into an equivalent reducible
 * graph by node splitting.  The derived sequence of graphs built from cfg
 * are returned in the pointer *derivedG.
 */
{ byte    reducible;            /* Reducible graph flag     */

    numInt = 1;         /* reinitialize no. of intervals*/
    stats.nOrder = 1;       /* nOrder(cfg) = 1      */
    *derivedG = newDerivedSeq();
    (*derivedG)->Gi = pProc->cfg;
    reducible = findDerivedSeq(*derivedG);

    if (! reducible) {
       pProc->flg |= GRAPH_IRRED;
       nodeSplitting (pProc->cfg);
    }
}

