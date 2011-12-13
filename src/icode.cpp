// Object oriented icode code for dcc
// (C) 1997 Mike Van Emmerik

#include <stdlib.h>
#include <malloc.h>
#include <memory.h>

#include "dcc.h"
#include "types.h"		// Common types like byte, etc
#include "ast.h"		// Some icode types depend on these
#include "icode.h"

#define ICODE_DELTA 25		// Amount to allocate for new chunk


CIcodeRec::CIcodeRec()
{
}

CIcodeRec::~CIcodeRec()
{
}

/* Copies the icode that is pointed to by pIcode to the icode array.
 * If there is need to allocate extra memory, it is done so, and
 * the alloc variable is adjusted.        */
ICODE * CIcodeRec::addIcode(ICODE *pIcode)
{
    push_back(*pIcode);
    back().loc_ip = size()-1;
    return &back();
}

ICODE * CIcodeRec::GetFirstIcode()
{
    return &front();
}

/* Don't need this; just pIcode++ since array is guaranteed to be contiguous
ICODE * CIcodeRec::GetNextIcode(ICODE * pCurIcode)
{
        int idx = pCurIcode - icode;		// Current index
        ASSERT(idx+1 < numIcode);
        return &icode[idx+1];
}
*/

boolT CIcodeRec::IsValid(ICODE *pCurIcode)
{
    ptrdiff_t idx = pCurIcode - &this->front();		// Current index
    return (idx>=0) && (idx < size());
}

int CIcodeRec::GetNumIcodes()
{
    return size();
}

void CIcodeRec::SetInBB(int start, int end, BB *pnewBB)
{
    for (int i = start; i <= end; i++)
        at(i).inBB = pnewBB;
}

void CIcodeRec::SetImmediateOp(int ip, dword dw)
{
    at(ip).ic.ll.immed.op = dw;
}

void CIcodeRec::SetLlFlag(int ip, dword flag)
{
    at(ip).ic.ll.flg |= flag;
}

dword CIcodeRec::GetLlFlag(int ip)
{
    return at(ip).ic.ll.flg;
}

void CIcodeRec::ClearLlFlag(int ip, dword flag)
{
    at(ip).ic.ll.flg &= (~flag);
}

void CIcodeRec::SetLlInvalid(int ip, boolT fInv)
{
    at(ip).invalid = fInv;
}

dword CIcodeRec::GetLlLabel(int ip)
{
    return at(ip).ic.ll.label;
}

llIcode CIcodeRec::GetLlOpcode(int ip)
{
    return at(ip).ic.ll.opcode;
}


/* labelSrchRepl - Searches the icodes for instruction with label = target, and
    replaces *pIndex with an icode index */
boolT CIcodeRec::labelSrch(dword target, Int *pIndex)
{
    Int  i;

    for (i = 0; i < size(); i++)
    {
        if (at(i).ic.ll.label == target)
        {
            *pIndex = i;
            return TRUE;
        }
    }
    return FALSE;
}

ICODE * CIcodeRec::GetIcode(int ip)
{
    return &at(ip);
}

extern char *indent(int level);
extern Int getNextLabel();
extern bundle cCode;
/* Checks the given icode to determine whether it has a label associated
 * to it.  If so, a goto is emitted to this label; otherwise, a new label
 * is created and a goto is also emitted.
 * Note: this procedure is to be used when the label is to be backpatched
 *       onto code in cCode.code */
void ICODE::emitGotoLabel (Int indLevel)
{
    if (! (ic.ll.flg & HLL_LABEL)) /* node hasn't got a lab */
    {
        /* Generate new label */
        ic.ll.hllLabNum = getNextLabel();
        ic.ll.flg |= HLL_LABEL;

        /* Node has been traversed already, so backpatch this label into
                 * the code */
        addLabelBundle (cCode.code, codeIdx, ic.ll.hllLabNum);
    }
    cCode.appendCode( "%sgoto L%ld;\n", indent(indLevel), ic.ll.hllLabNum);
    stats.numHLIcode++;
}

