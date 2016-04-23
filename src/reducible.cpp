/********************************************************************
 *  Checks for reducibility of a graph by intervals, and
 *  constructs an equivalent reducible graph if one is not found.
 * (C) Cristina Cifuentes
 ********************************************************************/
#include <algorithm>
#include <cassert>
#include <cstdio>
#include <cstring>
#include <stdint.h>
#include "dcc.h"

static int      numInt;     /* Number of intervals      */


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
        return nullptr;
    ++currNode;
    return *pq;
}


/* Appends node @node to the end of the interval list @pI, updates currNode
 * if necessary, and removes the node from the header list @pqH if it is
 * there.  The interval header information is placed in the field
 * node->inInterval.
 * Note: nodes are added to the interval list in interval order (which
 * topsorts the dominance relation).                    */
void interval::appendNodeInt(queue &pqH, BB *node)
{
    queue::iterator pq;        /* Pointer to current node of the list      */

    /* Append node if it is not already in the interval list */
    pq = appendQueue (nodes, node);

    /* Update currNode if necessary */
    if (currNode == nodes.end())
        currNode = pq;

    /* Check header list for occurrence of node, if found, remove it
     * and decrement number of out-edges from this interval.    */
    if (node->beenOnH && !pqH.empty())
    {
        auto found_iter=std::find(pqH.begin(),pqH.end(),node);
        if(found_iter!=pqH.end())
        {
            numOutEdges -= (uint8_t)(*found_iter)->inEdges.size() - 1;
            pqH.erase(found_iter);
        }
    }
    /* Update interval header information for this basic block */
    node->inInterval = this;
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
    queue H;            /* Queue of possible header nodes   */
    bool first = true;       /* First pass through the loop      */

    appendQueue (H, Gi);  /* H = {first node of G} */
    Gi->beenOnH = true;
    Gi->reachingInt = BB::Create(nullptr,"",c); /* ^ empty BB */

    /* Process header nodes list H */
    while (!H.empty())
    {
        header = firstOfQueue (H);
        pI = new interval;
        pI->numInt = (uint8_t)numInt++;
        if (first)               /* ^ to first interval  */
        {
            Ii = J = pI;
            m_intervals.push_back(pI);
        }
        pI->appendNodeInt (H, header);   /* pI(header) = {header} */

        /* Process all nodes in the current interval list */
        while ((h = pI->firstOfInt()) != nullptr)
        {
            /* Check all immediate successors of h */
            for (auto & elem : h->edges)
            {
                succ = elem.BBptr;
                succ->inEdgeCount--;

                if (succ->reachingInt == nullptr)   /* first visit */
                {
                    succ->reachingInt = header;
                    if (succ->inEdgeCount == 0)
                        pI->appendNodeInt (H, succ);
                    else if (! succ->beenOnH) /* out edge */
                    {
                        appendQueue (H, succ);
                        succ->beenOnH = true;
                        pI->numOutEdges++;
                    }
                }
                else     /* node has been visited before */
                    if (succ->inEdgeCount == 0)
                    {
                        if (succ->reachingInt == header || succ->inInterval == pI) /* same interval */
                        {
                            if (succ != header)
                                pI->appendNodeInt (H, succ);
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
            m_intervals.push_back(pI);
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

    while (pI)
    {
        printf ("  Interval #: %d\t#OutEdges: %d\n", pI->numInt, pI->numOutEdges);
        for(BB *node : pI->nodes)
        {
            if (node->correspInt == nullptr)    /* real BBs */
                printf ("    Node: %d\n", node->begin()->loc_ip);
            else             // BBs represent intervals
                printf ("   Node (corresp int): %d\n", node->correspInt->numInt);
        }
        pI = pI->next;
    }
}


/* Allocates space for a new derSeq node. */
//static derSeq_Entry *newDerivedSeq()
//{
//    return new derSeq_Entry;
//}


/* Frees the storage allocated for the queue q*/
//static void freeQueue (queue &q)
//{
//    q.clear();
//}


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
bool Function::nextOrderGraph (derSeq &derivedGi)
{
    interval *Ii;   /* Interval being processed         */
    BB *BBnode;     /* New basic block of intervals         */
    bool   sameGraph; /* Boolean, isomorphic graphs           */

    /* Process Gi's intervals */
    derSeq_Entry &prev_entry(derivedGi.back());
    derivedGi.push_back(derSeq_Entry());
    derSeq_Entry &new_entry(derivedGi.back());

    sameGraph = true;
    BBnode = nullptr;
    std::vector<BB *> bbs;
    for(Ii = prev_entry.Ii; Ii != nullptr; Ii = Ii->next)
    {

        BBnode = BB::CreateIntervalBB(this);
        BBnode->correspInt = Ii;
        bbs.push_back(BBnode);
        const queue &listIi(Ii->nodes);

        /* Check for more than 1 interval */
        if (sameGraph && (listIi.size()>1))
            sameGraph = false;

        /* Find out edges */

        if (Ii->numOutEdges <= 0)
            continue;
        for(BB *curr :  listIi)
        {
            for (size_t j = 0; j < curr->edges.size(); j++)
            {
                BB *successor_node = curr->edges[j].BBptr;
                if (successor_node->inInterval != curr->inInterval)
                    BBnode->addOutEdgeInterval(successor_node->inInterval);
            }
        }
    }

    /* Convert list of pointers to intervals into a real graph.
     * Determines the number of in edges to each new BB, and places it
     * in numInEdges and inEdgeCount for later interval processing. */
    //curr = new_entry.Gi = bbs.front();
    new_entry.Gi = bbs.front();
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
            (*iter)->inEdges.push_back((BB *)nullptr);
            (*iter)->inEdgeCount++;
        }
    }
    return !sameGraph;
}



/* Finds the derived sequence of the graph derivedG->Gi (ie. cfg).
 * Constructs the n-th order graph and places all the intermediate graphs
 * in the derivedG list sequence.                   */
bool Function::findDerivedSeq (derSeq &derivedGi)
{
    derSeq::iterator iter=derivedGi.begin();
    assert(iter!=derivedGi.end());
    BB *Gi = iter->Gi;      /* Current derived sequence graph       */
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
        derivedGi.erase(iter,derivedGi.end()); /* remove Gi+1 */
        //        freeDerivedSeq(derivedGi->next);
        //        derivedGi->next = NULL;
        return false;
    }
    derivedGi.back().findIntervals (this);
    return true;
}

/* Displays the derived sequence and intervals of the graph G */
void derSeq::display()
{
    int n = 1;      /* Derived sequence number */
    printf ("\nDerived Sequence Intervals\n");
    derSeq::iterator iter=this->begin();
    while (iter!=this->end())
    {
        printf ("\nIntervals for G%X\n", n++);
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
    uint8_t    reducible;  /* Reducible graph flag     */

    numInt = 1;         /* reinitialize no. of intervals*/
    stats.nOrder = 1;   /* nOrder(cfg) = 1      */
    der_seq = new derSeq;
    der_seq->resize(1);
    der_seq->back().Gi = *m_actual_cfg.begin(); /*m_cfg.front()*/;
    reducible = findDerivedSeq(*der_seq);

    if (! reducible)
    {
        flg |= GRAPH_IRRED;
        m_actual_cfg.nodeSplitting();
    }
    return der_seq;
}

