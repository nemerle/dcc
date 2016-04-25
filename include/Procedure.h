#pragma once
#include <llvm/ADT/ilist.h>
//#include <llvm/ADT/ilist_node.h>
#include <bitset>
#include <map>
#include "BasicBlock.h"
#include "locident.h"
#include "state.h"
#include "icode.h"
#include "StackFrame.h"
#include "CallConvention.h"
/* PROCEDURE NODE */
struct CALL_GRAPH;
struct Expr;
struct Disassembler;
struct Function;
struct CALL_GRAPH;
struct PROG;

struct Function;

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
/* Procedure FLAGS */
enum PROC_FLAGS
{
    PROC_BADINST=0x00000100,/* Proc contains invalid or 386 instruction */
    PROC_IJMP   =0x00000200,/* Proc incomplete due to indirect jmp	 	*/
    PROC_ICALL  =0x00000400, /* Proc incomplete due to indirect call		*/
    PROC_HLL    =0x00001000, /* Proc is likely to be from a HLL			*/
//    CALL_PASCAL =0x00002000, /* Proc uses Pascal calling convention		*/
//    CALL_C      =0x00004000, /* Proc uses C calling convention			*/
//    CALL_UNKNOWN=0x00008000, /* Proc uses unknown calling convention		*/
    PROC_NEAR   =0x00010000, /* Proc exits with near return				*/
    PROC_FAR    =0x00020000, /* Proc exits with far return				*/
    GRAPH_IRRED =0x00100000, /* Proc generates an irreducible graph		*/
    SI_REGVAR   =0x00200000, /* SI is used as a stack variable 			*/
    DI_REGVAR   =0x00400000, /* DI is used as a stack variable 			*/
    PROC_IS_FUNC=0x00800000,	/* Proc is a function 						*/
    REG_ARGS    =0x01000000, /* Proc has registers as arguments			*/
//    PROC_VARARG =0x02000000,	/* Proc has variable arguments				*/
    PROC_OUTPUT =0x04000000, /* C for this proc has been output 			*/
    PROC_RUNTIME=0x08000000, /* Proc is part of the runtime support		*/
    PROC_ISLIB  =0x10000000, /* Proc is a library function				*/
    PROC_ASM    =0x20000000, /* Proc is an intrinsic assembler routine   */
    PROC_IS_HLL =0x40000000 /* Proc has HLL prolog code					*/
//#define CALL_MASK    0xFFFF9FFF /* Masks off CALL_C and CALL_PASCAL		 	*/
};

struct FunctionType
{
    bool m_vararg=false;
    bool isVarArg() const {return m_vararg;}
};
struct Assignment
{
    Expr *lhs;
    Expr *rhs;
};
struct JumpTable
{
    uint32_t start;
    uint32_t finish;
    bool valid() {return start<finish;}
    size_t size() { return (finish-start)/2;}
    size_t entrySize() { return 2;}
    void pruneEntries(uint16_t cs);
};
class FunctionCfg
{
    std::list<BB*> m_listBB;      /* Ptr. to BB list/CFG                  	 */
public:
    typedef std::list<BB*>::iterator iterator;
    iterator	begin() {
        return m_listBB.begin();
    }
    iterator	end()	 {
        return m_listBB.end();
    }
    BB * &front() { return m_listBB.front();}
    void nodeSplitting()
    {
        /* Converts the irreducible graph G into an equivalent reducible one, by
         * means of node splitting.  */
        fprintf(stderr,"Attempt to perform node splitting: NOT IMPLEMENTED\n");
    }
    void push_back(BB *v) { m_listBB.push_back(v);}
};
struct Function : public llvm::ilist_node<Function>
{
    typedef llvm::iplist<BB> BasicBlockListType;
    // BasicBlock iterators...
    typedef BasicBlockListType::iterator iterator;
    typedef BasicBlockListType::const_iterator const_iterator;
protected:
    BasicBlockListType  BasicBlocks;        ///< The basic blocks
    Function(FunctionType */*ty*/) : procEntry(0),depth(0),flg(0),cbParam(0),m_dfsLast(0),numBBs(0),
        hasCase(false),liveAnal(0)
    {
        type = new FunctionType;
        callingConv(CConv::UNKNOWN);
    }

public:
    FunctionType *  type;
    CConv *         m_call_conv;
    uint32_t        procEntry; /* label number                         	 */
    std::string  name;      /* Meaningful name for this proc     	 */
    STATE        state;     /* Entry state                          	 */
    int          depth;     /* Depth at which we found it - for printing */
    uint32_t     flg;       /* Combination of Icode & Proc flags    	 */
    int16_t      cbParam;   /* Probable no. of bytes of parameters  	 */
    STKFRAME     args;      /* Array of arguments                   	 */
    LOCAL_ID	 localId;   /* Local identifiers                         */
    ID           retVal;    /* Return value - identifier    		 */

        /* Icodes and control flow graph */
    CIcodeRec	 Icode;     /* Object with ICODE records                 */
    FunctionCfg     m_actual_cfg;
    std::vector<BB*> m_dfsLast;
    std::map<int,BB*> m_ip_to_bb;
//                           * (reverse postorder) order            	 */
    size_t        numBBs;    /* Number of BBs in the graph cfg       	 */
    bool         hasCase;   /* Procedure has a case node            	 */

    /* For interprocedural live analysis */
    LivenessSet     liveIn;	/* Registers used before defined                 */
    LivenessSet     liveOut;	/* Registers that may be used in successors	 */
    bool            liveAnal;	/* Procedure has been analysed already		 */

    virtual ~Function() {
        delete type;
    }
public:
    static Function *Create(FunctionType *ty=0,int /*Linkage*/=0,const std::string &nm="",void */*module*/=0)
    {
        Function *r=new Function(ty);
        r->name = nm;
        return r;
    }
    FunctionType *getFunctionType() const {
        return type;
    }
    CConv *callingConv() const { return m_call_conv;}
    void callingConv(CConv::Type v);

//    bool anyFlagsSet(uint32_t t) { return (flg&t)!=0;}
    bool hasRegArgs() const { return (flg & REG_ARGS)!=0;}
    bool isLibrary() const { return (flg & PROC_ISLIB)!=0;}
    void compoundCond();
    void writeProcComments();
    void lowLevelAnalysis();
    void bindIcodeOff();
    void dataFlow(LivenessSet &liveOut);
    void compressCFG();
    void highLevelGen();
    void structure(derSeq *derivedG);
    derSeq *checkReducibility();
    void createCFG();
    void markImpure();
    void findImmedDom();
    void FollowCtrl(CALL_GRAPH *pcallGraph, STATE *pstate);
    void process_operands(ICODE &pIcode, STATE *pstate);
    bool process_JMP(ICODE &pIcode, STATE *pstate, CALL_GRAPH *pcallGraph);
    bool process_CALL(ICODE &pIcode, CALL_GRAPH *pcallGraph, STATE *pstate);
    void freeCFG();
    void codeGen(std::ostream &fs);
    void mergeFallThrough(BB *pBB);
    void structIfs();
    void structLoops(derSeq *derivedG);
    void buildCFG(Disassembler &ds);
    void controlFlowAnalysis();
    void newRegArg(iICODE picode, iICODE ticode);
    void writeProcComments(std::ostream &ostr);

    void displayCFG();
    void displayStats();
    void processHliCall(Expr *exp, iICODE picode);

    void preprocessReturnDU(LivenessSet &_liveOut);
    Expr * adjustActArgType(Expr *_exp, hlType forType);
    std::string writeCall(Function *tproc, STKFRAME &args, int *numLoc);
    void processDosInt(STATE *pstate, PROG &prog, bool done);
    ICODE *translate_DIV(LLInst *ll, ICODE &_Icode);
    ICODE *translate_XCHG(LLInst *ll, ICODE &_Icode);
protected:
    void extractJumpTableRange(ICODE& pIcode, STATE *pstate, JumpTable &table);
    bool followAllTableEntries(JumpTable &table, uint32_t cs, ICODE &pIcode, CALL_GRAPH *pcallGraph, STATE *pstate);
    bool removeInEdge_Flag_and_ProcessLatch(BB *pbb, BB *a, BB *b);
    bool Case_X_and_Y(BB* pbb, BB* thenBB, BB* elseBB);
    bool Case_X_or_Y(BB* pbb, BB* thenBB, BB* elseBB);
    bool Case_notX_or_Y(BB* pbb, BB* thenBB, BB* elseBB);
    bool Case_notX_and_Y(BB* pbb, BB* thenBB, BB* elseBB);
    void replaceInEdge(BB* where, BB* which, BB* with);
    void processExpPush(int &numHlIcodes, iICODE picode);

    // TODO: replace those with friend visitor ?
    void propLongReg(int loc_ident_idx, const ID &pLocId);
    void propLongStk(int i, const ID &pLocId);
    void propLongGlb(int i, const ID &pLocId);
    void processTargetIcode(iICODE picode, int &numHlIcodes, iICODE ticode, bool isLong);

    int     findBackwarLongDefs(int loc_ident_idx, const ID &pLocId, iICODE iter);
    int     findForwardLongUses(int loc_ident_idx, const ID &pLocId, iICODE beg);
    void    structCases();
    void    findExps();
    void    genDU1();
    void    elimCondCodes();
    void    liveRegAnalysis(LivenessSet &in_liveOut);
    void    findIdioms();
    void    propLong();
    void    genLiveKtes();
    bool    findDerivedSeq(derSeq &derivedGi);
    bool    nextOrderGraph(derSeq &derivedGi);
    void addOutEdgesForConditionalJump(BB*        pBB, int next_ip, LLInst *ll);
    
private:
    bool    decodeIndirectJMP(ICODE &pIcode, STATE *pstate, CALL_GRAPH *pcallGraph);
    bool    decodeIndirectJMP2(ICODE &pIcode, STATE *pstate, CALL_GRAPH *pcallGraph);
};
namespace llvm {
template<> struct ilist_traits<typename ::Function>
  : public ilist_default_traits<typename ::Function> {

  // createSentinel is used to get hold of the node that marks the end of the
  // list... (same trick used here as in ilist_traits<Instruction>)
  typename ::Function *createSentinel() const {
    return static_cast<typename ::Function*>(&Sentinel);
  }
  static void destroySentinel(typename ::Function*) {}

  typename ::Function *provideInitialHead() const { return createSentinel(); }
  typename ::Function *ensureHead(::Function*) const { return createSentinel(); }
  static void noteHead(typename ::Function*, typename ::Function*) {}

private:
  mutable ilist_node<typename ::Function> Sentinel;
};
}
typedef llvm::iplist<Function> FunctionListType;
typedef FunctionListType lFunction;
typedef lFunction::iterator ilFunction;
