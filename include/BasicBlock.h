#pragma once
#include <list>
#include <vector>
#include <bitset>
#include <string>
#include <llvm/ADT/ilist.h>
#include <llvm/ADT/ilist_node.h>
#include <boost/range/iterator_range.hpp>
#include "icode.h"
#include "types.h"
#include "graph.h"
//#include "icode.h"
/* Basic block (BB) node definition */
struct Function;
class CIcodeRec;
struct BB;
struct LOCAL_ID;
struct interval;
//TODO: consider default address value -> INVALID
struct TYPEADR_TYPE
{
    uint32_t         ip;             /* Out edge icode address       */
    BB *          BBptr;          /* Out edge pointer to next BB  */
    interval     *intPtr;         /* Out edge ptr to next interval*/
    TYPEADR_TYPE(uint32_t addr=0) : ip(addr),BBptr(nullptr),intPtr(nullptr)
    {}
    TYPEADR_TYPE(interval *v) : ip(0),BBptr(nullptr),intPtr(v)
    {}
};
struct BB : public llvm::ilist_node<BB>
{
    friend struct Function;
private:
    BB(const BB&);
    BB() : nodeType(0),traversed(DFS_NONE),
        numHlIcodes(0),flg(0),
        inEdges(0),
        edges(0),beenOnH(0),inEdgeCount(0),reachingInt(0),
        inInterval(0),correspInt(0),
        dfsFirstNum(0),dfsLastNum(0),immedDom(0),ifFollow(0),loopType(0),latchNode(0),
        numBackEdges(0),loopHead(0),loopFollow(0),caseHead(0),caseTail(0),index(0)
    {

    }
    //friend class SymbolTableListTraits<BB, Function>;
    typedef boost::iterator_range<iICODE> rCODE;
    rCODE instructions;
    rCODE &my_range() {return instructions;}

public:
    struct ValidFunctor
    {
        bool operator()(BB *p) {return p->valid();}
    };
    iICODE begin();
    iICODE end() const;
    riICODE rbegin();
    riICODE rend();
    ICODE &front();
    ICODE &back();
    size_t size();
    uint8_t            nodeType;       /* Type of node                 */
    eDFS             traversed;      /* last traversal id is held here traversed yet?      */
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
    interval       *correspInt;     //!< Corresponding interval in derived graph Gi-1
    // For live register analysis
    //  LiveIn(b) = LiveUse(b) U (LiveOut(b) - Def(b))
    LivenessSet liveUse;		/* LiveUse(b)					*/
    LivenessSet def;                /* Def(b)						*/
    LivenessSet liveIn;             /* LiveIn(b)					*/
    LivenessSet liveOut;		/* LiveOut(b)					*/

    /* For structuring analysis */
    int             dfsFirstNum;    /* DFS #: first visit of node   */
    int             dfsLastNum;     /* DFS #: last visit of node    */
    int             immedDom;       /* Immediate dominator (dfsLast index) */
    int             ifFollow;       /* node that ends the if        */
    int             loopType;       /* Type of loop (if any)        */
    int             latchNode;      /* latching node of the loop    */
    size_t          numBackEdges;   /* # of back edges              */
    int             loopHead;       /* most nested loop head to which this node belongs (dfsLast)  */
    int             loopFollow;     /* node that follows the loop   */
    int             caseHead;       /* most nested case to which this node belongs (dfsLast)      */
    int             caseTail;       /* tail node for the case       */

    int             index;          /* Index, used in several ways  */
    static BB * Create(void *ctx=0,const std::string &s="",Function *parent=0,BB *insertBefore=0);
    static BB * CreateIntervalBB(Function *parent);
    static BB * Create(const rCODE &r, uint8_t _nodeType, Function *parent);
    void    writeCode(int indLevel, Function *pProc, int *numLoc, int latchNode, int ifFollow);
    void    mergeFallThrough(CIcodeRec &Icode);
    void    dfsNumbering(std::vector<BB *> &dfsLast, int *first, int *last);
    void    displayDfs();
    void    display();
    /// getParent - Return the enclosing method, or null if none
    ///
    const Function *getParent() const { return Parent; }
    Function *getParent()       { return Parent; }
    void    writeBB(std::ostream &ostr, int lev, Function *pProc, int *numLoc);
    BB *    rmJMP(int marker, BB *pBB);
    void    genDU1();
    void findBBExps(LOCAL_ID &locals, Function *f);
    bool    valid() {return 0==(flg & INVALID_BB); }
    bool    wasTraversedAtLevel(int l) const {return traversed==l;}
    ICODE * writeLoopHeader(int &indLevel, Function* pProc, int *numLoc, BB *&latch, bool &repCond);
    void    addOutEdge(uint32_t ip)  // TODO: fix this
    {
        edges.push_back(TYPEADR_TYPE(ip));
    }
    void    addOutEdgeInterval(interval *i)  // TODO: fix this
    {
        edges.push_back(TYPEADR_TYPE(i));
    }

    void RemoveUnusedDefs(eReg regi, int defRegIdx, iICODE picode);
private:
    bool    FindUseBeforeDef(eReg regi, int defRegIdx, iICODE start_at);
    void    ProcessUseDefForFunc(eReg regi, int defRegIdx, ICODE &picode);
    bool    isEndOfPath(int latch_node_idx) const;
    Function *Parent;

};
