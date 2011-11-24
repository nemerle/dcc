#pragma once
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
struct Function
{
    dword        procEntry; /* label number                         	 */
    char         name[SYMLEN]; /* Meaningful name for this proc     	 */
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

        /* Double-linked list */
//    Function *next;
//    Function *prev;
public:
    Function() : procEntry(0),depth(0),flg(0),cbParam(0),cfg(0),dfsLast(0),numBBs(0),
        hasCase(false),liveIn(0),liveOut(0),liveAnal(0)//,next(0),prev(0)
    {
        memset(name,0,SYMLEN);
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
protected:
    void findExps();
    void genDU1();
    void elimCondCodes();
    void liveRegAnalysis(dword in_liveOut);
    void findIdioms();
    void propLong();
    void genLiveKtes();
};
