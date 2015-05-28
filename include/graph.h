/*****************************************************************************
 *      CFG, BB and interval related definitions
 * (C) Cristina Cifuentes
 ****************************************************************************/


/* Types of basic block nodes */
/* Real basic blocks: type defined according to their out-edges */
#define ONE_BRANCH      0   /* unconditional branch */
#define TWO_BRANCH      1   /* conditional branch   */
#define MULTI_BRANCH    2   /* case branch          */
#define FALL_NODE       3   /* fall through         */
#define RETURN_NODE     4   /* procedure/program return */
#define CALL_NODE       5   /* procedure call       */
#define LOOP_NODE       6   /* loop instruction     */
#define REP_NODE        7   /* repeat instruction   */
#define INTERVAL_NODE   8   /* contains interval list */

#define TERMINATE_NODE 11   /* Exit to DOS          */
#define NOWHERE_NODE   12   /* No outedges going anywhere */


/* Depth-first traversal constants */
#define DFS_DISP        1   /* Display graph pass   */
#define DFS_MERGE       2   /* Merge nodes pass     */
#define DFS_NUM         3   /* DFS numbering pass   */
#define DFS_CASE        4   /* Case pass            */
#define DFS_ALPHA       5   /* Alpha code generation*/
#define DFS_JMP         9   /* rmJMP pass - must be largest flag */

/* Control flow analysis constants */
#define NO_TYPE         0   /* node is not a loop header*/
#define WHILE_TYPE      1   /* node is a while header   */
#define REPEAT_TYPE     2   /* node is a repeat header  */
#define ENDLESS_TYPE    3   /* endless loop header      */

/* Uninitialized values for certain fields */
#define NO_NODE         MAX /* node has no associated node  */
#define NO_DOM          MAX /* node has no dominator    	*/
#define UN_INIT         MAX /* uninitialized variable   	*/

#define THEN            0   /* then edge            */
#define ELSE            1   /* else edge            */

/* Basic Block (BB) flags */
#define INVALID_BB		0x0001		/* BB is not valid any more 		 */
#define IS_LATCH_NODE	0x0002		/* BB is the latching node of a loop */


/* Interval structure */
typedef struct _queueNode {
    struct _BB         *node;   /* Ptr to basic block   */
    struct _queueNode  *next;
} queue;

typedef struct _intNode {
    byte            numInt;         /* # of the interval    */
    byte            numOutEdges;    /* Number of out edges  */
    queue           *nodes;         /* Nodes of the interval*/
    queue           *currNode;      /* Current node     */
    struct _intNode *next;          /* Next interval    */
} interval;

typedef union
{
      dword         ip;             /* Out edge icode address       */
      struct _BB   *BBptr;          /* Out edge pointer to next BB  */
      interval     *intPtr;         /* Out edge ptr to next interval*/
} TYPEADR_TYPE;

/* Basic block (BB) node definition */
typedef struct _BB {
    byte            nodeType;       /* Type of node                 */
    Int             traversed;      /* Boolean: traversed yet?      */
    Int             start;          /* First instruction offset     */
    Int             length;         /* No. of instructions this BB  */
	Int				numHlIcodes;	/* No. of high-level icodes		*/
	flags32			flg;			/* BB flags						*/

    /* In edges and out edges */
    Int             numInEdges;     /* Number of in edges           */
    struct _BB    **inEdges;        /* Array of ptrs. to in edges   */

    Int     		numOutEdges;    /* Number of out edges          */
    TYPEADR_TYPE	*edges;          /* Array of ptrs. to out edges  */

    /* For interval construction */
    Int             beenOnH;        /* #times been on header list H */
    Int             inEdgeCount;    /* #inEdges (to find intervals) */
    struct _BB     *reachingInt;    /* Reaching interval header     */
    interval       *inInterval;     /* Node's interval              */

    /* For derived sequence construction */
    interval       *correspInt;     /* Corresponding interval in
                                     * derived graph Gi-1       	*/

	/* For live register analysis 
	 * LiveIn(b) = LiveUse(b) U (LiveOut(b) - Def(b))	*/
	dword			liveUse;		/* LiveUse(b)					*/
	dword			def;			/* Def(b)						*/
	dword			liveIn;			/* LiveIn(b)					*/
	dword			liveOut;		/* LiveOut(b)					*/

    /* For structuring analysis */
    Int             dfsFirstNum;    /* DFS #: first visit of node   */
    Int             dfsLastNum;     /* DFS #: last visit of node    */
    Int             immedDom;       /* Immediate dominator (dfsLast
                                     * index)                       */
    Int             ifFollow;       /* node that ends the if        */
    Int             loopType;       /* Type of loop (if any)        */
    Int             latchNode;      /* latching node of the loop    */
	Int				numBackEdges;	/* # of back edges				*/
    Int             loopHead;       /* most nested loop head to which
                                     * this node belongs (dfsLast)  */
    Int             loopFollow;     /* node that follows the loop   */
    Int             caseHead;       /* most nested case to which this
                                        node belongs (dfsLast)      */
    Int             caseTail;       /* tail node for the case       */

    Int             index;          /* Index, used in several ways  */
    struct _BB     *next;           /* Next (list link)             */
} BB;
typedef BB *PBB;


/* Derived Sequence structure */
typedef struct _derivedNode {
    BB                  *Gi;        /* Graph pointer        */
    interval            *Ii;        /* Interval list of Gi  */
    struct _derivedNode *next;      /* Next derived graph   */
} derSeq;

