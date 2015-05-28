/**************************************************************************
 * File : propLong.c
 * Purpose: propagate the value of long variables (local variables and
 *          registers) along the graph.  Structure the graph in this way.
 * (C) Cristina Cifuentes
 **************************************************************************/

#include "dcc.h"
#include "string.h"
#ifdef __BORLAND__
#include <mem.h>
#else
#include <memory.h>
#endif


static boolT isJCond (llIcode opcode)
/* Returns whether the given icode opcode is within the range of valid
 * high-level conditional jump icodes (iJB..iJG) */
{
	if ((opcode >= iJB) && (opcode <= iJG))
		return (TRUE);
	return (FALSE);
}


static boolT isLong23 (Int i, PBB pbb, PICODE icode, Int *off, Int *arc)
/* Returns whether the conditions for a 2-3 long variable are satisfied */
{ PBB t, e, obb2;

	if (pbb->nodeType != TWO_BRANCH)
		return (FALSE);
	t = pbb->edges[THEN].BBptr;
	e = pbb->edges[ELSE].BBptr;

	/* Check along the THEN path */
	if ((t->length == 1) && (t->nodeType == TWO_BRANCH) && (t->numInEdges == 1))
	{
		obb2 = t->edges[THEN].BBptr;
		if ((obb2->length == 2) && (obb2->nodeType == TWO_BRANCH) &&
			(icode[obb2->start].ic.ll.opcode == iCMP))
		{
			*off = obb2->start - i;
			*arc = THEN;
			return (TRUE);
		}
	}

	/* Check along the ELSE path  */
	else if ((e->length == 1) && (e->nodeType == TWO_BRANCH) && 
			 (e->numInEdges == 1))
	{
		obb2 = e->edges[THEN].BBptr;
		if ((obb2->length == 2) && (obb2->nodeType == TWO_BRANCH) &&
			(icode[obb2->start].ic.ll.opcode == iCMP))
		{
			*off = obb2->start - i;
			*arc = ELSE;
			return (TRUE);
		}
	}
	return (FALSE);
}


static boolT isLong22 (PICODE pIcode, PICODE pEnd, Int *off)
/* Returns whether the conditions for a 2-2 long variable are satisfied */
{
	if (((pIcode+2) < pEnd) && ((pIcode+2)->ic.ll.opcode == iCMP) &&
		(isJCond ((pIcode+1)->ic.ll.opcode)) &&
		(isJCond ((pIcode+3)->ic.ll.opcode)))
	{
		*off = 2;
		return (TRUE);
	}
	return (FALSE);
}


static void longJCond23 (COND_EXPR *rhs, COND_EXPR *lhs, PICODE pIcode, 
						 Int *idx, PPROC pProc, Int arc, Int off)
/* Creates a long conditional <=, >=, <, or > at (pIcode+1).
 * Removes excess nodes from the graph by flagging them, and updates
 * the new edges for the remaining nodes.	*/
{ Int j;
  PBB pbb, obb1, obb2, tbb;

	if (arc == THEN)
  	{
		/* Find intermediate basic blocks and target block */
		pbb = pIcode->inBB;
		obb1 = pbb->edges[THEN].BBptr;
		obb2 = obb1->edges[THEN].BBptr;
		tbb = obb2->edges[THEN].BBptr;

		/* Modify out edge of header basic block */
		pbb->edges[THEN].BBptr = tbb;

		/* Modify in edges of target basic block */
		for (j = 0; j < (tbb->numInEdges-1); j++)
		{
			if ((tbb->inEdges[j] == obb1) || (tbb->inEdges[j] == obb2))
			{
				memmove (&tbb->inEdges[j], &tbb->inEdges[j+1],
					 	(tbb->numInEdges - j-1) * sizeof(PBB));
				memset (&tbb->inEdges[tbb->numInEdges-1], ' ',sizeof (PBB));
				j--;
			}
		}
		tbb->numInEdges--;	/* looses 2 arcs, gains 1 arc */
		tbb->inEdges[tbb->numInEdges-1] = pbb;

		/* Modify in edges of the ELSE basic block */
		tbb = pbb->edges[ELSE].BBptr;
		for (j = 0; j < (tbb->numInEdges-1); j++)
		{
			if (tbb->inEdges[j] == obb2)
			{
				memmove (&tbb->inEdges[j], &tbb->inEdges[j+1],
					 	(tbb->numInEdges - j-1) * sizeof(PBB));
				break;
			}
		}
		tbb->numInEdges--;	/* looses 1 arc */

		/* Update icode index */
		(*idx) += 5; 
  	}

  	else  /* ELSE arc */
  	{
		/* Find intermediate basic blocks and target block */
		pbb = pIcode->inBB;
		obb1 = pbb->edges[ELSE].BBptr;
		obb2 = obb1->edges[THEN].BBptr;
		tbb = obb2->edges[THEN].BBptr;

		/* Modify in edges of target basic block */
		for (j = 0; j < (tbb->numInEdges-1); j++)
		{
			if (tbb->inEdges[j] == obb2)
			{
				memmove (&tbb->inEdges[j], &tbb->inEdges[j+1],
					 	(tbb->numInEdges - j-1) * sizeof(PBB));
				break;	
			}
		}
		tbb->numInEdges--;	/* looses 1 arc */

		/* Modify in edges of the ELSE basic block */
		tbb = obb2->edges[ELSE].BBptr;
		for (j = 0; j < (tbb->numInEdges-1); j++)
		{
			if ((tbb->inEdges[j] == obb2) || (tbb->inEdges[j] == obb1))
			{
				memmove (&tbb->inEdges[j], &tbb->inEdges[j+1],
					 	(tbb->numInEdges - j-1) * sizeof(PBB));
				memset (&tbb->inEdges[tbb->numInEdges-1], ' ',sizeof (PBB));
				j--;
			}
		}
		tbb->numInEdges--;	/* looses 2 arcs, gains 1 arc */
		tbb->inEdges[tbb->numInEdges - 1] = pbb;

		/* Modify out edge of header basic block */
		pbb->edges[ELSE].BBptr = tbb;

		/* Update icode index */
		(*idx) += 2; 
  	}

	/* Create new HLI_JCOND and condition */
	lhs = boolCondExp (lhs, rhs, condOpJCond[(pIcode+off+1)->ic.ll.opcode-iJB]);
	newJCondHlIcode (pIcode+1, lhs);
	copyDU (pIcode+1, pIcode, E_USE, E_USE);
	(pIcode+1)->du.use |= (pIcode+off)->du.use;

	/* Update statistics */
	obb1->flg |= INVALID_BB;
	obb2->flg |= INVALID_BB;
	stats.numBBaft -= 2;

	invalidateIcode (pIcode);
	invalidateIcode (pProc->Icode.GetIcode(obb1->start));
	invalidateIcode (pProc->Icode.GetIcode(obb2->start));
	invalidateIcode (pProc->Icode.GetIcode(obb2->start+1));
}


static void longJCond22 (COND_EXPR *rhs, COND_EXPR *lhs, PICODE pIcode,
						 Int *idx)
/* Creates a long conditional equality or inequality at (pIcode+1).
 * Removes excess nodes from the graph by flagging them, and updates
 * the new edges for the remaining nodes.	*/
{ Int j;
  PBB pbb, obb1, tbb;

	/* Form conditional expression */
	lhs = boolCondExp (lhs, rhs, condOpJCond[(pIcode+3)->ic.ll.opcode - iJB]);
	newJCondHlIcode (pIcode+1, lhs);
	copyDU (pIcode+1, pIcode, E_USE, E_USE);
	(pIcode+1)->du.use |= (pIcode+2)->du.use;

	/* Adjust outEdges[0] to the new target basic block */ 
	pbb = pIcode->inBB;
	if ((pbb->start + pbb->length - 1) == (*idx + 1))
	{
		/* Find intermediate and target basic blocks */
		obb1 = pbb->edges[THEN].BBptr;
		tbb = obb1->edges[THEN].BBptr;

		/* Modify THEN out edge of header basic block */
		pbb->edges[THEN].BBptr = tbb;

		/* Modify in edges of target basic block */
		for (j = 0; j < (tbb->numInEdges-1); j++)
		{
			if (tbb->inEdges[j] == obb1)
			{
				memmove (&tbb->inEdges[j], &tbb->inEdges[j+1],
						(tbb->numInEdges - j-1) * sizeof(PBB));
				break;
			}
		}
		if ((pIcode+3)->ic.ll.opcode == iJE)
			tbb->numInEdges--;	/* looses 1 arc */
		else					/* iJNE => replace arc */
			tbb->inEdges[tbb->numInEdges-1] = pbb;

		/* Modify ELSE out edge of header basic block */
		tbb = obb1->edges[ELSE].BBptr;
		pbb->edges[ELSE].BBptr = tbb;

		/* Modify in edges of the ELSE basic block */
		for (j = 0; j < (tbb->numInEdges-1); j++)
		{
			if (tbb->inEdges[j] == obb1)
			{
				memmove (&tbb->inEdges[j], &tbb->inEdges[j+1],
						(tbb->numInEdges - j-1) * sizeof(PBB));
				break;
			}
		}
		if ((pIcode+3)->ic.ll.opcode == iJE)	/* replace */
			tbb->inEdges[tbb->numInEdges-1] = pbb;	
		else
			tbb->numInEdges--;		/* iJNE => looses 1 arc */


		/* Update statistics */
		obb1->flg |= INVALID_BB;
		stats.numBBaft--;
	}

	invalidateIcode (pIcode);
	invalidateIcode (pIcode+2);
	invalidateIcode (pIcode+3);
	(*idx) += 4;
}


static void propLongStk (Int i, ID *pLocId, PPROC pProc)
/* Propagates TYPE_LONG_(UN)SIGN icode information to the current pIcode
 * Pointer.
 * Arguments: i     : index into the local identifier table
 *            pLocId: ptr to the long local identifier 
 *            pProc : ptr to current procedure's record.        */
{ Int idx, off, arc;
  COND_EXPR *lhs, *rhs;     /* Pointers to left and right hand expression */
  PICODE pIcode, pEnd;

	/* Check all icodes for offHi:offLo */
	pEnd = pProc->Icode.GetIcode(pProc->Icode.GetNumIcodes() -1);
    for (idx = 0; idx < (pProc->Icode.GetNumIcodes() - 1); idx++)
	{
		pIcode = pProc->Icode.GetIcode(idx);
    	if ((pIcode->type == HIGH_LEVEL) || (pIcode->invalid == TRUE))
			continue;

		if (pIcode->ic.ll.opcode == (pIcode+1)->ic.ll.opcode)
		{
			switch (pIcode->ic.ll.opcode) {
			  case iMOV:
            	if (checkLongEq (pLocId->id.longStkId, pIcode, i, idx, pProc, 
                             &rhs, &lhs, 1) == TRUE)
            	{
                	newAsgnHlIcode (pIcode, lhs, rhs);
                	invalidateIcode (pIcode + 1);
                	idx++;
            	}
            	break;

			  case iAND: case iOR: case iXOR:
            	if (checkLongEq (pLocId->id.longStkId, pIcode, i, idx, pProc, 
                             &rhs, &lhs, 1) == TRUE)
            	{
					switch (pIcode->ic.ll.opcode) {
				  	case iAND: 	rhs = boolCondExp (lhs, rhs, AND);
								break;
				  	case iOR: 	rhs = boolCondExp (lhs, rhs, OR);
								break;
				  	case iXOR: 	rhs = boolCondExp (lhs, rhs, XOR);
								break;
					}
                	newAsgnHlIcode (pIcode, lhs, rhs);
                	invalidateIcode (pIcode + 1);
                	idx++;
            	}
            	break;

			  case iPUSH:
            	if (checkLongEq (pLocId->id.longStkId, pIcode, i, idx, pProc,
                             &rhs, &lhs, 1) == TRUE)
            	{
                	newUnaryHlIcode (pIcode, HLI_PUSH, lhs);
                	invalidateIcode (pIcode + 1);
                	idx++;
            	}
            	break;
    		} /*eos*/
		}

		/* Check long conditional (i.e. 2 CMPs and 3 branches */ 
		else if ((pIcode->ic.ll.opcode == iCMP) && 
				 (isLong23 (idx, pIcode->inBB, pProc->Icode.GetFirstIcode(),
				 &off, &arc)))
		{
			if (checkLongEq (pLocId->id.longStkId, pIcode, i, idx, pProc,
							 &rhs, &lhs, off) == TRUE)
				longJCond23 (rhs, lhs, pIcode, &idx, pProc, arc, off);
		}

		/* Check for long conditional equality or inequality.  This requires
		 * 2 CMPs and 2 branches */ 
		else if ((pIcode->ic.ll.opcode == iCMP) && 
										isLong22 (pIcode, pEnd, &off))
		{
			if (checkLongEq (pLocId->id.longStkId, pIcode, i, idx, pProc,
							 &rhs, &lhs, off) == TRUE)
				longJCond22 (rhs, lhs, pIcode, &idx);
		}
	}
}


static void propLongReg (Int i, ID *pLocId, PPROC pProc)
/* Finds the definition of the long register pointed to by pLocId, and 
 * transforms that instruction into a HIGH_LEVEL icode instruction.
 * Arguments: i     : index into the local identifier table
 *            pLocId: ptr to the long local identifier 
 *            pProc : ptr to current procedure's record.        */
{ COND_EXPR *lhs, *rhs; 
  Int idx, j, off, arc;
  PICODE pIcode, pEnd;
  PMEM pmH, pmL;            /* Pointers to dst LOW_LEVEL icodes */

  /* Process all definitions/uses of long registers at an icode position */
  pEnd = pProc->Icode.GetIcode(pProc->Icode.GetNumIcodes() -1);
  for (j = 0; j < pLocId->idx.csym; j++)
  {
    /* Check backwards for a definition of this long register */
    for (idx = pLocId->idx.idx[j] - 1; idx > 0 ; idx--)
    {
      pIcode = pProc->Icode.GetIcode(idx-1);
	  if ((pIcode->type == HIGH_LEVEL) || (pIcode->invalid == TRUE))
			continue;

      if (pIcode->ic.ll.opcode == (pIcode+1)->ic.ll.opcode)
        switch (pIcode->ic.ll.opcode) {
          case iMOV:
            pmH = &pIcode->ic.ll.dst;
            pmL = &(pIcode+1)->ic.ll.dst;
            if ((pLocId->id.longId.h == pmH->regi) && 
                (pLocId->id.longId.l == pmL->regi))
            {
                lhs = idCondExpLongIdx (i);
                insertIdx (&pProc->localId.id[i].idx, idx-1);
                setRegDU (pIcode, pmL->regi, E_DEF);
                rhs = idCondExpLong (&pProc->localId, SRC, pIcode, HIGH_FIRST, 
                                     idx, E_USE, 1);
                newAsgnHlIcode (pIcode, lhs, rhs);
                invalidateIcode (pIcode + 1);
                idx = 0;    /* to exit the loop */
            }
            break;

          case iPOP:
            pmH = &(pIcode+1)->ic.ll.dst;
            pmL = &pIcode->ic.ll.dst;
            if ((pLocId->id.longId.h == pmH->regi) &&
                (pLocId->id.longId.l == pmL->regi))
            {
                lhs = idCondExpLongIdx (i);
                setRegDU (pIcode, pmH->regi, E_DEF);
                newUnaryHlIcode (pIcode, HLI_POP, lhs);
                invalidateIcode (pIcode + 1);
                idx = 0;        /* to exit the loop */
            }
            break;
	
		  /**** others missing ***/

		  case iAND: case iOR: case iXOR:
            pmL = &pIcode->ic.ll.dst;
            pmH = &(pIcode+1)->ic.ll.dst;
            if ((pLocId->id.longId.h == pmH->regi) &&
                (pLocId->id.longId.l == pmL->regi))
            {
                lhs = idCondExpLongIdx (i);
                setRegDU (pIcode, pmH->regi, USE_DEF);
                rhs = idCondExpLong (&pProc->localId, SRC, pIcode, LOW_FIRST, 
                                     idx, E_USE, 1);
				switch (pIcode->ic.ll.opcode) {
				  case iAND: rhs = boolCondExp (lhs, rhs, AND);
							 break;
				  case iOR:	 rhs = boolCondExp (lhs, rhs, OR);
							 break;
				  case iXOR: rhs = boolCondExp (lhs, rhs, XOR);
							 break;
				} /* eos */
                newAsgnHlIcode (pIcode, lhs, rhs);
                invalidateIcode (pIcode + 1);
                idx = 0;
            }
            break;
        } /* eos */
    }

    /* If no definition backwards, check forward for a use of this long reg */
    if (idx <= 0)
      for (idx = pLocId->idx.idx[j] + 1; idx < pProc->Icode.GetNumIcodes() - 1; idx++)
	  {
        pIcode = pProc->Icode.GetIcode(idx);
		if ((pIcode->type == HIGH_LEVEL) || (pIcode->invalid == TRUE))
			continue;

        if (pIcode->ic.ll.opcode == (pIcode+1)->ic.ll.opcode)
            switch (pIcode->ic.ll.opcode) {
              case iMOV:
                if ((pLocId->id.longId.h == pIcode->ic.ll.src.regi) &&
                    (pLocId->id.longId.l == (pIcode+1)->ic.ll.src.regi))
                {
                    rhs = idCondExpLongIdx (i);
                    setRegDU (pIcode, (pIcode+1)->ic.ll.src.regi, E_USE); 
                    lhs = idCondExpLong (&pProc->localId, DST, pIcode, 
                                         HIGH_FIRST, idx, E_DEF, 1);
                    newAsgnHlIcode (pIcode, lhs, rhs);
                    invalidateIcode (pIcode + 1);
                    idx = pProc->Icode.GetNumIcodes();    /* to exit the loop */ 
                }
                break;

              case iPUSH:
                if ((pLocId->id.longId.h == pIcode->ic.ll.src.regi) &&
                    (pLocId->id.longId.l == (pIcode+1)->ic.ll.src.regi))
                {
                    rhs = idCondExpLongIdx (i);
                    setRegDU (pIcode, (pIcode+1)->ic.ll.src.regi, E_USE); 
                    newUnaryHlIcode (pIcode, HLI_PUSH, lhs);
                    invalidateIcode (pIcode + 1);
                }
                idx = pProc->Icode.GetNumIcodes();    /* to exit the loop  */
                break;

			  /*** others missing ****/

          	  case iAND: case iOR: case iXOR:
            	pmL = &pIcode->ic.ll.dst;
            	pmH = &(pIcode+1)->ic.ll.dst;
            	if ((pLocId->id.longId.h == pmH->regi) &&
                	(pLocId->id.longId.l == pmL->regi))
            	{
                	lhs = idCondExpLongIdx (i);
                	setRegDU (pIcode, pmH->regi, USE_DEF);
                	rhs = idCondExpLong (&pProc->localId, SRC, pIcode, 
										 LOW_FIRST, idx, E_USE, 1);
					switch (pIcode->ic.ll.opcode) {
                	  case iAND: rhs = boolCondExp (lhs, rhs, AND);
								 break;
                	  case iOR:  rhs = boolCondExp (lhs, rhs, OR);
								 break;
                	  case iXOR: rhs = boolCondExp (lhs, rhs, XOR);
								 break;
					}
                	newAsgnHlIcode (pIcode, lhs, rhs);
                	invalidateIcode (pIcode + 1);
                	idx = 0;
            	}
            	break;
            } /* eos */

		/* Check long conditional (i.e. 2 CMPs and 3 branches */ 
		else if ((pIcode->ic.ll.opcode == iCMP) && 
				 (isLong23 (idx, pIcode->inBB, pProc->Icode.GetFirstIcode(),
				  &off, &arc)))
		{
			if (checkLongRegEq (pLocId->id.longId, pIcode, i, idx, pProc,
							    &rhs, &lhs, off) == TRUE)
				longJCond23 (rhs, lhs, pIcode, &idx, pProc, arc, off);
		}

		/* Check for long conditional equality or inequality.  This requires
		 * 2 CMPs and 2 branches */ 
		else if ((pIcode->ic.ll.opcode == iCMP) && 
											(isLong22 (pIcode, pEnd, &off)))
		{
			if (checkLongRegEq (pLocId->id.longId, pIcode, i, idx, pProc,
							    &rhs, &lhs, off) == TRUE)
				longJCond22 (rhs, lhs, pIcode, &idx);
		}
		
		/* Check for OR regH, regL
		 *			 JX lab	
		 *		=> HLI_JCOND (regH:regL X 0) lab
		 * This is better code than HLI_JCOND (HI(regH:regL) | LO(regH:regL)) */
		else if ((pIcode->ic.ll.opcode == iOR) && ((pIcode+1) < pEnd) &&
				 (isJCond ((pIcode+1)->ic.ll.opcode)))
		{
			if ((pIcode->ic.ll.dst.regi == pLocId->id.longId.h) &&
				(pIcode->ic.ll.src.regi == pLocId->id.longId.l))
			{
				lhs = idCondExpLongIdx (i);
				rhs = idCondExpKte (0, 4);	/* long 0 */
				lhs = boolCondExp (lhs, rhs, 
								   condOpJCond[(pIcode+1)->ic.ll.opcode - iJB]);
				newJCondHlIcode (pIcode+1, lhs);
				copyDU (pIcode+1, pIcode, E_USE, E_USE);
				invalidateIcode (pIcode);
			}
		}

	  } /* end for */
  } /* end for */
}


static void propLongGlb (Int i, ID *pLocId, PPROC pProc)
/* Propagates the long global address across all LOW_LEVEL icodes. 
 * Transforms some LOW_LEVEL icodes into HIGH_LEVEL     */
{

}


void propLong (PPROC pProc)
/* Propagated identifier information, thus converting some LOW_LEVEL icodes
 * into HIGH_LEVEL icodes.  */
{ Int i;
  ID *pLocId;           /* Pointer to current local identifier */

    for (i = 0; i < pProc->localId.csym; i++)
    {
		pLocId = &pProc->localId.id[i];
        if ((pLocId->type==TYPE_LONG_SIGN) || (pLocId->type==TYPE_LONG_UNSIGN))
        {
            switch (pLocId->loc) {
			case STK_FRAME:	propLongStk (i, pLocId, pProc);
                			break;
            case REG_FRAME: propLongReg (i, pLocId, pProc);
                			break;
            case GLB_FRAME: propLongGlb (i, pLocId, pProc);
                			break; 
            }
        }
    }
}

