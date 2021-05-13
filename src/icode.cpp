// Object oriented icode code for dcc
// (C) 1997 Mike Van Emmerik
#include "icode.h"

#include "ast.h" // Some icode types depend on these
#include "dcc.h"
#include "msvc_fixes.h"
#include "types.h" // Common types like uint8_t, etc

#include <stdlib.h>

ICODE::TypeFilter<HIGH_LEVEL_ICODE> ICODE::select_high_level;
ICODE::TypeAndValidFilter<HIGH_LEVEL_ICODE> ICODE::select_valid_high_level;
CIcodeRec::CIcodeRec()
{
}

/* Copies the icode that is pointed to by pIcode to the icode array.
 * If there is need to allocate extra memory, it is done so, and
 * the alloc variable is adjusted.        */
ICODE * CIcodeRec::addIcode(ICODE *pIcode)
{
    entries.push_back(*pIcode);
    entries.back().loc_ip = entries.size()-1;
    return &entries.back();
}

void CIcodeRec::SetInBB(rCODE &rang, BB *pnewBB)
{
    for(ICODE &ic : rang)
        ic.setParent(pnewBB);
}

/* labelSrchRepl - Searches the icodes for instruction with label = target, and
   replaces *pIndex with an icode index */
bool CIcodeRec::labelSrch(uint32_t target, uint32_t &pIndex)
{
    iICODE location=labelSrch(target);
    if(entries.end()==location)
        return false;
    pIndex=location->loc_ip;
    return true;
}

bool CIcodeRec::alreadyDecoded(uint32_t target)
{
    iICODE location=labelSrch(target);
    return location!=entries.end();
}
CIcodeRec::iterator CIcodeRec::labelSrch(uint32_t target)
{
    return find_if(entries.begin(),entries.end(),[target](ICODE &l) -> bool {
        return l.ll()->label==target;
    });
}
ICODE * CIcodeRec::GetIcode(size_t ip)
{
    assert(ip<entries.size());
    iICODE res=entries.begin();
    advance(res,ip);
    return &(*res);
}

extern int getNextLabel();
extern bundle cCode;
/* Checks the given icode to determine whether it has a label associated
 * to it.  If so, a goto is emitted to this label; otherwise, a new label
 * is created and a goto is also emitted.
 * Note: this procedure is to be used when the label is to be backpatched
 *       onto code in cCode.code */
void LLInst::emitGotoLabel (int indLevel)
{
    if ( not testFlags(HLL_LABEL) ) /* node hasn't got a lab */
    {
        /* Generate new label */
        hllLabNum = getNextLabel();
        setFlags(HLL_LABEL);

        /* Node has been traversed already, so backpatch this label into
                 * the code */
        cCode.code.addLabelBundle (codeIdx, hllLabNum);
    }
    cCode.appendCode( "%sgoto L%ld;\n", indentStr(indLevel), hllLabNum);
    stats.numHLIcode++;
}



bool LLOperand::isReg() const
{
    return (regi>=rAX) and (regi<=rTMP);
}
void LLOperand::addProcInformation(int param_count, CConv::Type call_conv)
{
    proc.proc->cbParam = (int16_t)param_count;
    proc.cb = param_count;
    proc.proc->callingConv(call_conv);
}
void HLTYPE::setCall(Function *proc)
{
    opcode = HLI_CALL;
    call.proc = proc;
    call.args = new STKFRAME;
}
bool AssignType::removeRegFromLong(eReg regi, LOCAL_ID *locId)
{
    m_lhs=lhs()->performLongRemoval(regi,locId);
    return true;
}
void AssignType::lhs(Expr *l)
{
    assert(dynamic_cast<UnaryOperator *>(l));
    m_lhs=l;
}


LivenessSet &LivenessSet::operator&=(const LivenessSet &other)
{
    std::set<eReg> res;
    std::set_intersection(registers.begin(),registers.end(),
                          other.registers.begin(),other.registers.end(),
                          std::inserter(res, res.end()));
    registers = res;
    return *this;
}

LivenessSet &LivenessSet::operator-=(const LivenessSet &other)
{
    std::set<eReg> res;
    std::set_difference(registers.begin(),registers.end(),
                        other.registers.begin(),other.registers.end(),
                        std::inserter(res, res.end()));
    registers = res;
    return *this;
}
