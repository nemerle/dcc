// Object oriented icode code for dcc
// (C) 1997 Mike Van Emmerik

#include <stdlib.h>
#include <malloc.h>
#include <memory.h>

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




