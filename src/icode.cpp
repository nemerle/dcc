// Object oriented icode code for dcc
// (C) 1997 Mike Van Emmerik

#include <stdlib.h>
#include <malloc.h>
#include <memory.h>

#include "types.h"		// Common types like byte, etc
#include "ast.h"		// Some icode types depend on these
#include "icode.h"

void   *reallocVar(void *p, Int newsize);                   /* frontend.c !?   */
#define ICODE_DELTA 25		// Amount to allocate for new chunk


CIcodeRec::CIcodeRec()
{
	numIcode = 0;
	alloc = 0;
	icode = 0;			// Initialise the pointer to 0
}

CIcodeRec::~CIcodeRec()
{
	if (icode)
	{
		free(icode);
	}
}

PICODE CIcodeRec::addIcode(PICODE pIcode)

/* Copies the icode that is pointed to by pIcode to the icode array.
 * If there is need to allocate extra memory, it is done so, and
 * the alloc variable is adjusted.        */
{
	PICODE resIcode;

    if (numIcode == alloc)
    {
        alloc += ICODE_DELTA;
		icode = (PICODE)reallocVar(icode, alloc * sizeof(ICODE));
		memset (&icode[numIcode], 0, ICODE_DELTA * sizeof(ICODE));

    }
    resIcode = (PICODE)memcpy (&icode[numIcode], pIcode, 
                               sizeof(ICODE));
    numIcode++;
    return (resIcode);
}

PICODE CIcodeRec::GetFirstIcode()
{
	return icode;
}

/* Don't need this; just pIcode++ since array is guaranteed to be contiguous
PICODE CIcodeRec::GetNextIcode(PICODE pCurIcode)
{
	int idx = pCurIcode - icode;		// Current index
	ASSERT(idx+1 < numIcode);
	return &icode[idx+1];
}
*/

boolT CIcodeRec::IsValid(PICODE pCurIcode)
{
	int idx = pCurIcode - icode;		// Current index
	return idx < numIcode;
}

int CIcodeRec::GetNumIcodes()
{
	return numIcode;
}

void CIcodeRec::SetInBB(int start, int end, struct _BB* pnewBB)
{
	for (int i = start; i <= end; i++)
	icode[i].inBB = pnewBB;
}

void CIcodeRec::SetImmediateOp(int ip, dword dw)
{
	icode[ip].ic.ll.immed.op = dw;
}

void CIcodeRec::SetLlFlag(int ip, dword flag)
{
	icode[ip].ic.ll.flg |= flag;
}

dword CIcodeRec::GetLlFlag(int ip)
{
	return icode[ip].ic.ll.flg;
}

void CIcodeRec::ClearLlFlag(int ip, dword flag)
{
	icode[ip].ic.ll.flg &= (~flag);
}

void CIcodeRec::SetLlInvalid(int ip, boolT fInv)
{
	icode[ip].invalid = fInv;
}

dword CIcodeRec::GetLlLabel(int ip)
{
	return icode[ip].ic.ll.label;
}

llIcode CIcodeRec::GetLlOpcode(int ip)
{
	return icode[ip].ic.ll.opcode;
}


boolT CIcodeRec::labelSrch(dword target, Int *pIndex)
/* labelSrchRepl - Searches the icodes for instruction with label = target, and
    replaces *pIndex with an icode index */
{
    Int  i;

    for (i = 0; i < numIcode; i++)
    {
        if (icode[i].ic.ll.label == target)
        {
            *pIndex = i;
            return TRUE;
        }
    }
    return FALSE;
}

PICODE CIcodeRec::GetIcode(int ip)
{
	return &icode[ip];
}




