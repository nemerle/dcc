/********************************************************************
 *  Checks for reducibility of a graph by intervals, and
 *  constructs an equivalent reducible graph if one is not found.
 * (C) Cristina Cifuentes
 ********************************************************************/
#include <algorithm>
#include <cassert>
#include "dcc.h"
#include <stdio.h>
#include <malloc.h>		/* For free() */
#include <string.h>

static Int      numInt;     /* Number of intervals      */


#define nonEmpty(q)     (q != NULL)
/* Returns whether the queue q is empty or not */
bool trivialGraph(BB *G)
{
    return G->edges.empty();
}
/* Returns whether the graph is a trivial graph or not */


/* Returns the first element in the queue Q, and removes this element
 * from the list.  Q is not an empty queue.                 */
static BB *firstOfQueue (queue &Q)
{
    assert(!Q.empty());
    BB *res=Q.front();
    Q.pop_front();
    return res;
}


/* Appends pointer to node at the end of the queue Q if node is not present
 * in this queue.  Returns the queue node just appended.        */
queue::iterator appendQueue (queue &Q, BB *node)
{
    auto iter=std::find(Q.begin(),Q.end(),node);
    if(iter!=Q.end())
        return iter;
    Q.push_back(node);
    iter=Q.end();
    --iter;
    return iter;
}


/* Returns the next unprocessed node of the interval list (pointed to by
 * pI->currNode).  Removes this element logically from the list, by updating
 * the currNode pointer to the next unprocessed element.  */
BB *interval::firstOfInt ()
{
    auto pq = currNode;
    if (pq == nodes.end())
        return 0;
    ++currNode;
    return *pq;
}


/* Appends node @node to the end of the interval list @pI, updates currNode
 * if necessary, and removes the node from the header list @pqH if it is
 * there.  The interval header information is placed in the field
 * node->inInterval.
 * Note: nodes are added to the interval list in interval order (which
 * topsorts the dominance relation).                    */
static void appendNodeInt (queue &pqH, BB *node, interval *pI)
{
    queue::iterator pq;        /* Pointer to current node of the list      */

    /* Append node if it is not already in the interval list */
    pq = appendQueue (pI->nodes, node);

    /* Update currNode if necessary */
    if (pI->currNode == pI->nodes.end())
        pI->currNode = pq;

    /* Check header list for occurrence of node, if found, remove it
     * and decrement number of out-edges from this interval.    */
    if (node->beenOnH && !pqH.empty())
    {
        auto found_iter=std::find(pqH.begin(),pqH.end(),node);
        if(found_iter!=pqH.end())
        {
            pI->numOutEdges -= (byte)(*found_iter)->inEdges.size() - 1;
            pqH.erase(found_iter);
        }
    }
    /* Update interval header information for this basic block */
    node->inInterval = pI;
}


/* Finds the intervals of graph derivedGi->Gi and places them in the list
 * of intervals derivedGi->Ii.
 * Algorithm by M.S.Hecht.                      */
void derSeq_Entry::findIntervals (Function *c)
{
    interval *pI,        /* Interval being processed         */
            *J;         /* ^ last interval in derivedGi->Ii */
    BB *h,           /* Node being processed         */
            *header,          /* Current interval's header node   */
            *succ;            /* Successor basic block        */
    Int i;           /* Counter              */
    queue H;            /* Queue of possible header nodes   */
    boolT first = TRUE;       /* First pass through the loop      */

    appendQueue (H, Gi);  /* H = {first node of G} */
    Gi->beenOnH = TRUE;
    Gi->reachingInt = BB::Create(0,"",c); /* ^ empty BB */

    /* Process header nodes list H */
    while (!H.empty())
    {
        header = firstOfQueue (H);
        pI = new interval;
        pI->numInt = (byte)numInt++;
        if (first)               /* ^ to first interval  */
            Ii = J = pI;
        appendNodeInt (H, header, pI);   /* pI(header) = {header} */

        /* Process all nodes in the current interval list */
        while ((h = pI->firstOfInt()) != NULL)
        {
            /* Check all immediate successors of h */
            for (i = 0; i < h->edges.size(); i++)
            {
                succ = h->edges[i].BBptr;
                succ->inEdgeCount--;

                if (succ->reachingInt == NULL)   /* first visit */
                {
                    succ->reachingInt = header;
                    if (succ->inEdgeCount == 0)
                        appendNodeInt (H, succ, pI);
                    else if (! succ->beenOnH) /* out edge */
                    {
                        appendQueue (H, succ);
                        succ->beenOnH = TRUE;
                        pI->numOutEdges++;
                    }
                }
                else     /* node has been visited before */
                    if (succ->inEdgeCount == 0)
                    {
                        if (succ->reachingInt == header || succ->inInterval == pI) /* same interval */
                        {
                            if (succ != header)
                                appendNodeInt (H, succ, pI);
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
            first = false;
    }
}


/* Displays the intervals of the graph Gi.              */
static void displayIntervals (interval *pI)
{
    queue::iterator nodePtr;

    while (pI)
    {
        nodePtr = pI->nodes.begin();
        printf ("  Interval #: %ld\t#OutEdges: %ld\n", pI->numInt, pI->numOutEdges);
        while (nodePtr!=pI->nodes.end())
        {
            if ((*nodePtr)->correspInt == NULL)    /* real BBs */
                printf ("    Node: %ld\n", (*nodePtr)->begin());
            else             // BBs represent intervals
                printf ("   Node (corresp int): %d\n", (*nodePtr)->correspInt->numInt);
            ++nodePtr;
        }
        pI = pI->next;
    }
}


/* Allocates space for a new derSeq node. */
static derSeq_Entry *newDerivedSeq()
{
    return new derSeq_Entry;
}


/* Frees the storage allocated for the queue q*/
void freeQueue (queue &q)
{
    q.clear();
}


/* Frees the storage allocated for the interval pI */
static void freeInterval (interval **pI)
{
    interval *Iptr;

    while (*pI)
    {
        (*pI)->nodes.clear();
        Iptr = *pI;
        *pI = (*pI)->next;
        delete (Iptr);
    }
}


/* Frees the storage allocated by the derived sequence structure, except
 * for the original graph cfg (derivedG->Gi).               */
void freeDerivedSeq(derSeq &derivedG)
{
    derivedG.clear();
}
derSeq_Entry::~derSeq_Entry()
{
    freeInterval (&Ii);
    //    if(Gi && Gi->nodeType == INTERVAL_NODE)
    //        freeCFG (Gi);
}

/* Finds the next order graph of derivedGi->Gi according to its intervals
 * (derivedGi->Ii), and places it in derivedGi->next->Gi.       */
bool Function::nextOrderGraph (derSeq *derivedGi)
{
    interval *Ii;   /* Interval being processed         */
    BB *BBnode,     /* New basic block of intervals         */
            *curr,      /* BB being checked for out edges       */
            *succ       /* Successor node               */
            ;
    //queue *listIi;    /* List of intervals                */
    Int i,        /* Index to outEdges array          */
        j;        /* Index to successors              */
    boolT   sameGraph; /* Boolean, isomorphic graphs           */

    /* Process Gi's intervals */
    derSeq_Entry &prev_entry(derivedGi->back());
    derivedGi->push_back(derSeq_Entry());
    derSeq_Entry &new_entry(derivedGi->back());
    Ii = prev_entry.Ii;
    sameGraph = TRUE;
    BBnode = 0;
    std::vector<BB *> bbs;
    while (Ii)
    {
        i = 0;
        bbs.push_back(BB::Create(-1, -1, INTERVAL_NODE, Ii->numOutEdges, this));
        BBnode = bbs.back();
        BBnode->correspInt = Ii;
        const queue &listIi(Ii->nodes);

        /* Check for more than 1 interval */
        if (sameGraph && (listIi.size()>1))
            sameGraph = FALSE;

        /* Find out edges */

        if (BBnode->edges.size() > 0)
        {
            for(BB *curr :  listIi)
            {
                for (j = 0; j < curr->edges.size(); j++)
                {
                    succ = curr->edges[j].BBptr;
                    if (succ->inInterval != curr->inInterval)
                        BBnode->edges[i++].intPtr = succ->inInterval;
                }
            }
        }

        /* Next interval */
        Ii = Ii->next;
    }

    /* Convert list of pointers to intervals into a real graph.
     * Determines the number of in edges to each new BB, and places it
     * in numInEdges and inEdgeCount for later interval processing. */
    curr = new_entry.Gi = bbs.front();
    for(BB *curr : bbs)
    {
        for(TYPEADR_TYPE &edge : curr->edges)
        {
            BBnode = new_entry.Gi;    /* BB of an interval */
            auto iter= std::find_if(bbs.begin(),bbs.end(),
                                    [&edge](BB *node)->bool { return edge.intPtr==node->correspInt;});
        if(iter==bbs.end())
            fatalError (INVALID_INT_BB);
        edge.BBptr = *iter;
        (*iter)->inEdges.push_back(0);
        (*iter)->inEdgeCount++;
    }
}
return (boolT)(! sameGraph);
}



/* Finds the derived sequence of the graph derivedG->Gi (ie. cfg).
 * Constructs the n-th order graph and places all the intermediate graphs
 * in the derivedG list sequence.                   */
byte Function::findDerivedSeq (derSeq *derivedGi)
{
    BB *Gi;      /* Current derived sequence graph       */

    derSeq::iterator iter=derivedGi->begin();
    Gi = iter->Gi;
    while (! trivialGraph (Gi))
    {
        /* Find the intervals of Gi and place them in derivedGi->Ii */
        iter->findIntervals(this);

        /* Create Gi+1 and check if it is equivalent to Gi */
        if (! nextOrderGraph (derivedGi))
            break;
        ++iter;
        Gi = iter->Gi;
        stats.nOrder++;
    }

    if (! trivialGraph (Gi))
    {
        ++iter;
        derivedGi->erase(iter,derivedGi->end()); /* remove Gi+1 */
        //        freeDerivedSeq(derivedGi->next);
        //        derivedGi->next = NULL;
        return FALSE;
    }
    derivedGi->back().findIntervals (this);
    return TRUE;
}

/* Converts the irreducible graph G into an equivalent reducible one, by
 * means of node splitting.  */
static void nodeSplitting (std::vector<BB *> &G)
{
    fprintf(stderr,"Attempt to perform node splitting: NOT IMPLEMENTED\n");
}

/* Displays the derived sequence and intervals of the graph G */
void derSeq::display()
{
    Int n = 1;      /* Derived sequence number */
    printf ("\nDerived Sequence Intervals\n");
    derSeq::iterator iter=this->begin();
    while (iter!=this->end())
    {
        printf ("\nIntervals for G%lX\n", n++);
        displayIntervals (iter->Ii);
        ++iter;
    }
}


/* Checks whether the control flow graph, cfg, is reducible or not.
 * If it is not reducible, it is converted into an equivalent reducible
 * graph by node splitting.  The derived sequence of graphs built from cfg
 * are returned in the pointer *derivedG.
 */
derSeq * Function::checkReducibility()
{
    derSeq * der_seq;
    byte    reducible;  /* Reducible graph flag     */

    numInt = 1;         /* reinitialize no. of intervals*/
    stats.nOrder = 1;   /* nOrder(cfg) = 1      */
    der_seq = new derSeq;
    der_seq->resize(1);
    der_seq->back().Gi = m_cfg.front();
    reducible = findDerivedSeq(der_seq);

    if (! reducible)
    {
        flg |= GRAPH_IRRED;
        nodeSplitting (m_cfg);
    }
    return der_seq;
}

