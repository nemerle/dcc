/****************************************************************************
 *          dcc project procedure list builder
 * (C) Cristina Cifuentes, Mike van Emmerik, Jeff Ledermann
 ****************************************************************************/
#include "parser.h"

#include "dcc.h"
#include "project.h"
#include "CallGraph.h"
#include "msvc_fixes.h"
#include "chklib.h"
#include "FollowControlFlow.h"

#include <inttypes.h>
#include <string.h>
#include <stdlib.h>     /* For exit() */
#include <sstream>
#include <stdio.h>
#include <algorithm>
#include <deque>
#include <QMap>
#include <QtCore/QDebug>

//TODO: The OS service resolution should be done iteratively:
// for every unprocessed INT instruction, that calls OS service that does not terminate execution
//  mark INT as non-termination instruction
//  follow execution flow from next instruction
//  recheck OS services

using namespace std;

//static void     FollowCtrl (Function * pProc, CALL_GRAPH * pcallGraph, STATE * pstate);
static void     setBits(int16_t type, uint32_t start, uint32_t len);
static void     process_MOV(LLInst &ll, STATE * pstate);
static bool     process_JMP(const PtrFunction &func,ICODE & pIcode, STATE *pstate, CALL_GRAPH * pcallGraph);
static bool     process_CALL(const PtrFunction &func,ICODE & pIcode, CALL_GRAPH * pcallGraph, STATE *pstate);
static SYM *    lookupAddr (LLOperand *pm, STATE * pstate, int size, uint16_t duFlag);
//void    interactDis(Function * initProc, int ic);

/* Returns the size of the string pointed by sym and delimited by delim.
 * Size includes delimiter.     */
int strSize (const uint8_t *sym, char delim)
{
    PROG &prog(Project::get()->prog);
    int till_end = sym-prog.image();
    const uint8_t *end_ptr=std::find(sym,sym+(prog.cbImage-(till_end)),delim);
    return end_ptr-sym+1;
}

static std::vector<ICODE> rewrite_DIV(LLInst *ll, ICODE &_Icode)
{
    ICODE synth_mov = ICODE(); // MOV rTMP, reg
    ICODE synth_mod = ICODE(); // MOD

    synth_mov.type = LOW_LEVEL_ICODE;
    synth_mov.ll()->set(iMOV,0,rTMP);
    if (ll->testFlags(B) )
    {
        synth_mov.ll()->setFlags( B );
        synth_mov.ll()->replaceSrc(rAX);
    }
    else    /* implicit dx:ax */
    {
        synth_mov.ll()->setFlags( IM_SRC );
        synth_mov.setRegDU( rDX, eUSE);
    }
    synth_mov.setRegDU( rAX, eUSE);
    synth_mov.setRegDU( rTMP, eDEF);
    synth_mov.ll()->setFlags( SYNTHETIC );
    /* eIcode.ll()->label = Project::get()->SynthLab++; */
    synth_mov.ll()->label = _Icode.ll()->label;

    /* iMOD */
    synth_mod.type = LOW_LEVEL_ICODE;
    synth_mod.ll()->set(iMOD,ll->getFlag() | SYNTHETIC  | IM_TMP_DST);
    synth_mod.ll()->replaceSrc(_Icode.ll()->src());
    synth_mod.du = _Icode.du;
    synth_mod.ll()->label = Project::get()->SynthLab++;
    return {
        synth_mov,
        _Icode,
        synth_mod
    };
}
static std::vector<ICODE> rewrite_XCHG(LLInst *ll,ICODE &_Icode)
{
    /* MOV rTMP, regDst */
    ICODE mov_tmp_dst;
    mov_tmp_dst.type = LOW_LEVEL_ICODE;
    mov_tmp_dst.ll()->set(iMOV,SYNTHETIC,rTMP,ll->m_dst);
    mov_tmp_dst.setRegDU( rTMP, eDEF);
    if(mov_tmp_dst.ll()->src().getReg2())
    {
        eReg srcreg=mov_tmp_dst.ll()->src().getReg2();
        mov_tmp_dst.setRegDU( srcreg, eUSE);
        if((srcreg>=rAL) and (srcreg<=rBH))
            mov_tmp_dst.ll()->setFlags( B );
    }
    mov_tmp_dst.ll()->label = ll->label;
    /* MOV regDst, regSrc */
    ICODE mov_dst_src  = _Icode; // copy all XCHG things, but set opcode to iMOV and mark it as synthetic insn
    mov_dst_src.ll()->set(iMOV,SYNTHETIC|ll->getFlag());

    /* MOV regSrc, rTMP */

    ICODE mov_src_tmp;
    mov_src_tmp.type = LOW_LEVEL_ICODE;
    mov_src_tmp.ll()->set(iMOV,SYNTHETIC);
    mov_src_tmp.ll()->replaceDst(ll->src());
    if(mov_src_tmp.ll()->m_dst.regi)
    {
        if((mov_src_tmp.ll()->m_dst.regi>=rAL) and (mov_src_tmp.ll()->m_dst.regi<=rBH))
            mov_src_tmp.ll()->setFlags( B );
        mov_src_tmp.setRegDU( mov_src_tmp.ll()->m_dst.regi, eDEF);
    }
    mov_src_tmp.ll()->replaceSrc(rTMP);
    mov_src_tmp.setRegDU( rTMP, eUSE);
    mov_src_tmp.ll()->label = Project::get()->SynthLab++;
    return {
        mov_tmp_dst,
        mov_dst_src,
        mov_src_tmp
    };
}
/**
 * @brief resolveOSServices
 * @param ll
 * @param pstate
 * @return true if given OS service will terminate the program
 */
//TODO: convert into Command class
static bool resolveOSServices(LLInst *ll, STATE *pstate)
{
    PROG &prog(Project::get()->prog);
    SYMTAB &global_symbol_table(Project::get()->symtab);

    bool terminates=false;
    if(pstate->isKnown(rAX))
        assert(pstate->isKnown(rAH));
    if (ll->src().getImm2() == 0x21 and pstate->isKnown(rAH))
    {
        int funcNum = pstate->r[rAH];
        int operand;
        int size;

        /* Save function number */
        ll->m_dst.off = (int16_t)funcNum;
        //Icode.GetIcode(Icode.GetNumIcodes() - 1)->

        /* Program termination: int21h, fn 00h, 31h, 4Ch */
        terminates  = (bool)(funcNum == 0x00 or funcNum == 0x31 or funcNum == 0x4C);

        /* String functions: int21h, fn 09h */
        /* offset goes into DX */
        if (pstate->isKnown(rDX))
            if (funcNum == 0x09)
            {
                operand = ((uint32_t)(uint16_t)pstate->r[rDS]<<4) + (uint32_t)(uint16_t)pstate->r[rDX];
                size = prog.fCOM ?
                           strSize (&prog.image()[operand], '$') :
                           strSize (&prog.image()[operand], '$'); // + 0x100
                global_symbol_table.updateSymType (operand, TypeContainer(TYPE_STR, size));
            }
    }
    else if ((ll->src().getImm2() == 0x2F) and pstate->isKnown(rAH))
    {
        ll->m_dst.off = pstate->r[rAH];
    }
    else    /* Program termination: int20h, int27h */
        terminates = (ll->src().getImm2() == 0x20 or ll->src().getImm2() == 0x27);

    return terminates;
}

/** FollowCtrl - Given an initial procedure, state information and symbol table
 * builds a list of procedures reachable from the initial procedure
 * using a depth first search.     */
void FollowCtrl(Function &func,CALL_GRAPH * pcallGraph, STATE *pstate)
{
    Project *project(Project::get());
    PROG &prog(Project::get()->prog);
    ICODE   _Icode, *pIcode;     /* This gets copied to pProc->Icode[] later */
    SYM *    psym;
    uint32_t   offset;
    eErrorId err;
    bool   done = false;

    if (func.name.contains("chkstk"))
    {
        // Danger! Dcc will likely fall over in this code.
        // So we act as though we have done with this proc
        // pProc->flg &= ~TERMINATES; // Not sure about this
        done = true;
        // And mark it as a library function, so structure() won't choke on it
        func.flg |= PROC_ISLIB;
        return;
    }
    if (option.VeryVerbose)
    {
        qDebug() << "Parsing proc" << func.name << "at"<< QString::number(pstate->IP,16).toUpper();
    }

    while (not done )
    {
        err = scan(pstate->IP, _Icode);
        if(err)
            break;
        LLInst *ll = _Icode.ll();
        pstate->IP += (uint32_t)ll->numBytes;
        setBits(BM_CODE, ll->label, (uint32_t)ll->numBytes);

        func.process_operands(_Icode,pstate);

        /* Keep track of interesting instruction flags in procedure */
        func.flg |= (ll->getFlag() & (NOT_HLL | FLOAT_OP));

        /* Check if this instruction has already been parsed */
        iICODE labLoc = func.Icode.labelSrch(ll->label);
        if (func.Icode.end()!=labLoc)
        {   /* Synthetic jump */
            _Icode.type = LOW_LEVEL_ICODE;
            ll->set(iJMP,I | SYNTHETIC | NO_OPS);
            ll->replaceSrc(LLOperand::CreateImm2(labLoc->ll()->GetLlLabel()));
            ll->label = Project::get()->SynthLab++;
        }

        /* Copy Icode to Proc */
        if ((ll->getOpcode() == iDIV) or (ll->getOpcode() == iIDIV)) {
            std::vector<ICODE> translated = rewrite_DIV(ll,_Icode);
            for(const ICODE &ic : translated)
                pIcode = func.Icode.addIcode(&ic);
        }
        else if (_Icode.ll()->getOpcode() == iXCHG) {
            std::vector<ICODE> translated = rewrite_XCHG(ll,_Icode);
            for(const ICODE &ic : translated)
                pIcode = func.Icode.addIcode(&ic);
        }
        else {
            pIcode = func.Icode.addIcode(&_Icode);
        }

        switch (ll->getOpcode()) {
            /*** Conditional jumps ***/
            case iLOOP: case iLOOPE:    case iLOOPNE:
            case iJB:   case iJBE:      case iJAE:  case iJA:
            case iJL:   case iJLE:      case iJGE:  case iJG:
            case iJE:   case iJNE:      case iJS:   case iJNS:
            case iJO:   case iJNO:      case iJP:   case iJNP:
            case iJCXZ:
            {
                STATE   StCopy;
                int     ip      = func.Icode.size()-1;   /* Index of this jump */
                ICODE  &prev(*(++func.Icode.rbegin()));  /* Previous icode */
                bool   fBranch = false;

                pstate->JCond.regi = 0;

                /* This sets up range check for indexed JMPs hopefully
                 * Handles JA/JAE for fall through and JB/JBE on branch
                 */
                if (ip > 0 and prev.ll()->match(iCMP,I) )
                {
                    pstate->JCond.immed = (int16_t)prev.ll()->src().getImm2();
                    if (ll->match(iJA) or ll->match(iJBE) )
                        pstate->JCond.immed++;
                    if (ll->getOpcode() == iJAE or ll->getOpcode() == iJA)
                        pstate->JCond.regi = prev.ll()->m_dst.regi;
                    fBranch = (ll->getOpcode() == iJB or ll->getOpcode() == iJBE);
                }
                StCopy = *pstate;

                /* Straight line code */
                project->addCommand(func.shared_from_this(),new FollowControlFlow(StCopy)); // recurrent ?

                if (fBranch)                /* Do branching code */
                {
                    pstate->JCond.regi = prev.ll()->m_dst.regi;
                }
                /* Next icode. Note: not the same as GetLastIcode() because of the call to FollowCtrl() */
                pIcode = func.Icode.GetIcode(ip);
                /* do the jump path */
                done = process_JMP(func.shared_from_this(),*pIcode, pstate, pcallGraph);
                break;
            }
                /*** Jumps ***/
            case iJMP:
            case iJMPF: /* Returns true if we've run into a loop */
                done = process_JMP (func.shared_from_this(),*pIcode, pstate, pcallGraph);
                break;

                /*** Calls ***/
            case iCALL:
            case iCALLF:
                done = process_CALL(func.shared_from_this(),*pIcode, pcallGraph, pstate);
                pstate->kill(rBX);
                pstate->kill(rCX);
                break;

                /*** Returns ***/
            case iRET:
            case iRETF:
                func.flg |= (ll->getOpcode() == iRET)? PROC_NEAR:PROC_FAR;
                func.flg &= ~TERMINATES;
                done = true;
                break;
            case iIRET:
                func.flg &= ~TERMINATES;
                done = true;
                break;

            case iINT:
                done = resolveOSServices(ll, pstate);
                if (done)
                    pIcode->ll()->setFlags(TERMINATES);
                break;

            case iMOV:
                process_MOV(*pIcode->ll(), pstate);
                break;
            case iSHL:
                if (pstate->JCond.regi == ll->m_dst.regi)
                {
                    if (ll->srcIsImmed() and ll->src().getImm2() == 1)
                        pstate->JCond.immed *= 2;
                    else
                        pstate->JCond.regi = 0;
                }
                break;

            case iLEA:
                if (ll->src().getReg2()== rUNDEF)      /* direct mem offset */
                    pstate->setState( ll->m_dst.getReg2(), ll->src().off);
                break;

            case iLDS: case iLES:
                if ((psym = lookupAddr(&ll->src(), pstate, 4, eDuVal::USE))
                        /* and (Icode.ll()->flg & SEG_IMMED) */ )
                {
                    offset = LH(&prog.image()[psym->label]);
                    pstate->setState( (ll->getOpcode() == iLDS)? rDS : rES,
                                      LH(&prog.image()[psym->label + 2]));
                    pstate->setState( ll->m_dst.regi, (int16_t)offset);
                    psym->type = TYPE_PTR;
                }
                break;
        }
    }

    if (err) {
        func.flg &= ~TERMINATES;

        if (err == INVALID_386OP or err == INVALID_OPCODE)
        {
            fatalError(err, prog.image()[_Icode.ll()->label], _Icode.ll()->label);
            func.flg |= PROC_BADINST;
        }
        else if (err == IP_OUT_OF_RANGE)
            fatalError (err, _Icode.ll()->label);
        else
            reportError(err, _Icode.ll()->label);
    }
}

/* Firstly look for a leading range check of the form:-
 *      CMP {BX | SI | DI}, immed
 *      JA | JAE | JB | JBE
 * This is stored in the current state as if we had just
 * followed a JBE branch (i.e. [reg] lies between 0 - immed).
*/
void Function::extractJumpTableRange(ICODE& pIcode, STATE *pstate, JumpTable &table)
{
    static uint8_t i2r[4] = {rSI, rDI, rBP, rBX};
    if (pstate->JCond.regi == i2r[pIcode.ll()->src().getReg2()-INDEX_SI])
        table.finish = table.start + pstate->JCond.immed;
    else
        table.finish = table.start + 2;
}

static bool decodeIndirectJMP(const PtrFunction &func,ICODE & pIcode, STATE *pstate)
{
    PROG &prog(Project::get()->prog);
//    mov cx,NUM_CASES
//    mov bx,JUMP_TABLE

//  LAB1:
//    mov ax, [bx]
//    cmp ax,VAL
//    jz LAB2
//    add bx,2
//    loop LAB1
//    jmp DEFAULT_CASE
//  LAB2
//    jmp word ptr [bx+2*NUM_CASES]
    static const llIcode match_seq[] = {iMOV,iMOV,iMOV,iCMP,iJE,iADD,iLOOP,iJMP,iJMP};

    if(func->Icode.size()<8)
        return false;
    if(&func->Icode.back()!=&pIcode) // not the last insn in the list skip it
        return false;
    if(pIcode.ll()->src().regi != INDEX_BX) {
        return false;
    }
    // find address-wise predecessors of the icode
    std::deque<ICODE *> matched;
    QMap<uint32_t,ICODE *> addrmap;
    for(ICODE & ic : func->Icode) {
        addrmap[ic.ll()->GetLlLabel()] = &ic;
    }
    auto iter = addrmap.find(pIcode.ll()->GetLlLabel());
    while(matched.size()<9) {
        matched.push_front(*iter);
        --iter;
        if(iter==addrmap.end())
            return false;
    }
    // pattern starts at the last jmp
    ICODE *load_num_cases = matched[0];
    ICODE *load_jump_table_addr = matched[1];
//    ICODE *read_case_entry_insn = matched[2];
//    ICODE *cmp_case_val_insn = matched[3];
//    ICODE *exit_loop_insn = matched[4];
//    ICODE *add_bx_insn = matched[5];
//    ICODE *loop_insn = matched[6];
    ICODE *default_jmp = matched[7];
    ICODE *last_jmp = matched[8];
    for(int i=0; i<8; ++i) {
        if(matched[i+1]->ll()->GetLlLabel()!=matched[i]->ll()->GetLlLabel()+matched[i]->ll()->numBytes) {
            qDebug() << "Matching jump pattern impossible - undecoded instructions in pattern area ";
            return false;
        }
    }
    for(int i=0; i<9; ++i) {
        if(matched[i]->ll()->getOpcode()!=match_seq[i]) {
            return false;
        }
    }
    // verify that bx+offset == 2* case count
    uint32_t num_cases = load_num_cases->ll()->src().getImm2();
    if(last_jmp->ll()->src().off != 2*num_cases)
        return false;
    const LLOperand &op = last_jmp->ll()->src();
    if(op.regi != INDEX_BX)
        return false;
    if(not load_jump_table_addr->ll()->src().isImmediate())
        return false;
    uint32_t cs = (uint32_t)(uint16_t)pstate->r[rCS] << 4;
    uint32_t table_addr = cs + load_jump_table_addr->ll()->src().getImm2();
    uint32_t default_label = cs + default_jmp->ll()->src().getImm2();
    setBits(BM_DATA, table_addr, num_cases*2 + num_cases*2); // num_cases of short values + num cases short ptrs
    pIcode.ll()->setFlags(SWITCH);

    for(int i=0; i<num_cases; ++i) {
        STATE   StCopy = *pstate;
        uint32_t jump_target_location = table_addr + num_cases*2 + i*2;
        StCopy.IP = cs + *(uint16_t *)(prog.image()+jump_target_location);
        Project::get()->addCommand(func,new FollowControlFlow(StCopy));
        Project::get()->addCommand(func,new MarkAsSwitchCase(pIcode.ll()->label,StCopy.IP,i));
    }
    return true;
}
static bool decodeIndirectJMP2(const PtrFunction &func,ICODE & pIcode, STATE *pstate)
{
    PROG &prog(Project::get()->prog);
//    mov cx,NUM_CASES
//    mov bx,JUMP_TABLE

//  LAB1:
//    mov ax, [bx]
//    cmp ax, LOW_WORD_OF_VAL
//    jnz LAB2
//    mov ax, [bx + 2 * NUM_CASES]
//    cmp ax, HIGH_WORD_OF_VAL
//    jz LAB3
//  LAB2
//    add bx,2
//    loop LAB1
//    jmp DEFAULT_CASE
//  LAB3
//    jmp word ptr [bx+2*NUM_CASES]
    static const llIcode match_seq[] = {iMOV,iMOV,iMOV,iCMP,iJNE,iMOV,iCMP,iJE,iADD,iLOOP,iJMP,iJMP};

    if(func->Icode.size()<12)
        return false;
    if(&func->Icode.back() != &pIcode) // not the last insn in the list skip it
        return false;
    if(pIcode.ll()->src().regi != INDEX_BX) {
        return false;
    }
    // find address-wise predecessors of the icode
    std::deque<ICODE *> matched;
    QMap<uint32_t,ICODE *> addrmap;
    for(ICODE & ic : func->Icode) {
        addrmap[ic.ll()->GetLlLabel()] = &ic;
    }
    auto iter = addrmap.find(pIcode.ll()->GetLlLabel());
    while(matched.size()<12) {
        matched.push_front(*iter);
        --iter;
        if(iter==addrmap.end())
            return false;
    }
    // pattern starts at the last jmp
    ICODE *load_num_cases = matched[0];
    ICODE *load_jump_table_addr = matched[1];
    ICODE *default_jmp = matched[10];
    ICODE *last_jmp = matched[11];

    for(int i=0; i<11; ++i) {
        if(matched[i+1]->ll()->GetLlLabel()!=matched[i]->ll()->GetLlLabel()+matched[i]->ll()->numBytes) {
            qDebug() << "Matching jump pattern impossible - undecoded instructions in pattern area ";
            return false;
        }
    }
    for(int i=0; i<12; ++i) {
        if(matched[i]->ll()->getOpcode()!=match_seq[i]) {
            return false;
        }
    }
    // verify that bx+offset == 2* case count
    uint32_t num_cases = load_num_cases->ll()->src().getImm2();
    if(last_jmp->ll()->src().off != 4*num_cases)
        return false;
    const LLOperand &op = last_jmp->ll()->src();
    if(op.regi != INDEX_BX)
        return false;
    if(not load_jump_table_addr->ll()->src().isImmediate())
        return false;
    uint32_t cs = (uint32_t)(uint16_t)pstate->r[rCS] << 4;
    uint32_t table_addr = cs + load_jump_table_addr->ll()->src().getImm2();
    int default_label = cs + default_jmp->ll()->src().getImm2();
    setBits(BM_DATA, table_addr, num_cases*4 + num_cases*2); // num_cases of long values + num cases short ptrs
    pIcode.ll()->setFlags(SWITCH);

    for(int i=0; i<num_cases; ++i) {
        STATE   StCopy = *pstate;
        uint32_t jump_target_location = table_addr + num_cases*4 + i*2;
        StCopy.IP = cs + *(uint16_t *)(prog.image()+jump_target_location);
        Project::get()->addCommand(func,new FollowControlFlow(StCopy));
        Project::get()->addCommand(func,new MarkAsSwitchCase(pIcode.ll()->label,StCopy.IP,i));
    }
    return true;
}

/* Look for switch() stmt. idiom of the form
 *   JMP  uint16_t ptr  word_offset[rBX | rSI | rDI]        */
static bool decodeIndirectJMP0(const PtrFunction &func,ICODE & pIcode, STATE *pstate) {
    const static uint8_t i2r[4] = {rSI, rDI, rBP, rBX};

    uint32_t seg = (pIcode.ll()->src().seg)? pIcode.ll()->src().seg: rDS;
    PROG &prog(Project::get()->prog);

    /* Ensure we have a uint16_t offset & valid seg */
    if (pIcode.ll()->match(iJMP) and (pIcode.ll()->testFlags(WORD_OFF)) and
            pstate->f[seg] and
            (pIcode.ll()->src().regi == INDEX_SI or
             pIcode.ll()->src().regi == INDEX_DI or /* Idx reg. BX, SI, DI */
             pIcode.ll()->src().regi == INDEX_BX))
    {

       uint32_t offTable = ((uint32_t)(uint16_t)pstate->r[seg] << 4) + pIcode.ll()->src().off;
       uint32_t endTable;
        uint32_t i;
        /* Firstly look for a leading range check of the form:-
         *      CMP {BX | SI | DI}, immed
         *      JA | JAE | JB | JBE
         * This is stored in the current state as if we had just
         * followed a JBE branch (i.e. [reg] lies between 0 - immed).
        */
        if (pstate->JCond.regi == i2r[pIcode.ll()->src().regi-(INDEX_BX_SI+4)])
            endTable = offTable + pstate->JCond.immed;
        else
            endTable = (uint32_t)prog.cbImage;

        /* Search for first uint8_t flagged after start of table */
        for (i = offTable; i <= endTable; i++)
            if (BITMAP(i, BM_CODE | BM_DATA))
                break;
        endTable = i & ~1;      /* Max. possible table size */

        /* Now do some heuristic pruning.  Look for ptrs. into the table
         * and for addresses that don't appear to point to valid code.
        */
        uint32_t cs = (uint32_t)(uint16_t)pstate->r[rCS] << 4;
        for (i = offTable; i < endTable; i += 2)
        {
            uint32_t target = cs + LH(&prog.image()[i]);
            if (target < endTable and target >= offTable)
                endTable = target;
            else if (target >= (uint32_t)prog.cbImage)
                endTable = i;
        }

        for (i = offTable; i < endTable; i += 2)
        {
            uint32_t target = cs + LH(&prog.image()[i]);
            /* Be wary of 00 00 as code - it's probably data */
            ICODE _Icode;
            if (not (prog.image()[target] or prog.image()[target+1]) or scan(target, _Icode)!=NO_ERR)
                endTable = i;
        }

        /* Now for each entry in the table take a copy of the current
         * state and recursively call FollowCtrl(). */
        if (offTable < endTable)
        {
            assert(((endTable - offTable) / 2)<512);
            STATE   StCopy;
            //int     ip;
            //uint32_t  *psw;

            setBits(BM_DATA, offTable, endTable - offTable);

            pIcode.ll()->setFlags(SWITCH);

            uint32_t k;
            for (i = offTable, k = 0; i < endTable; i += 2)
            {
                StCopy = *pstate;
                StCopy.IP = cs + LH(&prog.image()[i]);
                Project::get()->addCommand(func,new FollowControlFlow(StCopy));
                Project::get()->addCommand(func,new MarkAsSwitchCase(pIcode.ll()->label,StCopy.IP,k++));
            }
            return true;
        }
    }
    return false;
}
/* process_JMP - Handles JMPs, returns true if we should end recursion  */
static bool process_JMP(const PtrFunction &func,ICODE & pIcode, STATE *pstate, CALL_GRAPH * pcallGraph)
{
    PROG &prog(Project::get()->prog);

    if (pIcode.ll()->srcIsImmed())
    {
        if (pIcode.ll()->getOpcode() == iJMPF)
            pstate->setState( rCS, LH(prog.image() + pIcode.ll()->label + 3));
        pstate->IP = pIcode.ll()->src().getImm2();
        int64_t i = pIcode.ll()->src().getImm2();
        if (i < 0)
        {
            exit(1);
        }

        /* Return true if jump target is already parsed */
        return func->Icode.alreadyDecoded(i);
    }
    /* We've got an indirect JMP */
    if(decodeIndirectJMP0(func,pIcode,pstate)) {
        return true;
    }
    if(decodeIndirectJMP(func,pIcode,pstate)) {
        return true;
    }
    if(decodeIndirectJMP2(func,pIcode,pstate)) {
        return true;
    }

    /* Can't do anything with this jump */

    func->flg |= PROC_IJMP;
    func->flg &= ~TERMINATES;
    // TODO: consider adding a new user-interactive command ResolveControlFlowFailure ?
    interactDis(func, func->Icode.size()-1);
    return true;
}


/* Process procedure call.
 * Note: We assume that CALL's will return unless there is good evidence to
 *       the contrary - thus we return false unless all paths in the called
 *       procedure end in DOS exits.  This is reasonable since C procedures
 *       will always include the epilogue after the call anyway and it's to
 *       be assumed that if an assembler program contains a CALL that the
 *       programmer expected it to come back - otherwise surely a JMP would
 *       have been used.  */

bool process_CALL(const PtrFunction &func,ICODE & pIcode, CALL_GRAPH * pcallGraph, STATE *pstate)
{
    Project &project(*Project::get());
    PROG &prog(Project::get()->prog);
    ICODE &last_insn(func->Icode.back());
    uint32_t off;
    /* For Indirect Calls, find the function address */
    bool indirect = false;
    //pIcode.ll()->immed.proc.proc=fakeproc;
    if ( not pIcode.ll()->srcIsImmed() )
    {
        /* Not immediate, i.e. indirect call */

        if (pIcode.ll()->m_dst.regi and (not option.Calls))
        {
            /* We have not set the brave option to attempt to follow
                the execution path through register indirect calls.
                So we just exit this function, and ignore the call.
                We probably should not have parsed this deep, anyway. */
            return false;
        }

        /* Offset into program image is seg:off of read input */
        /* Note: this assumes that the pointer itself is at
                        es:0 where es:0 is the start of the image. This is
                        usually wrong! Consider also CALL [BP+0E] in which the
                        segment for the pointer is in SS! - Mike */
        if(pIcode.ll()->m_dst.isReg())
        {
            if( not  pstate->isKnown(pIcode.ll()->m_dst.regi)
                    or
                    not  pstate->isKnown(pIcode.ll()->m_dst.seg)
                    )
            {
                fprintf(stderr,"Indirect call with unknown register values\n");
                return false;
            }
            off = pstate->r[pIcode.ll()->m_dst.seg];
            off <<=4;
            off += pstate->r[pIcode.ll()->m_dst.regi];

        }
        else
        {
            off = (uint32_t)(uint16_t)pIcode.ll()->m_dst.off +
                  ((uint32_t)(uint16_t)pIcode.ll()->m_dst.segValue << 4);
        }

        /* Address of function is given by 4 (CALLF) or 2 (CALL) bytes at
         * previous offset into the program image */
        uint32_t tgtAddr=0;
        if (pIcode.ll()->getOpcode() == iCALLF)
            tgtAddr= LH(&prog.image()[off]) + ((uint32_t)(LH(&prog.image()[off+2])) << 4);
        else
            tgtAddr= LH(&prog.image()[off]) + ((uint32_t)(uint16_t)pstate->r[rCS] << 4);
        pIcode.ll()->replaceSrc(LLOperand::CreateImm2( tgtAddr ) );
        pIcode.ll()->setFlags(I);
        indirect = true;
    }

    /* Process CALL.  Function address is located in pIcode.ll()->immed.op */
    if (pIcode.ll()->srcIsImmed())
    {
        /* Search procedure list for one with appropriate entry point */
        PtrFunction iter = Project::get()->findByEntry(pIcode.ll()->src().getImm2());

        /* Create a new procedure node and save copy of the state */
        if ( iter == nullptr )
        {
            iter = Project::get()->createFunction(0,"",{0,pIcode.ll()->src().getImm2()});
            Function &x(*iter);
            if(project.m_pattern_locator)
                project.m_pattern_locator->LibCheck(x);

            if (x.flg & PROC_ISLIB)
            {
                /* A library function. No need to do any more to it */
                pcallGraph->insertCallGraph (func, iter);
                //iter = (++pProcList.rbegin()).base();
                last_insn.ll()->src().proc.proc = &x;
                return false;
            }

            if (indirect)
                x.flg |= PROC_ICALL;

            if (x.name.isEmpty())     /* Don't overwrite existing name */
            {
                x.name = QString("proc_%1_%2").arg(x.procEntry ,6,16,QChar('0')).arg(++prog.cProcs);
            }
            x.depth = x.depth + 1;
            x.flg |= TERMINATES;

            pstate->IP = pIcode.ll()->src().getImm2();
            if (pIcode.ll()->getOpcode() == iCALLF)
                pstate->setState( rCS, LH(prog.image() + pIcode.ll()->label + 3));
            x.state = *pstate;

            /* Insert new procedure in call graph */
            pcallGraph->insertCallGraph (func, iter);

            /* Process new procedure */
            Project::get()->addCommand(iter,new FollowControlFlow(*pstate));


        }
        else
            Project::get()->callGraph->insertCallGraph (func, iter);

        last_insn.ll()->src().proc.proc = &(*iter); // ^ target proc

        /* return ((p->flg & TERMINATES) != 0); */
    }
    return false;           // Cristina, please check!!
}


/* process_MOV - Handles state changes due to simple assignments    */
static void process_MOV(LLInst & ll, STATE * pstate)
{
    PROG &prog(Project::get()->prog);
    SYM *  psym, *psym2;        /* Pointer to symbol in global symbol table */
    uint8_t  dstReg = ll.m_dst.regi;
    uint8_t  srcReg = ll.src().regi;
    if (dstReg > 0 and dstReg < INDEX_BX_SI)
    {
        if (ll.srcIsImmed())
            pstate->setState( dstReg, (int16_t)ll.src().getImm2());
        else if (srcReg == 0)   /* direct memory offset */
        {
            psym = lookupAddr(&ll.src(), pstate, 2, eDuVal::USE);
            if (psym and ((psym->flg & SEG_IMMED) or psym->duVal.val))
                pstate->setState( dstReg, LH(&prog.image()[psym->label]));
        }
        else if (srcReg < INDEX_BX_SI and pstate->f[srcReg])  /* reg */
        {
            pstate->setState( dstReg, pstate->r[srcReg]);

            /* Follow moves of the possible index register */
            if (pstate->JCond.regi == srcReg)
                pstate->JCond.regi = dstReg;
        }
    }
    else if (dstReg == 0) {     /* direct memory offset */
        int size=2;
        if((ll.src().regi>=rAL)and(ll.src().regi<=rBH))
            size=1;
        psym = lookupAddr (&ll.m_dst, pstate, size, eDEF);
        if (psym and not (psym->duVal.val))      /* no initial value yet */
        {
            if (ll.testFlags(I))   /* immediate */
            {
                //prog.image()[psym->label] = (uint8_t)ll.src().getImm2();
                pstate->setMemoryByte(psym->label,(uint8_t)ll.src().getImm2());
                if(psym->size>1)
                {
                    pstate->setMemoryByte(psym->label+1,uint8_t(ll.src().getImm2()>>8));
                    //prog.image()[psym->label+1] = (uint8_t)(ll.src().getImm2()>>8);
                }
                psym->duVal.val = 1;
            }
            else if (srcReg == 0) /* direct mem offset */
            {
                psym2 = lookupAddr (&ll.src(), pstate, 2, eDuVal::USE);
                if (psym2 and ((psym->flg & SEG_IMMED) or (psym->duVal.val)))
                {
                    //prog.image()[psym->label] = (uint8_t)prog.image()[psym2->label];
                    pstate->setMemoryByte(psym->label,(uint8_t)prog.image()[psym2->label]);
                    if(psym->size>1)
                    {
                        pstate->setMemoryByte(psym->label+1,(uint8_t)prog.image()[psym2->label+1]);
                        //prog.image()[psym->label+1] = prog.image()[psym2->label+1];//(uint8_t)(prog.image()[psym2->label+1] >> 8);
                    }
                    psym->duVal.setFlags(eDuVal::DEF);
                    psym2->duVal.setFlags(eDuVal::USE);
                }
            }
            else if (srcReg < INDEX_BX_SI and pstate->f[srcReg])  /* reg */
            {
                //prog.image()[psym->label] = (uint8_t)pstate->r[srcReg];
                pstate->setMemoryByte(psym->label,(uint8_t)pstate->r[srcReg]);
                if(psym->size>1)
                {
                    pstate->setMemoryByte(psym->label,uint8_t(pstate->r[srcReg]>>8));
                    //prog.image()[psym->label+1] = (uint8_t)(pstate->r[srcReg] >> 8);
                }
                psym->duVal.setFlags(eDuVal::DEF);
            }
        }
    }
}



/* Updates the offset entry to the stack frame table (arguments),
 * and returns a pointer to such entry. */
void STKFRAME::updateFrameOff ( int16_t off, int _size, uint16_t duFlag)
{
    /* Check for symbol in stack frame table */
    auto iter=findByLabel(off);
    if(iter!=end())
    {
        if (iter->size < _size)
        {
            iter->size = _size;
        }
    }
    else
    {
        char nm[16];
        STKSYM new_sym;

        sprintf (nm, "arg%" PRIu64, uint64_t(size()));
        new_sym.name = nm;
        new_sym.label= off;
        new_sym.size = _size;
        new_sym.type = TypeContainer::defaultTypeForSize(_size);
        if (duFlag == eDuVal::USE)  /* must already have init value */
        {
            new_sym.duVal.use=1;
            //new_sym.duVal.val=1;
        }
        else
        {
            new_sym.duVal.setFlags(duFlag);
        }
        push_back(new_sym);
        this->numArgs++;
    }

    /* Save maximum argument offset */
    if ((uint32_t)this->m_maxOff < (off + (uint32_t)_size))
        this->m_maxOff = off + (int16_t)_size;
}


/* lookupAddr - Looks up a data reference in the symbol table and stores it
 *      if necessary.
 *      Returns a pointer to the symbol in the
 *      symbol table, or Null if it's not a direct memory offset.  */
static SYM * lookupAddr (LLOperand *pm, STATE *pstate, int size, uint16_t duFlag)
{
    PROG &prog(Project::get()->prog);
    int     i;
    SYM *    psym=nullptr;
    uint32_t   operand;
    bool created_new=false;
    if (pm->regi != rUNDEF)
        return nullptr; // register or indexed

    /* Global var */
    if (pm->segValue)  /* there is a value in the seg field */
    {
        operand = opAdr (pm->segValue, pm->off);
        psym = Project::get()->symtab.updateGlobSym (operand, size, duFlag,created_new);
    }
    else if (pstate->f[pm->seg]) /* new value */
    {
        pm->segValue = pstate->r[pm->seg];
        operand = opAdr(pm->segValue, pm->off);
        psym = Project::get()->symtab.updateGlobSym (operand, size, duFlag,created_new);

        /* Flag new memory locations that are segment values */
        if (created_new)
        {
            if (size == 4)
                operand += 2;   /* High uint16_t */
            for (i = 0; i < prog.cReloc; i++)
                if (prog.relocTable[i] == operand) {
                    psym->flg = SEG_IMMED;
                    break;
                }
        }
    }
    /* Check for out of bounds */
    if (psym and (psym->label < (uint32_t)prog.cbImage))
        return psym;
    return nullptr;
}


/* setState - Assigns a value to a reg.     */
void STATE::setState(uint16_t reg, int16_t value)
{
    value &= 0xFFFF;
    r[reg] = value;
    f[reg] = true;
    switch (reg) {
        case rAX: case rCX: case rDX: case rBX:
            r[reg + rAL - rAX] = value & 0xFF;
            f[reg + rAL - rAX] = true;
            r[reg + rAH - rAX] = (value >> 8) & 0xFF;
            f[reg + rAH - rAX] = true;
            break;

        case rAL: case rCL: case rDL: case rBL:
            if (f[reg - rAL + rAH]) {
                r[reg - rAL + rAX] =(r[reg - rAL + rAH] << 8) + (value & 0xFF);
                f[reg - rAL + rAX] = true;
            }
            break;

        case rAH: case rCH: case rDH: case rBH:
            if (f[reg - rAH + rAL])
            {
                r[reg - rAH + rAX] = r[reg - rAH + rAL] + ((value & 0xFF) << 8);
                f[reg - rAH + rAX] = true;
            }
            break;
    }
}


/* labelSrchRepl - Searches Icode for instruction with label = target, and
    replaces *pIndex with an icode index */


/* setBits - Sets memory bitmap bits for BM_CODE or BM_DATA (additively) */
static void setBits(int16_t type, uint32_t start, uint32_t len)
{
    PROG &prog(Project::get()->prog);
    uint32_t   i;

    if (start < (uint32_t)prog.cbImage)
    {
        if (start + len > (uint32_t)prog.cbImage)
            len = (uint32_t)(prog.cbImage - start);

        for (i = start + len - 1; i >= start; i--)
        {
            prog.map[i >> 2] |= type << ((i & 3) << 1);
            if (i == 0) break;  // Fixes inf loop!
        }
    }
}

/* Checks which registers were used and updates the du.u flag.
 * Places local variables on the local symbol table.
 * Arguments: d     : SRC or DST icode operand
 *            pIcode: ptr to icode instruction
 *            pProc : ptr to current procedure structure
 *            pstate: ptr to current procedure state
 *            size  : size of the operand
 *            ix    : current index into icode array    */
static void use (opLoc d, ICODE & pIcode, Function * pProc, STATE * pstate, int size)
{
    const LLOperand * pm = pIcode.ll()->get(d) ;
    SYM *  psym;

    if ( Machine_X86::isMemOff(pm->regi) )
    {
        if (pm->regi == INDEX_BP)      /* indexed on bp */
        {
            if (pm->off >= 2) {
                pProc->args.updateFrameOff ( pm->off, size, eDuVal::USE);
            }
            else if (pm->off < 0)
                pProc->localId.newByteWordStk (TYPE_WORD_SIGN, pm->off, 0);
        }

        else if (pm->regi == INDEX_BP_SI or pm->regi == INDEX_BP_DI)
            pProc->localId.newByteWordStk (TYPE_WORD_SIGN, pm->off,
                                           (uint8_t)((pm->regi == INDEX_BP_SI) ? rSI : rDI));

        else if ((pm->regi >= INDEX_SI) and (pm->regi <= INDEX_BX))
        {
            if ((pm->seg == rDS) and (pm->regi == INDEX_BX))    /* bx */
            {
                if (pm->off > 0)    /* global indexed variable */
                    pProc->localId.newIntIdx(pm->segValue, pm->off, rBX,TYPE_WORD_SIGN);
            }
            pIcode.du.use.addReg(pm->regi);
        }

        else
        {
            psym = lookupAddr(const_cast<LLOperand *>(pm), pstate, size, eDuVal::USE);
            if( nullptr != psym )
            {
                setBits (BM_DATA, psym->label, (uint32_t)size);
                pIcode.ll()->setFlags(SYM_USE);
                pIcode.ll()->caseEntry = distance(&Project::get()->symtab[0],psym); //WARNING: was setting case count

            }
        }
    }

    /* Use of register */
    else if ((d == DST) or ((d == SRC) and (not pIcode.ll()->srcIsImmed())))
        pIcode.du.use.addReg(pm->regi);
}


/* Checks which registers were defined (ie. got a new value) and updates the
 * du.d flag.
 * Places local variables in the local symbol table.    */
static void def (opLoc d, ICODE & pIcode, Function * pProc, STATE * pstate, int size)
{
    LLOperand *pm   = pIcode.ll()->get(d);
    if (pm->regi==0)
    {
        SYM *  psym;
        psym = lookupAddr(pm, pstate, size, eDEF);
        if (nullptr!=psym)
        {
            setBits(BM_DATA, psym->label, (uint32_t)size);
            pIcode.ll()->setFlags(SYM_DEF);
            pIcode.ll()->caseEntry = distance(&Project::get()->symtab[0],psym); // WARNING: was setting Case count
        }
    }
    else if (pm->regi >= INDEX_BX_SI)
    {
        if (pm->regi == INDEX_BP)      /* indexed on bp */
        {
            if (pm->off >= 2)
                pProc->args.updateFrameOff ( pm->off, size, eDEF);
            else if (pm->off < 0)
                pProc->localId.newByteWordStk (TYPE_WORD_SIGN, pm->off, 0);
        }

        else if (pm->regi == INDEX_BP_SI or pm->regi == INDEX_BP_DI)
        {
            pProc->localId.newByteWordStk(TYPE_WORD_SIGN, pm->off,
                                          (uint8_t)((pm->regi == INDEX_BP_SI) ? rSI : rDI));
        }

        else if ((pm->regi >= INDEX_SI) and (pm->regi <= INDEX_BX))
        {
            if ((pm->seg == rDS) and (pm->regi == INDEX_BX))    /* bx */
            {
                if (pm->off > 0)        /* global var */
                    pProc->localId.newIntIdx(pm->segValue, pm->off, rBX,TYPE_WORD_SIGN);
            }
            pIcode.du.use.addReg(pm->regi);
        }
    }
    /* Definition of register */
    else if ((d == DST) or ((d == SRC) and (not pIcode.ll()->srcIsImmed())))
    {
        assert(not pIcode.ll()->match(iPUSH));
        pIcode.du1.addDef(pm->regi);
        pIcode.du.def.addReg(pm->regi);
    }
}


/* use_def - operand is both use and def'd.
 * Note: the destination will always be a register, stack variable, or global
 *       variable.      */
static void use_def(opLoc d, ICODE & pIcode, Function * pProc, STATE * pstate, int cb)
{
    const LLOperand *  pm = pIcode.ll()->get(d);

    use (d, pIcode, pProc, pstate, cb);

    if (pm->regi < INDEX_BX_SI)                   /* register */
    {
        assert(not pIcode.ll()->match(iPUSH));
        pIcode.du1.addDef(pm->regi);
        pIcode.du.def.addReg(pm->regi);
    }
}


/* Set DU vector, local variables and arguments, and DATA bits in the
 * bitmap       */
extern LLOperand convertOperand(const x86_op_t &from);
void Function::process_operands(ICODE & pIcode,  STATE * pstate)
{
    LLInst &ll_ins(*pIcode.ll());

    int   sseg = (ll_ins.src().seg)? ll_ins.src().seg: rDS;
    int   cb   = pIcode.ll()->testFlags(B) ? 1: 2;
    //x86_op_t *im= pIcode.insn.x86_get_imm();
    bool Imm  = (pIcode.ll()->srcIsImmed());

    switch (pIcode.ll()->getOpcode()) {
        case iAND:  case iOR:   case iXOR:
        case iSAR:  case iSHL:  case iSHR:
        case iRCL:  case iRCR:  case iROL:  case iROR:
        case iADD:  case iADC:  case iSUB:  case iSBB:
            if (not Imm) {
                use(SRC, pIcode, this, pstate, cb);
            }
        case iINC:  case iDEC:  case iNEG:  case iNOT:
        case iAAA:  case iAAD:  case iAAM:  case iAAS:
        case iDAA:  case iDAS:
            use_def(DST, pIcode, this, pstate, cb);
            break;

        case iXCHG:
            /* This instruction is replaced by 3 instructions, only need
             * to define the src operand and use the destination operand
             * in the mean time.*/
            use(SRC, pIcode, this, pstate, cb);
            def(DST, pIcode, this, pstate, cb);
            break;

        case iTEST: case iCMP:
            if (not Imm)
                use(SRC, pIcode, this, pstate, cb);
            use(DST, pIcode, this, pstate, cb);
            break;

        case iDIV:  case iIDIV:
            use(SRC, pIcode, this, pstate, cb);
            if (cb == 1)
                pIcode.du.use.addReg(rTMP);
            break;

        case iMUL:  case iIMUL:
            use(SRC, pIcode, this, pstate, cb);
            if (not Imm)
            {
                use (DST, pIcode, this, pstate, cb);
                if (cb == 1)
                {
                    pIcode.du.def.addReg(rAX);
                    pIcode.du1.addDef(rAX);
                }
                else
                {
                    pIcode.du.def.addReg(rAX).addReg(rDX);
                    pIcode.du1.addDef(rAX).addDef(rDX);
                }
            }
            else
                def (DST, pIcode, this, pstate, cb);
            break;

        case iSIGNEX:
            cb = pIcode.ll()->testFlags(SRC_B) ? 1 : 2;
            if (cb == 1)                    /* uint8_t */
            {
                pIcode.du.def.addReg(rAX);
                pIcode.du1.addDef(rAX);
                pIcode.du.use.addReg(rAL);
            }
            else                            /* uint16_t */
            {
                pIcode.du.def.addReg(rDX).addReg(rAX);
                pIcode.du1.addDef(rAX).addDef(rDX);
                pIcode.du.use.addReg(rAX);
            }
            break;

        case iCALLF:            /* Ignore def's on CS for now */
            cb = 4;
        case iCALL:  case iPUSH:  case iPOP:
            if (not Imm) {
                if (pIcode.ll()->getOpcode() == iPOP)
                    def(DST, pIcode, this, pstate, cb);
                else
                    use(DST, pIcode, this, pstate, cb);
            }
            break;

        case iESC:  /* operands may be larger */
            use(DST, pIcode, this, pstate, cb);
            break;

        case iLDS:  case iLES:
        {
            eReg r=((pIcode.ll()->getOpcode() == iLDS) ? rDS : rES);
            pIcode.du.def.addReg(r);
            pIcode.du1.addDef(r);
            cb = 4;
            // fallthrough
        }
        case iMOV:
            use(SRC, pIcode, this, pstate, cb);
            def(DST, pIcode, this, pstate, cb);
            break;

        case iLEA:
            use(SRC, pIcode, this, pstate, 2);
            def(DST, pIcode, this, pstate, 2);
            break;

        case iBOUND:
            use(SRC, pIcode, this, pstate, 4);
            use(DST, pIcode, this, pstate, cb);
            break;

        case iJMPF:
            cb = 4;
        case iJMP:
            if (not Imm)
                use(SRC, pIcode, this, pstate, cb);
            break;

        case iLOOP:
        case iLOOPE:
        case iLOOPNE:
            pIcode.du.def.addReg(rCX);
            pIcode.du1.addDef(rCX);
        case iJCXZ:
            pIcode.du.use.addReg(rCX);
            break;

        case iREPNE_CMPS: case iREPE_CMPS:  case iREP_MOVS:
            pIcode.du.addDefinedAndUsed(rCX);
            pIcode.du1.addDef(rCX);
        case iCMPS:  case iMOVS:
            pIcode.du.addDefinedAndUsed(rSI);
            pIcode.du.addDefinedAndUsed(rDI);
            pIcode.du1.addDef(rSI).addDef(rDI);
            pIcode.du.use.addReg(rES).addReg(sseg);
            break;

        case iREPNE_SCAS: case iREPE_SCAS:  case iREP_STOS:  case iREP_INS:
            pIcode.du.addDefinedAndUsed(rCX);
            pIcode.du1.addDef(rCX);
        case iSCAS:  case iSTOS:  case iINS:
            pIcode.du.def.addReg(rDI);
            pIcode.du1.addDef(rDI);
            if (pIcode.ll()->getOpcode() == iREP_INS or pIcode.ll()->getOpcode()== iINS)
            {
                pIcode.du.use.addReg(rDI).addReg(rES).addReg(rDX);
            }
            else
            {
                pIcode.du.use.addReg(rDI).addReg(rES).addReg((cb == 2)? rAX: rAL);
            }
            break;

        case iREP_LODS:
            pIcode.du.addDefinedAndUsed(rCX);
            pIcode.du1.addDef(rCX);
        case iLODS:
        {
            eReg r = (cb==2)? rAX: rAL;
            pIcode.du.addDefinedAndUsed(rSI);
            pIcode.du1.addDef(rSI);
            pIcode.du.def.addReg(r);
            pIcode.du1.addDef(r);
            pIcode.du.use.addReg(sseg);
        }
            break;
        case iREP_OUTS:
            pIcode.du.addDefinedAndUsed(rCX);
            pIcode.du1.addDef(rCX);
        case iOUTS:
            pIcode.du.addDefinedAndUsed(rSI);
            pIcode.du1.addDef(rSI);
            pIcode.du.use.addReg(rDX).addReg(sseg);
            break;

        case iIN:
        case iOUT:
            def(DST, pIcode, this, pstate, cb);
            if (not Imm)
            {
                pIcode.du.use.addReg(rDX);
            }
            break;
    }

    for (int i = rSP; i <= rBH; i++)        /* Kill all defined registers */
        if (pIcode.ll()->flagDU.d & (1 << i))
            pstate->f[i] = false;
}

