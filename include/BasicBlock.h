#pragma once
#include <list>
#include <vector>
#include <string>
#include "types.h"
/* Basic block (BB) node definition */
struct Function;
class CIcodeRec;
struct BB;
struct interval;
typedef union
{
    dword         ip;             /* Out edge icode address       */
    BB *          BBptr;          /* Out edge pointer to next BB  */
    interval     *intPtr;         /* Out edge ptr to next interval*/
} TYPEADR_TYPE;

struct BB
{
protected:
    BB(const BB&);
    BB() : nodeType(0),traversed(0),start(0),length(0),
        numHlIcodes(0),flg(0),
        numInEdges(0),inEdges(0),
        numOutEdges(0),edges(0),beenOnH(0),inEdgeCount(0),reachingInt(0),
        inInterval(0),correspInt(0),liveUse(0),def(0),liveIn(0),liveOut(0),
        dfsFirstNum(0),dfsLastNum(0),immedDom(0),ifFollow(0),loopType(0),latchNode(0),
        numBackEdges(0),loopHead(0),loopFollow(0),caseHead(0),caseTail(0),index(0)
    {

    }

public:
    byte            nodeType;       /* Type of node                 */
    Int             traversed;      /* Boolean: traversed yet?      */
    Int             start;          /* First instruction offset     */
    Int             length;         /* No. of instructions this BB  */
    Int             numHlIcodes;	/* No. of high-level icodes		*/
    flags32         flg;			/* BB flags						*/

    /* In edges and out edges */
    Int             numInEdges;     /* Number of in edges           */
    std::vector<BB *> inEdges; // does not own held pointers

    Int             numOutEdges;    /* Number of out edges          */
    std::vector<TYPEADR_TYPE> edges;/* Array of ptrs. to out edges  */

    /* For interval construction */
    Int             beenOnH;        /* #times been on header list H */
    Int             inEdgeCount;    /* #inEdges (to find intervals) */
    BB *            reachingInt;    /* Reaching interval header     */
    interval       *inInterval;     /* Node's interval              */

    /* For derived sequence construction */
    interval       *correspInt;     /* Corresponding interval in
                                     * derived graph Gi-1       	*/

    /* For live register analysis
         * LiveIn(b) = LiveUse(b) U (LiveOut(b) - Def(b))	*/
    dword           liveUse;		/* LiveUse(b)					*/
    dword           def;			/* Def(b)						*/
    dword           liveIn;			/* LiveIn(b)					*/
    dword           liveOut;		/* LiveOut(b)					*/

    /* For structuring analysis */
    Int             dfsFirstNum;    /* DFS #: first visit of node   */
    Int             dfsLastNum;     /* DFS #: last visit of node    */
    Int             immedDom;       /* Immediate dominator (dfsLast
                                     * index)                       */
    Int             ifFollow;       /* node that ends the if        */
    Int             loopType;       /* Type of loop (if any)        */
    Int             latchNode;      /* latching node of the loop    */
    Int             numBackEdges;	/* # of back edges				*/
    Int             loopHead;       /* most nested loop head to which
                                     * thcis node belongs (dfsLast)  */
    Int             loopFollow;     /* node that follows the loop   */
    Int             caseHead;       /* most nested case to which this
                                        node belongs (dfsLast)      */
    Int             caseTail;       /* tail node for the case       */

    Int             index;          /* Index, used in several ways  */
    static BB *Create(void *ctx=0,const std::string &s="",Function *parent=0,BB *insertBefore=0);
    static BB *Create(Int start, Int ip, byte nodeType, Int numOutEdges, Function * parent);
    void writeCode(Int indLevel, Function *pProc, Int *numLoc, Int latchNode, Int ifFollow);
    void mergeFallThrough(CIcodeRec &Icode);
    void dfsNumbering(std::vector<BB *> &dfsLast, Int *first, Int *last);
    void displayDfs();
};
