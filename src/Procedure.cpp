#include "Procedure.h"
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
        uint32_t target = cs + LH(&prg->Image[i]);
        if (target < finish && target >= start)
            finish = target;
        else if (target >= (uint32_t)prg->cbImage)
            finish = i;
    }
    ICODE _Icode; // used as scan input
    for (uint32_t i = start; i < finish; i += 2)
    {
        uint32_t target = cs + LH(&prg->Image[i]);
        /* Be wary of 00 00 as code - it's probably data */
        if (! (prg->Image[target] || prg->Image[target+1]) || scan(target, _Icode))
            finish = i;
    }

}
