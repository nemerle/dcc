/*****************************************************************************
 * 			dcc project CFG related functions
 * (C) Cristina Cifuentes
 ****************************************************************************/

#include "dcc.h"
#include <string.h>
#if __BORLAND__
#include <alloc.h>
#else
#include <malloc.h>		/* For free() */
#endif

static PBB  rmJMP(PPROC pProc, Int marker, PBB pBB);
static void mergeFallThrough(PPROC pProc, PBB pBB);
static void dfsNumbering(PBB pBB, PBB *dfsLast, Int *first, Int *last);

/*****************************************************************************
 * createCFG - Create the basic control flow graph
 ****************************************************************************/
PBB createCFG(PPROC pProc)
{
	/* Splits Icode associated with the procedure into Basic Blocks.
	 * The links between BBs represent the control flow graph of the 
	 * procedure.
	 * A Basic Block is defined to end on one of the following instructions:
	 * 1) Conditional and unconditional jumps
	 * 2) CALL(F)
	 * 3) RET(F)
	 * 4) On the instruction before a join (a flagged TARGET)
	 * 5) Repeated string instructions 
	 * 6) End of procedure
	*/   
	Int		i;
	Int		ip, start;
	BB		cfg;
	PBB		psBB;
	PBB		pBB    = &cfg;
	PICODE	pIcode = pProc->Icode.GetFirstIcode();

	cfg.next = NULL;
	stats.numBBbef = stats.numBBaft = 0;
	for (ip = start = 0; pProc->Icode.IsValid(pIcode); ip++, pIcode++) 
	{
		/* Stick a NOWHERE_NODE on the end if we terminate
		 * with anything other than a ret, jump or terminate */
		if (ip + 1 == pProc->Icode.GetNumIcodes() && 
			! (pIcode->ic.ll.flg & TERMINATES) &&
			pIcode->ic.ll.opcode != iJMP && pIcode->ic.ll.opcode != iJMPF &&
			pIcode->ic.ll.opcode != iRET && pIcode->ic.ll.opcode != iRETF)
			newBB(pBB, start, ip, NOWHERE_NODE, 0, pProc);

		/* Only process icodes that have valid instructions */
		else if ((pIcode->ic.ll.flg & NO_CODE) != NO_CODE) 
		{
			switch (pIcode->ic.ll.opcode) {
			case iJB:  case iJBE:  case iJAE:  case iJA:
			case iJL:  case iJLE:  case iJGE:  case iJG:
			case iJE:  case iJNE:  case iJS:   case iJNS:
			case iJO:  case iJNO:  case iJP:   case iJNP:
			case iJCXZ:
				pBB = newBB(pBB, start, ip, TWO_BRANCH, 2, pProc);
			CondJumps:
				start = ip + 1;
				pBB->edges[0].ip = (dword)start;
				/* This is for jumps off into nowhere */
				if (pIcode->ic.ll.flg & NO_LABEL)
					pBB->numOutEdges--;
				else
					pBB->edges[1].ip = pIcode->ic.ll.immed.op;
				break;

			case iLOOP: case iLOOPE: case iLOOPNE:
				pBB = newBB(pBB, start, ip, LOOP_NODE, 2, pProc);
				goto CondJumps;

			case iJMPF: case iJMP:
				if (pIcode->ic.ll.flg & SWITCH)
				{
					pBB = newBB(pBB, start, ip, MULTI_BRANCH,
							   pIcode->ic.ll.caseTbl.numEntries, pProc);
					for (i = 0; i < pIcode->ic.ll.caseTbl.numEntries; i++)
						pBB->edges[i].ip = pIcode->ic.ll.caseTbl.entries[i];
					pProc->hasCase = TRUE;
				}
				else if ((pIcode->ic.ll.flg & (I | NO_LABEL)) == I) {
				    pBB = newBB(pBB, start, ip, ONE_BRANCH, 1, pProc);
					pBB->edges[0].ip = pIcode->ic.ll.immed.op;
				}
				else	
					newBB(pBB, start, ip, NOWHERE_NODE, 0, pProc);
				start = ip + 1;
				break;

			case iCALLF: case iCALL:
				{ PPROC p = pIcode->ic.ll.immed.proc.proc;
				  if (p)
					  i = ((p->flg) & TERMINATES) ? 0 : 1;
				  else
					  i = 1;
				  pBB = newBB(pBB, start, ip, CALL_NODE, i, pProc);
				  start = ip + 1;
				  if (i)
					 pBB->edges[0].ip = (dword)start;
				}
				break;

			case iRET:  case iRETF:
				newBB(pBB, start, ip, RETURN_NODE, 0, pProc);
				start = ip + 1;
				break;

			default:
				/* Check for exit to DOS */
				if (pIcode->ic.ll.flg & TERMINATES) 
				{
					pBB = newBB(pBB, start, ip, TERMINATE_NODE, 0, pProc);
					start = ip + 1;
				}
				/* Check for a fall through */
				else if (pProc->Icode.GetFirstIcode()[ip + 1].ic.ll.flg & (TARGET | CASE))
				{
					pBB = newBB(pBB, start, ip, FALL_NODE, 1, pProc);
					start = ip + 1;
					pBB->edges[0].ip = (dword)start;
				}
				break;
			}
		}
	}

	/* Convert list of BBs into a graph */
	for (pBB = cfg.next; pBB; pBB = pBB->next)
	{
		for (i = 0; i < pBB->numOutEdges; i++)
		{
			ip = pBB->edges[i].ip;
			if (ip >= SYNTHESIZED_MIN)
				fatalError (INVALID_SYNTHETIC_BB);
			else
			{
				for (psBB = cfg.next; psBB; psBB = psBB->next)
					if (psBB->start == ip)
					{
						pBB->edges[i].BBptr = psBB;
						psBB->numInEdges++;
						break;
					}
				if (! psBB)
					fatalError(NO_BB, ip, pProc->name);
			}
		}
	}
	return cfg.next;
}


/*****************************************************************************
 * newBB - Allocate new BB and link to end of list
 *****************************************************************************/
PBB newBB (PBB pBB, Int start, Int ip, byte nodeType, Int numOutEdges, 
		   PPROC pproc)
{
  PBB pnewBB;

	pnewBB = allocStruc(BB);
	memset (pnewBB, 0, sizeof(BB));
	pnewBB->nodeType = nodeType;	/* Initialise */
	pnewBB->start = start;
	pnewBB->length = ip - start + 1;
	pnewBB->numOutEdges = (byte)numOutEdges;
	pnewBB->immedDom = NO_DOM;
	pnewBB->loopHead = pnewBB->caseHead = pnewBB->caseTail =
		pnewBB->latchNode= pnewBB->loopFollow = NO_NODE;

	if (numOutEdges)
		pnewBB->edges = (TYPEADR_TYPE*)allocMem(numOutEdges * sizeof(TYPEADR_TYPE));

	/* Mark the basic block to which the icodes belong to, but only for
	 * real code basic blocks (ie. not interval bbs) */
	if (start >= 0)
		pproc->Icode.SetInBB(start, ip, pnewBB);

	while (pBB->next)		/* Link */
		pBB = pBB->next;
	pBB->next = pnewBB;

	if (start != -1) {		/* Only for code BB's */
		stats.numBBbef++;
	}
	return pnewBB;
}


/*****************************************************************************
 * freeCFG - Deallocates a cfg
 ****************************************************************************/
void freeCFG(PBB cfg)
{
	PBB	pBB;

	for (pBB = cfg; pBB; pBB = cfg) {
		if (pBB->inEdges)
			free(pBB->inEdges);
		if (pBB->edges)
			free(pBB->edges);
		cfg = pBB->next;
		free(pBB);
	}
}


/*****************************************************************************
 * compressCFG - Remove redundancies and add in-edge information
 ****************************************************************************/
void compressCFG(PPROC pProc)
{ PBB	pBB, pNxt;
  Int	ip, first=0, last, i;

	/* First pass over BB list removes redundant jumps of the form
	 * (Un)Conditional -> Unconditional jump  */
	for (pBB = pProc->cfg; pBB; pBB = pBB->next)
		if (pBB->numInEdges != 0 && (pBB->nodeType == ONE_BRANCH ||
			pBB->nodeType == TWO_BRANCH))
			for (i = 0; i < pBB->numOutEdges; i++) 
	{
			ip   = pBB->start + pBB->length - 1;
			pNxt = rmJMP(pProc, ip, pBB->edges[i].BBptr);

			if (pBB->numOutEdges)   /* Might have been clobbered */
			{
				pBB->edges[i].BBptr = pNxt;
				pProc->Icode.SetImmediateOp(ip, (dword)pNxt->start);
			}
	}

	/* Next is a depth-first traversal merging any FALL_NODE or
	 * ONE_BRANCH that fall through to a node with that as their only
	 * in-edge. */
	mergeFallThrough(pProc, pProc->cfg);

	/* Remove redundant BBs created by the above compressions
	 * and allocate in-edge arrays as required. */
	stats.numBBaft = stats.numBBbef;

	for (pBB = pProc->cfg; pBB; pBB = pNxt)
	{
		pNxt = pBB->next;
		if (pBB->numInEdges == 0) 
		{
			if (pBB == pProc->cfg)	/* Init it misses out on */
				pBB->index = UN_INIT;
			else
			{
				if (pBB->numOutEdges)
					free(pBB->edges);
				free(pBB);
				stats.numBBaft--;
			}
		}
		else 
		{
			pBB->inEdgeCount = pBB->numInEdges;
			pBB->inEdges = (PBB*)allocMem(pBB->numInEdges * sizeof(PBB));
		}
	}

	/* Allocate storage for dfsLast[] array */
	pProc->numBBs = stats.numBBaft;
	pProc->dfsLast = (PBB*)allocMem(pProc->numBBs * sizeof(PBB)); 

	/* Now do a dfs numbering traversal and fill in the inEdges[] array */
	last = pProc->numBBs - 1;
	dfsNumbering(pProc->cfg, pProc->dfsLast, &first, &last);
}


/****************************************************************************
 * rmJMP - If BB addressed is just a JMP it is replaced with its target
 ***************************************************************************/
static PBB rmJMP(PPROC pProc, Int marker, PBB pBB)
{
	marker += DFS_JMP;

	while (pBB->nodeType == ONE_BRANCH && pBB->length == 1) {
		if (pBB->traversed != marker) {
			pBB->traversed = marker;
			if (--pBB->numInEdges)
				pBB->edges[0].BBptr->numInEdges++;
			else
			{
				pProc->Icode.SetLlFlag(pBB->start, NO_CODE);
				pProc->Icode.SetLlInvalid(pBB->start, TRUE);
			}

			pBB = pBB->edges[0].BBptr;
		}
		else {			/* We are going around in circles */
			pBB->nodeType = NOWHERE_NODE;
			pProc->Icode.GetIcode(pBB->start)->ic.ll.immed.op = (dword)pBB->start;
			pProc->Icode.SetImmediateOp(pBB->start, (dword)pBB->start);
			do {
				pBB = pBB->edges[0].BBptr;
				if (! --pBB->numInEdges)
				{
					pProc->Icode.SetLlFlag(pBB->start, NO_CODE);
					pProc->Icode.SetLlInvalid(pBB->start, TRUE);
				}
			} while (pBB->nodeType != NOWHERE_NODE);

			free(pBB->edges);
			pBB->numOutEdges = 0;
			pBB->edges       = NULL;
		}
	}
	return pBB;
}


/*****************************************************************************
 * mergeFallThrough
 ****************************************************************************/
static void mergeFallThrough(PPROC pProc, PBB pBB)
{
	PBB	pChild;
	Int	i, ip;

	if (pBB) {
		while (pBB->nodeType == FALL_NODE || pBB->nodeType == ONE_BRANCH) 
		{
			pChild = pBB->edges[0].BBptr;
			/* Jump to next instruction can always be removed */
			if (pBB->nodeType == ONE_BRANCH) 
			{
				ip = pBB->start + pBB->length;
				for (i = ip; i < pChild->start
					&& (pProc->Icode.GetLlFlag(i) & NO_CODE); i++);
				if (i != pChild->start)
					break;
				pProc->Icode.SetLlFlag(ip - 1, NO_CODE);
				pProc->Icode.SetLlInvalid(ip - 1, TRUE);
				pBB->nodeType = FALL_NODE;
				pBB->length--;

			}
			/* If there's no other edges into child can merge */
			if (pChild->numInEdges != 1)
				break;

			pBB->nodeType = pChild->nodeType;
			pBB->length = pChild->start + pChild->length - pBB->start;
			pProc->Icode.ClearLlFlag(pChild->start, TARGET);
			pBB->numOutEdges = pChild->numOutEdges;
			free(pBB->edges);
			pBB->edges = pChild->edges;

			pChild->numOutEdges = pChild->numInEdges = 0;
			pChild->edges = NULL;
		}
		pBB->traversed = DFS_MERGE;

		/* Process all out edges recursively */
		for (i = 0; i < pBB->numOutEdges; i++)
			if (pBB->edges[i].BBptr->traversed != DFS_MERGE)
				mergeFallThrough(pProc, pBB->edges[i].BBptr);
	}
}


/*****************************************************************************
 * dfsNumbering - Numbers nodes during first and last visits and determine 
 * in-edges
 ****************************************************************************/
static void dfsNumbering(PBB pBB, PBB *dfsLast, Int *first, Int *last)
{
	PBB		pChild;
	byte	i;

	if (pBB) 
	{
		pBB->traversed = DFS_NUM;
		pBB->dfsFirstNum = (*first)++;

		/* index is being used as an index to inEdges[]. */
		for (i = 0; i < pBB->numOutEdges; i++) 
		{
			pChild = pBB->edges[i].BBptr;
			pChild->inEdges[pChild->index++] = pBB;

			/* Is this the last visit? */
			if (pChild->index == pChild->numInEdges)
				pChild->index = UN_INIT;

			if (pChild->traversed != DFS_NUM)
				dfsNumbering(pChild, dfsLast, first, last);
		}
		pBB->dfsLastNum = *last;
		dfsLast[(*last)--] = pBB;
	}
}
