/*
 ****************************************************************************
 *      CFG, BB and interval related definitions
 * ( C ) Cristina Cifuentes
 ****************************************************************************
 */
#pragma once
#include <stdint.h>
#include <list>

struct Function;
/* Types of basic block nodes */
/* Real basic blocks: type defined according to their out-edges */
enum eBBKind
{
    ONE_BRANCH = 0,   /* unconditional branch */
    TWO_BRANCH = 1,   /* conditional branch   */
    MULTI_BRANCH=2,   /* case branch          */
    FALL_NODE=3,   /* fall through         */
    RETURN_NODE=4,   /* procedure/program return */
    CALL_NODE=5,   /* procedure call       */
    LOOP_NODE=6,   /* loop instruction     */
    REP_NODE=7,   /* repeat instruction   */
    INTERVAL_NODE=8,   /* contains interval list */

    TERMINATE_NODE=11,   /* Exit to DOS          */
    NOWHERE_NODE=12   /* No outedges going anywhere */
};


/* Depth-first traversal constants */
enum eDFS
{
    DFS_NONE,
    DFS_DISP=1,   /* Display graph pass   */
    DFS_MERGE=2,   /* Merge nodes pass     */
    DFS_NUM=3,   /* DFS numbering pass   */
    DFS_CASE=4,   /* Case pass            */
    DFS_ALPHA=5,   /* Alpha code generation*/
    DFS_JMP=9   /* rmJMP pass - must be largest flag */
};

/* Control flow analysis constants */
enum eNodeHeaderType
{
    NO_TYPE=0,   /* node is not a loop header*/
    WHILE_TYPE=1,   /* node is a while header   */
    REPEAT_TYPE=2,   /* node is a repeat header  */
    ENDLESS_TYPE=3   /* endless loop header      */
};

/* Uninitialized values for certain fields */
#define NO_NODE         MAX /* node has no associated node  */
#define NO_DOM          MAX /* node has no dominator    	*/
#define UN_INIT         MAX /* uninitialized variable   	*/

#define THEN            0   /* then edge            */
#define ELSE            1   /* else edge            */

/* Basic Block (BB) flags */

#define INVALID_BB      0x0001		/* BB is not valid any more 		 */
#define IS_LATCH_NODE	0x0002		/* BB is the latching node of a loop */

struct BB;
/* Interval structure */
typedef std::list<BB *> queue;

struct interval
{
    uint8_t         numInt=0;         /* # of the interval    */
    uint8_t         numOutEdges=0;    /* Number of out edges  */
    queue           nodes;         /* Nodes of the interval*/
    queue::iterator currNode;      /* Current node     */
    interval *      next=0;          /* Next interval    */
    BB *            firstOfInt();
                    interval() : currNode(nodes.end()){
                    }
    void            appendNodeInt(queue &pqH, BB *node);
};


/* Derived Sequence structure */
struct derSeq_Entry
{
    BB *                Gi=nullptr;        /* Graph pointer        */
    std::list<interval *> m_intervals;
    interval *          Ii=nullptr;        /* Interval list of Gi  */
    ~derSeq_Entry();
public:
    void findIntervals(Function *c);
};
class derSeq : public std::list<derSeq_Entry>
{
public:
    void display();
};
void    freeDerivedSeq(derSeq &derivedG);                   /* reducible.c  */

