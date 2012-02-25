#pragma once
#include <llvm/ADT/ilist.h>
#include <llvm/ADT/ilist_node.h>
#include "BasicBlock.h"
#include "types.h"
#include "ast.h"
#include "icode.h"
#include "locident.h"
#include "error.h"
#include "graph.h"
#include "bundle.h"
#include "StackFrame.h"
/* PROCEDURE NODE */
struct CALL_GRAPH;
namespace llvm
{
// Traits for intrusive list of basic blocks...
template<>
struct ilist_traits<BB> : public ilist_default_traits<BB>
{

    // createSentinel is used to get hold of the node that marks the end of the
    // list... (same trick used here as in ilist_traits<Instruction>)
    BB *createSentinel() const {
        return static_cast<BB*>(&Sentinel);
    }
    static void destroySentinel(BB*) {}

    BB *provideInitialHead() const { return createSentinel(); }
    BB *ensureHead(BB*) const { return createSentinel(); }
    static void noteHead(BB*, BB*) {}

    //static ValueSymbolTable *getSymTab(Function *ItemParent);
private:
    mutable ilist_half_node<BB> Sentinel;
};
}
struct FunctionType
{
    bool m_vararg;
    bool isVarArg() const {return m_vararg;}
};
struct Assignment
{
    COND_EXPR *lhs;
    COND_EXPR *rhs;
};

struct Function : public llvm::ilist_node<Function>
{
    typedef llvm::iplist<BB> BasicBlockListType;
    // BasicBlock iterators...
    typedef BasicBlockListType::iterator iterator;
    typedef BasicBlockListType::const_iterator const_iterator;
private:
    BasicBlockListType  BasicBlocks;        ///< The basic blocks

public:
    dword        procEntry; /* label number                         	 */
    std::string  name;      /* Meaningful name for this proc     	 */
    STATE        state;     /* Entry state                          	 */
    Int          depth;     /* Depth at which we found it - for printing */
    flags32      flg;       /* Combination of Icode & Proc flags    	 */
    int16        cbParam;   /* Probable no. of bytes of parameters  	 */
    STKFRAME     args;      /* Array of arguments                   	 */
    LOCAL_ID	 localId;   /* Local identifiers                         */
    ID           retVal;    /* Return value - identifier    		 */

        /* Icodes and control flow graph */
    CIcodeRec	 Icode;     /* Object with ICODE records                 */
    std::vector<BB*> cfg;      /* Ptr. to BB list/CFG                  	 */
    std::vector<BB*> dfsLast;
    std::vector<BB*> heldBBs;
    //BB *         *dfsLast;  /* Array of pointers to BBs in dfsLast
//                           * (reverse postorder) order            	 */
    Int          numBBs;    /* Number of BBs in the graph cfg       	 */
    boolT        hasCase;   /* Procedure has a case node            	 */

    /* For interprocedural live analysis */
    dword		 liveIn;	/* Registers used before defined                 */
    dword		 liveOut;	/* Registers that may be used in successors	 */
    boolT		 liveAnal;	/* Procedure has been analysed already		 */

    Function(void *ty=0) : procEntry(0),depth(0),flg(0),cbParam(0),cfg(0),dfsLast(0),numBBs(0),
        hasCase(false),liveIn(0),liveOut(0),liveAnal(0)//,next(0),prev(0)
    {
    }
public:
    static Function *Create(void *ty=0,int Linkage=0,const std::string &nm="",void *module=0)
    {
        Function *r=new Function(ty);
        r->name = nm;
        return r;
    }
    void compoundCond();
    void writeProcComments();
    void lowLevelAnalysis();
    void bindIcodeOff();
    void dataFlow(dword liveOut);
    void compressCFG();
    void highLevelGen();
    void structure(derSeq *derivedG);
    derSeq *checkReducibility();
    void createCFG();
    void markImpure();
    void findImmedDom();
    void FollowCtrl(CALL_GRAPH *pcallGraph, STATE *pstate);
    void process_operands(ICODE *pIcode, STATE *pstate);
    boolT process_JMP(ICODE *pIcode, STATE *pstate, CALL_GRAPH *pcallGraph);
    boolT process_CALL(ICODE *pIcode, CALL_GRAPH *pcallGraph, STATE *pstate);
    void displayCFG();
    void freeCFG();
    void codeGen(std::ostream &fs);
    void displayStats();
    void mergeFallThrough(BB *pBB);
    void structIfs();
    void structLoops(derSeq *derivedG);
    void buildCFG();
    void controlFlowAnalysis();
    void newRegArg(ICODE *picode, ICODE *ticode);
protected:
    // TODO: replace those with friend visitor ?
    void propLongReg(Int loc_ident_idx, ID *pLocId);
    void propLongStk(Int i, ID *pLocId);
    void propLongGlb(Int i, ID *pLocId);

    int     checkBackwarLongDefs(int loc_ident_idx, ID *pLocId, int pLocId_idx, Assignment &assign);
    void    structCases();
    void    findExps();
    void    genDU1();
    void    elimCondCodes();
    void    liveRegAnalysis(dword in_liveOut);
    void    findIdioms();
    void    propLong();
    void    genLiveKtes();
    byte    findDerivedSeq (derSeq *derivedGi);
    bool    nextOrderGraph(derSeq *derivedGi);
};
