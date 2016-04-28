#include "Procedure.h"

#include "msvc_fixes.h"
#include "project.h"
#include "scanner.h"

//FunctionType *Function::getFunctionType() const
//{
//    return &m_type;
//}

/* Does some heuristic pruning.  Looks for ptrs. into the table
 * and for addresses that don't appear to point to valid code.
*/
void JumpTable::pruneEntries(uint16_t cs)
{
    PROG *prg(Project::get()->binary());
    for (uint32_t i = start; i < finish; i += 2)
    {
        uint32_t target = cs + LH(&prg->image()[i]);
        if (target < finish and target >= start)
            finish = target;
        else if (target >= (uint32_t)prg->cbImage)
            finish = i;
    }
    ICODE _Icode; // used as scan input
    for (uint32_t i = start; i < finish; i += 2)
    {
        uint32_t target = cs + LH(&prg->image()[i]);
        /* Be wary of 00 00 as code - it's probably data */
        if (not (prg->image()[target] or prg->image()[target+1]) or scan(target, _Icode))
            finish = i;
    }

}


void Function::callingConv(CConv::CC_Type v) {
    type->setCallingConvention(v);
    getFunctionType()->m_call_conv->calculateStackLayout(this);
}

void FunctionType::setCallingConvention(CConv::CC_Type cc)
{
        m_call_conv=CConv::create(cc);
        assert(m_call_conv);
}
