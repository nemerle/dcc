#pragma once
#include <list>
#include <vector>
#include <bitset>
#include <string>
#include <llvm/ADT/ilist.h>
#include <llvm/ADT/ilist_node.h>
#include "types.h"
#include "graph.h"
//#include "icode.h"
/* Basic block (BB) node definition */
struct Function;
class CIcodeRec;
struct BB;
struct interval;
struct ICODE;
typedef std::list<ICODE>::iterator iICODE;
typedef std::list<ICODE>::reverse_iterator riICODE;
typedef union
{
    uint32_t         ip;             /* Out edge icode address       */
    BB *          BBptr;          /* Out edge pointer to next BB  */
    interval     *intPtr;         /* Out edge ptr to next interval*/
} TYPEADR_TYPE;

struct BB : public llvm::ilist_node<BB>
{
private:
    BB(const BB&);
    BB() : start(0),length(0),nodeType(0),traversed(0),
        numHlIcodes(0),flg(0),
        inEdges(0),
        edges(0),beenOnH(0),inEdgeCount(0),reachingInt(0),
        inInterval(0),correspInt(0),liveUse(0),def(0),liveIn(0),liveOut(0),
        dfsFirstNum(0),dfsLastNum(0),immedDom(0),ifFollow(0),loopType(0),latchNode(0),
        numBackEdges(0),loopHead(0),loopFollow(0),caseHead(0),caseTail(0),index(0)
    {

    }
    //friend class SymbolTableListTraits<BB, Function>;
    //int             numInEdges;     /* Number of in edges           */
    int             start;          /* First instruction offset     */
    int             length;         /* No. of instructions this BB  */

public:
    int    begin();
    iICODE begin2();
    iICODE end2();
    int	   end();
    int    rbegin();
    int    rend();
    riICODE rbegin2();
    riICODE rend2();
    ICODE &front();
    ICODE &back();
    size_t size();
    uint8_t            nodeType;       /* Type of node                 */
    int             traversed;      /* Boolean: traversed yet?      */
    int             numHlIcodes;	/* No. of high-level icodes		*/
    uint32_t         flg;			/* BB flags						*/

    /* In edges and out edges */
    std::vector<BB *> inEdges; // does not own held pointers

    //int             numOutEdges;    /* Number of out edges          */
    std::vector<TYPEADR_TYPE> edges;/* Array of ptrs. to out edges  */

    /* For interval construction */
    int             beenOnH;        /* #times been on header list H */
    int             inEdgeCount;    /* #inEdges (to find intervals) */
    BB *            reachingInt;    /* Reaching interval header     */
    interval       *inInterval;     /* Node's interval              */

    /* For derived sequence construction */
    interval       *correspInt;     /* Corresponding interval in
                                     * derived graph Gi-1       	*/

    /* For live register analysis
         * LiveIn(b) = LiveUse(b) U (LiveOut(b) - Def(b))	*/
    std::bitset<32> liveUse;		/* LiveUse(b)					*/
    std::bitset<32> def;			/* Def(b)						*/
    std::bitset<32> liveIn;			/* LiveIn(b)					*/
    std::bitset<32> liveOut;		/* LiveOut(b)					*/

    /* For structuring analysis */
    int             dfsFirstNum;    /* DFS #: first visit of node   */
    int             dfsLastNum;     /* DFS #: last visit of node    */
    int             immedDom;       /* Immediate dominator (dfsLast
                                     * index)                       */
    int             ifFollow;       /* node that ends the if        */
    int             loopType;       /* Type of loop (if any)        */
    int             latchNode;      /* latching node of the loop    */
    int             numBackEdges;	/* # of back edges				*/
    int             loopHead;       /* most nested loop head to which
                                     * thcis node belongs (dfsLast)  */
    int             loopFollow;     /* node that follows the loop   */
    int             caseHead;       /* most nested case to which this
                                        node belongs (dfsLast)      */
    int             caseTail;       /* tail node for the case       */

    int             index;          /* Index, used in several ways  */
static BB * Create(void *ctx=0,const std::string &s="",Function *parent=0,BB *insertBefore=0);
static BB * Create(int start, int ip, uint8_t nodeType, int numOutEdges, Function * parent);
    void    writeCode(int indLevel, Function *pProc, int *numLoc, int latchNode, int ifFollow);
    void    mergeFallThrough(CIcodeRec &Icode);
    void    dfsNumbering(std::vector<BB *> &dfsLast, int *first, int *last);
    void    displayDfs();
    void    display();
    /// getParent - Return the enclosing method, or null if none
    ///
    const Function *getParent() const { return Parent; }
    Function *getParent()       { return Parent; }
    void writeBB(int lev, Function *pProc, int *numLoc);
    BB *rmJMP(int marker, BB *pBB);
    void genDU1();
private:
    Function *Parent;

};
