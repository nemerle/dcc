/*****************************************************************************
 * Project: dcc
 * File:    dataflow.c
 * Purpose: Data flow analysis module.
 * (C) Cristina Cifuentes
 ****************************************************************************/

#include "dcc.h"
#include <string.h>
#include <stdio.h>


static Int getLocVar (PSTKFRAME pStkFrame, Int off)
/* Returns the index of the local variable or parameter at offset off, if it 
 * is in the stack frame provided.  */
{   Int     i;

    for (i = 0; i < pStkFrame->csym; i++)
       if (pStkFrame->sym[i].off == off) 
          break;
    return (i);
}


static COND_EXPR *srcIdent (PICODE Icode, PPROC pProc, Int i, PICODE duIcode, 
                            operDu du)
/* Returns a string with the source operand of Icode */
{ COND_EXPR *n;

    if (Icode->ic.ll.flg & I)   /* immediate operand */
	{
		if (Icode->ic.ll.flg & B)
        	n = idCondExpKte (Icode->ic.ll.immed.op, 1);
		else
        	n = idCondExpKte (Icode->ic.ll.immed.op, 2);
	}
    else
       n = idCondExp (Icode, SRC, pProc, i, duIcode, du);
    return (n);
}


static COND_EXPR *dstIdent (PICODE pIcode, PPROC pProc, Int i, PICODE duIcode,
                            operDu du) 
/* Returns the destination operand */
{ COND_EXPR *n; 

    n = idCondExp (pIcode, DST, pProc, i, duIcode, du);
            /** Is it needed? (pIcode->ic.ll.flg) & NO_SRC_B **/
    return (n);
}



static void elimCondCodes (PPROC pProc)
/* Eliminates all condition codes and generates new hlIcode instructions */
{ Int i, 
      useAt,            /* Index to instruction that used flag    */
      defAt;            /* Index to instruction that defined flag */
  byte use;             /* Used flags bit vector                  */
  byte def;             /* Defined flags bit vector               */
  boolT notSup;         /* Use/def combination not supported      */
  COND_EXPR *rhs;       /* Source operand                         */
  COND_EXPR *lhs;       /* Destination operand                    */
  COND_EXPR *exp;       /* Boolean expression                     */
  PBB pBB;              /* Pointer to BBs in dfs last ordering    */
  ICODE *prev;          /* For extended basic blocks - previous icode inst */

  for (i = 0; i < pProc->numBBs; i++) 
  {
    pBB = pProc->dfsLast[i];
	if (pBB->flg & INVALID_BB)	continue; /* Do not process invalid BBs */

    for (useAt = pBB->start + pBB->length; useAt != pBB->start; useAt--)
        if ((pProc->Icode.GetIcode(useAt-1)->type == LOW_LEVEL) &&
            (pProc->Icode.GetIcode(useAt-1)->invalid == FALSE) &&
            (use = pProc->Icode.GetIcode(useAt-1)->ic.ll.flagDU.u))
        {
            /* Find definition within the same basic block */
            for (defAt = useAt-1; defAt != pBB->start; defAt--)
            {
                def = pProc->Icode.GetIcode(defAt-1)->ic.ll.flagDU.d;
                if ((use & def) == use)
                {
                    notSup = FALSE;
                    if ((pProc->Icode.GetLlOpcode(useAt-1) >= iJB) &&
                        (pProc->Icode.GetLlOpcode(useAt-1) <= iJNS))
                    {
                        switch (pProc->Icode.GetLlOpcode(defAt-1))
						{
                          case iCMP:
                            rhs = srcIdent (pProc->Icode.GetIcode(defAt-1),
                                            pProc, defAt-1, 
                                            pProc->Icode.GetIcode(useAt-1), E_USE);
                            lhs = dstIdent (pProc->Icode.GetIcode(defAt-1),
                                            pProc, defAt-1, 
                                            pProc->Icode.GetIcode(useAt-1), E_USE);
                            break;

                          case iOR:
                            lhs = copyCondExp (pProc->Icode.GetIcode(defAt-1)->ic.hl.oper.asgn.lhs);
                            copyDU (pProc->Icode.GetIcode(useAt-1),
                                    pProc->Icode.GetIcode(defAt-1), E_USE, E_DEF);
							if (pProc->Icode.GetLlFlag(defAt-1) & B)
                            	rhs = idCondExpKte (0, 1); 
							else
                            	rhs = idCondExpKte (0, 2); 
                            break;

                          case iTEST:
                            rhs = srcIdent (pProc->Icode.GetIcode(defAt-1),
                                            pProc, defAt-1, 
                                            pProc->Icode.GetIcode(useAt-1), E_USE);
                            lhs = dstIdent (pProc->Icode.GetIcode(defAt-1), 
                                            pProc, defAt-1, 
                                            pProc->Icode.GetIcode(useAt-1), E_USE);
                            lhs = boolCondExp (lhs, rhs, AND);
							if (pProc->Icode.GetLlFlag(defAt-1) & B)
                            	rhs = idCondExpKte (0, 1); 
							else
                            	rhs = idCondExpKte (0, 2); 
                            break;

                          default: 
                            notSup = TRUE;
							reportError (JX_NOT_DEF, pProc->Icode.GetLlOpcode(defAt-1));
							pProc->flg |= PROC_ASM;		/* generate asm */
                        }
                        if (! notSup)
                        {
                            exp = boolCondExp (lhs, rhs,
                              condOpJCond[pProc->Icode.GetLlOpcode(useAt-1)-iJB]);
                            newJCondHlIcode (pProc->Icode.GetIcode(useAt-1), exp);
                        }
                    }
                    
                    else if (pProc->Icode.GetLlOpcode(useAt-1) == iJCXZ) 
                    {
                        lhs = idCondExpReg (rCX, 0, &pProc->localId);
                        setRegDU (pProc->Icode.GetIcode(useAt-1), rCX, E_USE);
                        rhs = idCondExpKte (0, 2);
                        exp = boolCondExp (lhs, rhs, EQUAL);
                        newJCondHlIcode (pProc->Icode.GetIcode(useAt-1), exp);
                    }

                    else
					{
						reportError (NOT_DEF_USE, 
                            pProc->Icode.GetLlOpcode(defAt-1),
                            pProc->Icode.GetLlOpcode(useAt-1));
						pProc->flg |= PROC_ASM;		/* generate asm */
					}
                    break;
                }
            }

            /* Check for extended basic block */
            if ((pBB->length == 1) && 
                (pProc->Icode.GetLlOpcode(useAt-1) >= iJB) &&
                (pProc->Icode.GetLlOpcode(useAt-1) <= iJNS))
            {
                prev = pProc->Icode.GetIcode(pBB->inEdges[0]->start + 
                                            pBB->inEdges[0]->length - 1);
                if (prev->ic.hl.opcode == HLI_JCOND)
                {
                    exp = copyCondExp (prev->ic.hl.oper.exp);
                    changeBoolCondExpOp (exp, 
                        condOpJCond[pProc->Icode.GetLlOpcode(useAt-1)-iJB]);
                    copyDU (pProc->Icode.GetIcode(useAt-1), prev, E_USE, E_USE);
                    newJCondHlIcode (pProc->Icode.GetIcode(useAt-1), exp);
                }
            }
            /* Error - definition not found for use of a cond code */
            else if (defAt == pBB->start)
                fatalError (DEF_NOT_FOUND, 
                            pProc->Icode.GetLlOpcode(useAt-1));
        }
  }
}


static void genLiveKtes (PPROC pproc)
/* Generates the LiveUse() and Def() sets for each basic block in the graph.
 * Note: these sets are constant and could have been constructed during
 *       the construction of the graph, but since the code hasn't been 
 *       analyzed yet for idioms, the procedure preamble misleads the
 *       analysis (eg: push si, would include si in LiveUse; although it
 *       is not really meant to be a register that is used before defined). */
{ Int i, j;
  PBB pbb;
  PICODE picode;
  dword liveUse, def;

    for (i = 0; i < pproc->numBBs; i++)
    {
        liveUse = def = 0;
        pbb = pproc->dfsLast[i];
		if (pbb->flg & INVALID_BB)	continue;	/* skip invalid BBs */
        for (j = pbb->start; j < (pbb->start + pbb->length); j++)
        {
            picode = pproc->Icode.GetIcode(j);
            if ((picode->type == HIGH_LEVEL) && (picode->invalid == FALSE))
            {
                liveUse |= (picode->du.use & ~def); 
                def |= picode->du.def;
            }
        }
        pbb->liveUse = liveUse;
        pbb->def = def;
    }
}


static void liveRegAnalysis (PPROC pproc, dword liveOut)
/* Generates the liveIn() and liveOut() sets for each basic block via an
 * iterative approach.  
 * Propagates register usage information to the procedure call. */
{ Int i, j;
  PBB pbb;              /* pointer to current basic block   */
  PPROC pcallee;        /* invoked subroutine               */
  PICODE ticode,        /* icode that invokes a subroutine  */
		 picode;		/* icode of function return			*/
  dword prevLiveOut,	/* previous live out 				*/
		prevLiveIn;		/* previous live in					*/
  boolT change;			/* is there change in the live sets?*/

    /* liveOut for this procedure */
    pproc->liveOut = liveOut;

  change = TRUE;
  while (change)
  {
    /* Process nodes in reverse postorder order */
	change = FALSE;
    for (i = pproc->numBBs; i > 0; i--)
    {
        pbb = pproc->dfsLast[i-1];
		if (pbb->flg & INVALID_BB)		/* Do not process invalid BBs */
			continue;

		/* Get current liveIn() and liveOut() sets */
		prevLiveIn = pbb->liveIn;
		prevLiveOut = pbb->liveOut;

        /* liveOut(b) = U LiveIn(s); where s is successor(b) 
         * liveOut(b) = {liveOut}; when b is a HLI_RET node     */
        if (pbb->numOutEdges == 0)      /* HLI_RET node         */
		{
            pbb->liveOut = liveOut;

			/* Get return expression of function */
			if (pproc->flg & PROC_IS_FUNC)
			{
				picode = pproc->Icode.GetIcode(pbb->start + pbb->length - 1);
				if (picode->ic.hl.opcode == HLI_RET)
				{
					picode->ic.hl.oper.exp = idCondExpID (&pproc->retVal, 
							&pproc->localId, pbb->start + pbb->length - 1);
					picode->du.use = liveOut;
				}
			}
		}
        else                            /* Check successors */
        {
            for (j = 0; j < pbb->numOutEdges; j++)
                pbb->liveOut |= pbb->edges[j].BBptr->liveIn;

            /* propagate to invoked procedure */
            if (pbb->nodeType == CALL_NODE)
            {
                ticode = pproc->Icode.GetIcode(pbb->start + pbb->length - 1);
                pcallee = ticode->ic.hl.oper.call.proc;

                /* user/runtime routine */
                if (! (pcallee->flg & PROC_ISLIB))
                {
                    if (pcallee->liveAnal == FALSE) /* hasn't been processed */
                        dataFlow (pcallee, pbb->liveOut);
                    pbb->liveOut = pcallee->liveIn;
                }
                else    /* library routine */
				{
					if ((pcallee->flg & PROC_IS_FUNC) && /* returns a value */
						(pcallee->liveOut & pbb->edges[0].BBptr->liveIn)) 
						pbb->liveOut = pcallee->liveOut;
					else
                    	pbb->liveOut = 0;
				} 

				if ((! (pcallee->flg & PROC_ISLIB)) || (pbb->liveOut != 0))
				{
					switch (pcallee->retVal.type) {
				  	case TYPE_LONG_SIGN: case TYPE_LONG_UNSIGN:
				  		ticode->du1.numRegsDef = 2;
						break;
				  	case TYPE_WORD_SIGN: case TYPE_WORD_UNSIGN:
				  	case TYPE_BYTE_SIGN: case TYPE_BYTE_UNSIGN:
                    	ticode->du1.numRegsDef = 1;
						break;
               	 	} /*eos*/

                	/* Propagate def/use results to calling icode */
                	ticode->du.use = pcallee->liveIn;
                	ticode->du.def = pcallee->liveOut;
				}
            }
        }

        /* liveIn(b) = liveUse(b) U (liveOut(b) - def(b) */
        pbb->liveIn = pbb->liveUse | (pbb->liveOut & ~pbb->def);

		/* Check if live sets have been modified */
		if ((prevLiveIn != pbb->liveIn) || (prevLiveOut != pbb->liveOut))
			change = TRUE;
    }
  }

    /* Propagate liveIn(b) to procedure header */
    if (pbb->liveIn != 0)   /* uses registers */
        pproc->liveIn = pbb->liveIn;

	/* Remove any references to register variables */
	if (pproc->flg & SI_REGVAR)
	{
		pproc->liveIn &= maskDuReg[rSI];
		pbb->liveIn &= maskDuReg[rSI];
	}
	if (pproc->flg & DI_REGVAR)
	{
		pproc->liveIn &= maskDuReg[rDI];
		pbb->liveIn &= maskDuReg[rDI];
	}
}


static void genDU1 (PPROC pProc)
/* Generates the du chain of each instruction in a basic block */
{ byte regi;            /* Register that was defined */
  Int i, j, k, p, n, lastInst, defRegIdx, useIdx;
  PICODE picode, ticode;/* Current and target bb    */
  PBB pbb, tbb;         /* Current and target basic block */
  boolT res;
  COND_EXPR *exp, *lhs;

    /* Traverse tree in dfsLast order */
    for (i = 0; i < pProc->numBBs; i++)
    {
        pbb = pProc->dfsLast[i];
		if (pbb->flg & INVALID_BB)	continue;

        /* Process each register definition of a HIGH_LEVEL icode instruction.  
         * Note that register variables should not be considered registers.  
         */
        lastInst = pbb->start + pbb->length;
        for (j = pbb->start; j < lastInst; j++)
        {
            picode = pProc->Icode.GetIcode(j);
            if (picode->type == HIGH_LEVEL)
            {
              regi = 0;
              defRegIdx = 0;
              for (k = 0; k < INDEXBASE; k++)
              {
                if ((picode->du.def & power2(k)) != 0)
                {
                    regi = (byte)(k + 1);       /* defined register */
					picode->du1.regi[defRegIdx] = regi;

                    /* Check remaining instructions of the BB for all uses
                     * of register regi, before any definitions of the
                     * register */
                    if ((regi == rDI) && (pProc->flg & DI_REGVAR))
                        continue;
                    if ((regi == rSI) && (pProc->flg & SI_REGVAR))
                        continue;
                    if ((j + 1) < lastInst)		/* several instructions */
                    {
                        useIdx = 0;
                        for (n = j+1; n < lastInst; n++)
                        {
                            /* Only check uses of HIGH_LEVEL icodes */
                            ticode = pProc->Icode.GetIcode(n);
                            if (ticode->type == HIGH_LEVEL) 
                            {
                                /* if used, get icode index */
                                if (ticode->du.use & duReg[regi])
                                    picode->du1.idx[defRegIdx][useIdx++] = n;

                                /* if defined, stop finding uses for this reg */
                                if (ticode->du.def & duReg[regi])
                                    break;
                            }
                        }

						/* Check if last definition of this register */
						if ((! (ticode->du.def & duReg[regi])) &&
                      		(pbb->liveOut & duReg[regi]))
							picode->du.lastDefRegi |= duReg[regi]; 
                    }
					else		/* only 1 instruction in this basic block */
					{
						/* Check if last definition of this register */
                      	if (pbb->liveOut & duReg[regi])
							picode->du.lastDefRegi |= duReg[regi]; 
					}

					/* Find target icode for HLI_CALL icodes to procedures
					 * that are functions.  The target icode is in the
					 * next basic block (unoptimized code) or somewhere else
					 * on optimized code. */
					if ((picode->ic.hl.opcode == HLI_CALL) && 
						(picode->ic.hl.oper.call.proc->flg & PROC_IS_FUNC))
					{
						tbb = pbb->edges[0].BBptr;
						useIdx = 0;
						for (n = tbb->start; n < tbb->start + tbb->length; n++)
						{
							ticode = pProc->Icode.GetIcode(n);
							if (ticode->type == HIGH_LEVEL)
							{
                                /* if used, get icode index */
                                if (ticode->du.use & duReg[regi])
                                    picode->du1.idx[defRegIdx][useIdx++] = n;

                                /* if defined, stop finding uses for this reg */
                                if (ticode->du.def & duReg[regi])
                                    break;
							}
						}

						/* if not used in this basic block, check if the
						 * register is live out, if so, make it the last
						 * definition of this register */
						if ((picode->du1.idx[defRegIdx][useIdx] == 0) &&
							(tbb->liveOut & duReg[regi]))
							picode->du.lastDefRegi |= duReg[regi];
					}

                    /* If not used within this bb or in successors of this
                     * bb (ie. not in liveOut), then register is useless, 
                     * thus remove it.  Also check that this is not a return
					 * from a library function (routines such as printf 
					 * return an integer, which is normally not taken into
					 * account by the programmer). 	*/
                    if ((picode->invalid == FALSE) &&
						(picode->du1.idx[defRegIdx][0] == 0) &&
						(! (picode->du.lastDefRegi & duReg[regi])) &&
//						(! ((picode->ic.hl.opcode != HLI_CALL) &&
						(! ((picode->ic.hl.opcode == HLI_CALL) &&
							(picode->ic.hl.oper.call.proc->flg & PROC_ISLIB)))) 
					{
                      if (! (pbb->liveOut & duReg[regi]))	/* not liveOut */
                      {
                        res = removeDefRegi (regi, picode, defRegIdx+1,
                                             &pProc->localId); 

                        /* Backpatch any uses of this instruction, within
                         * the same BB, if the instruction was invalidated */
                        if (res == TRUE) 
                            for (p = j; p > pbb->start; p--)
                            {
                                ticode = pProc->Icode.GetIcode(p-1);
                                for (n = 0; n < MAX_USES; n++) 
                                {
                                    if (ticode->du1.idx[0][n] == j)
                                    {
                                        if (n < MAX_USES - 1)
                                        {
                                            memmove (&ticode->du1.idx[0][n],
                                             &ticode->du1.idx[0][n+1], 
                                             (size_t)((MAX_USES - n - 1) * sizeof(Int)));
                                            n--;
                                        }
                                        ticode->du1.idx[0][MAX_USES - 1] = 0;
                                    }
                                }
                            }
                      }
					  else		/* liveOut */
						picode->du.lastDefRegi |= duReg[regi];
					}
                    defRegIdx++;

                    /* Check if all defined registers have been processed */
                    if ((defRegIdx >= picode->du1.numRegsDef) ||
                        (defRegIdx == MAX_REGS_DEF))
                        break;
                }
              }
            }   
        }
    }

}


static void forwardSubs (COND_EXPR *lhs, COND_EXPR *rhs, PICODE picode, 
                         PICODE ticode, LOCAL_ID *locsym, Int *numHlIcodes)
/* Substitutes the rhs (or lhs if rhs not possible) of ticode for the rhs
 * of picode. */
{ boolT res;

    if (rhs == NULL)        /* In case expression popped is NULL */
        return;

    /* Insert on rhs of ticode, if possible */
    res = insertSubTreeReg (rhs, &ticode->ic.hl.oper.asgn.rhs, 
                            locsym->id[lhs->expr.ident.idNode.regiIdx].id.regi,
                            locsym);
    if (res)
	{
        invalidateIcode (picode);   
		(*numHlIcodes)--;
	}
    else
    {
        /* Try to insert it on lhs of ticode*/
        res = insertSubTreeReg (rhs, &ticode->ic.hl.oper.asgn.lhs,
                            locsym->id[lhs->expr.ident.idNode.regiIdx].id.regi,
                            locsym);
        if (res)
		{
            invalidateIcode (picode);
			(*numHlIcodes)--;
		}
    }
}


static void forwardSubsLong (Int longIdx, COND_EXPR *exp, PICODE picode,
							 PICODE ticode, Int *numHlIcodes) 
/* Substitutes the rhs (or lhs if rhs not possible) of ticode for the 
 * expression exp given */
{ boolT res;

    if (exp == NULL)        /* In case expression popped is NULL */
        return;

    /* Insert on rhs of ticode, if possible */
    res = insertSubTreeLongReg (exp, &ticode->ic.hl.oper.asgn.rhs, longIdx);
    if (res)
	{
        invalidateIcode (picode);   
		(*numHlIcodes)--;
	}
    else
    {
        /* Try to insert it on lhs of ticode*/
        res = insertSubTreeLongReg (exp, &ticode->ic.hl.oper.asgn.lhs, longIdx);
        if (res)
		{
            invalidateIcode (picode);
			(*numHlIcodes)--;
		}
    }
}


static boolT xClear (COND_EXPR *rhs, Int f, Int t, Int lastBBinst, PPROC pproc)
/* Returns whether the elements of the expression rhs are all x-clear from
 * instruction f up to instruction t.	*/
{ Int i;
  boolT res;
  byte regi;
  PICODE picode;
 
	if (rhs == NULL)
		return (FALSE);

	switch (rhs->type) {
	  case IDENTIFIER:
			if (rhs->expr.ident.idType == REGISTER)
			{
				picode = pproc->Icode.GetFirstIcode();
				regi= pproc->localId.id[rhs->expr.ident.idNode.regiIdx].id.regi;
				for (i = (f + 1); (i < lastBBinst) && (i < t); i++)
					if ((picode[i].type == HIGH_LEVEL) &&
						(picode[i].invalid == FALSE))
					{
						if (picode[i].du.def & duReg[regi])
							return (FALSE);
					}
				if (i < lastBBinst)
					return (TRUE);
				else
					return (FALSE);	
			}
			else
				return (TRUE);
			/* else if (rhs->expr.ident.idType == LONG_VAR)
			{
missing all other identifiers ****
			} */

	  case BOOLEAN_OP:
			res = xClear (rhs->expr.boolExpr.rhs, f, t, lastBBinst, pproc);
			if (res == FALSE)
				return (FALSE);
			return (xClear (rhs->expr.boolExpr.lhs, f, t, lastBBinst, pproc));

	  case NEGATION:
	  case ADDRESSOF:
	  case DEREFERENCE:
			return (xClear (rhs->expr.unaryExp, f, t, lastBBinst, pproc));
	} /* eos */
	return FALSE;
}


static void processCArg (PPROC pp, PPROC pProc, PICODE picode, Int numArgs,
						 Int *k)
/* Checks the type of the formal argument as against to the actual argument,
 * whenever possible, and then places the actual argument on the procedure's
 * argument list.	*/
{ COND_EXPR *exp;
  boolT res;

	/* if (numArgs == 0)
		return; */

	exp = popExpStk();
	if (pp->flg & PROC_ISLIB) /* library function */
	{
	  if (pp->args.numArgs > 0)
		if (pp->flg & PROC_VARARG) 
		{
			if (numArgs < pp->args.csym)
				adjustActArgType (exp, pp->args.sym[numArgs].type, pProc);
		}
		else
			adjustActArgType (exp, pp->args.sym[numArgs].type, pProc);
	  res = newStkArg (picode, exp, picode->ic.ll.opcode, pProc);
	}
	else			/* user function */
	{
		if (pp->args.numArgs > 0)
			adjustForArgType (&pp->args, numArgs, expType (exp, pProc));
		res = newStkArg (picode, exp, picode->ic.ll.opcode, pProc);
	}

	/* Do not update the size of k if the expression was a segment register
	 * in a near call */
	if (res == FALSE)
		*k += hlTypeSize (exp, pProc);
}


static void findExps (PPROC pProc)
/* Eliminates extraneous intermediate icode instructions when finding
 * expressions.  Generates new hlIcodes in the form of expression trees. 
 * For HLI_CALL hlIcodes, places the arguments in the argument list.    */
{ Int i, j, k, lastInst, lastInstN, numHlIcodes;
  PICODE picode,        /* Current icode                            */
         ticode;        /* Target icode                             */
  PBB pbb, nbb;         /* Current and next basic block             */
  boolT res;
  COND_EXPR *exp,       /* expression pointer - for HLI_POP and HLI_CALL    */
			*lhs;		/* exp ptr for return value of a HLI_CALL		*/
  PSTKFRAME args;       /* pointer to arguments - for HLI_CALL          */
  byte regi, regi2;		/* register(s) to be forward substituted	*/
  ID *retVal;			/* function return value 					*/

	/* Initialize expression stack */
    initExpStk();

    /* Traverse tree in dfsLast order */
    for (i = 0; i < pProc->numBBs; i++)
    {
        /* Process one BB */
        pbb = pProc->dfsLast[i];
		if (pbb->flg & INVALID_BB) 	continue;
        lastInst = pbb->start + pbb->length;
		numHlIcodes = 0;
        for (j = pbb->start; j < lastInst; j++)
        {
            picode = pProc->Icode.GetIcode(j);
            if ((picode->type == HIGH_LEVEL) && (picode->invalid == FALSE))
            {
				numHlIcodes++;
                if (picode->du1.numRegsDef == 1)    /* byte/word regs */ 
                {
                    /* Check for only one use of this register.  If this is 
					 * the last definition of the register in this BB, check
					 * that it is not liveOut from this basic block */
                    if ((picode->du1.idx[0][0] != 0) && 
                        (picode->du1.idx[0][1] == 0)) 
                    {
						/* Check that this register is not liveOut, if it
						 * is the last definition of the register */
						regi = picode->du1.regi[0];

						/* Check if we can forward substitute this register */
                        switch (picode->ic.hl.opcode) {
                        case HLI_ASSIGN:    
                            /* Replace rhs of current icode into target
                             * icode expression */
                            ticode = pProc->Icode.GetIcode(picode->du1.idx[0][0]);
							if ((picode->du.lastDefRegi & duReg[regi]) &&
								((ticode->ic.hl.opcode != HLI_CALL) &&
								 (ticode->ic.hl.opcode != HLI_RET)))
								continue;

							if (xClear (picode->ic.hl.oper.asgn.rhs, j,
									picode->du1.idx[0][0],  lastInst, pProc)) 
							{
                              switch (ticode->ic.hl.opcode) {
							  case HLI_ASSIGN:
                                    forwardSubs (picode->ic.hl.oper.asgn.lhs,
                                            picode->ic.hl.oper.asgn.rhs,
                                            picode, ticode, &pProc->localId,
											&numHlIcodes);
                                    break;

                              case HLI_JCOND: case HLI_PUSH: case HLI_RET:
                                res = insertSubTreeReg (
                                    picode->ic.hl.oper.asgn.rhs,
                                    &ticode->ic.hl.oper.exp,
                                    pProc->localId.id[picode->ic.hl.oper.asgn.lhs->expr.ident.idNode.regiIdx].id.regi,
                                    &pProc->localId);
                                if (res)
								{
                                    invalidateIcode (picode);
									numHlIcodes--;
								}
                                break;

                              case HLI_CALL:    /* register arguments */
                                newRegArg (pProc, picode, ticode);
								invalidateIcode (picode);
								numHlIcodes--;
                                break;
                              } /* eos */
							}
                            break;

                        case HLI_POP:
                            ticode = pProc->Icode.GetIcode(picode->du1.idx[0][0]);
							if ((picode->du.lastDefRegi & duReg[regi]) &&
								((ticode->ic.hl.opcode != HLI_CALL) &&
								 (ticode->ic.hl.opcode != HLI_RET)))
								continue;

                            exp = popExpStk();  /* pop last exp pushed */
                            switch (ticode->ic.hl.opcode) {
                              case HLI_ASSIGN:
                                forwardSubs (picode->ic.hl.oper.exp, exp, 
                                             picode, ticode, &pProc->localId,
											 &numHlIcodes); 
                                break;

                              case HLI_JCOND: case HLI_PUSH: case HLI_RET:
                                res = insertSubTreeReg (exp,
                                    &ticode->ic.hl.oper.exp,
                                    pProc->localId.id[picode->ic.hl.oper.exp->expr.ident.idNode.regiIdx].id.regi,
                                    &pProc->localId);
                                if (res)
								{
                                    invalidateIcode (picode);
									numHlIcodes--;
								}
                                break;

                              /****case HLI_CALL:    /* register arguments 
                                newRegArg (pProc, picode, ticode);
								invalidateIcode (picode);
								numHlIcodes--;
								break;	*/
                            } /* eos */
                            break;

                        case HLI_CALL:  
							ticode = pProc->Icode.GetIcode(picode->du1.idx[0][0]);
                            switch (ticode->ic.hl.opcode) {
                              case HLI_ASSIGN:
								  exp = idCondExpFunc (
												picode->ic.hl.oper.call.proc,
												picode->ic.hl.oper.call.args);
                                  res = insertSubTreeReg (exp,
                                    	&ticode->ic.hl.oper.asgn.rhs,
										picode->ic.hl.oper.call.proc->retVal.id.regi,
                                    	&pProc->localId);
								  if (! res)
                                  	insertSubTreeReg (exp,
                                    	&ticode->ic.hl.oper.asgn.lhs,
										picode->ic.hl.oper.call.proc->retVal.id.regi,
                                    	&pProc->localId);
	/***  HERE missing: 2 regs ****/
                              	  invalidateIcode (picode); 
								  numHlIcodes--;
                              	  break;

                              case HLI_PUSH: case HLI_RET:
                              	  exp = idCondExpFunc (
												picode->ic.hl.oper.call.proc,
												picode->ic.hl.oper.call.args);
							  	  ticode->ic.hl.oper.exp = exp;	
							  	  invalidateIcode (picode);
								  numHlIcodes--;
                              	  break; 

							  case HLI_JCOND:
								exp = idCondExpFunc (
												picode->ic.hl.oper.call.proc,
												picode->ic.hl.oper.call.args);
								retVal = &picode->ic.hl.oper.call.proc->retVal,
                                res = insertSubTreeReg (exp, 
										&ticode->ic.hl.oper.exp,
										retVal->id.regi, &pProc->localId);
                                if (res)	/* was substituted */
								{
                              		invalidateIcode (picode);
									numHlIcodes--;
								}
								else	/* cannot substitute function */
								{
									lhs = idCondExpID(retVal,&pProc->localId,j);
									newAsgnHlIcode (picode, lhs, exp);
								}
								break;
                            } /* eos */
                            break;
                      } /* eos */
                    }
                }

                else if (picode->du1.numRegsDef == 2)   /* long regs */
                {
                    /* Check for only one use of these registers */
                    if ((picode->du1.idx[0][0] != 0) && 
						(picode->du1.idx[0][1] == 0) && 
                        (picode->du1.idx[1][0] != 0) &&
                        (picode->du1.idx[1][1] == 0))
                    {
                        switch (picode->ic.hl.opcode) {
                          case HLI_ASSIGN:  
                            /* Replace rhs of current icode into target
                             * icode expression */
                            if (picode->du1.idx[0][0] == picode->du1.idx[1][0])
                            {
                                ticode = pProc->Icode.GetIcode(picode->du1.idx[0][0]);
								if ((picode->du.lastDefRegi & duReg[regi]) &&
									((ticode->ic.hl.opcode != HLI_CALL) &&
								 	 (ticode->ic.hl.opcode != HLI_RET)))
								continue;

                                switch (ticode->ic.hl.opcode) {
                                  case HLI_ASSIGN:
									forwardSubsLong (picode->ic.hl.oper.asgn.lhs->expr.ident.idNode.longIdx,
										picode->ic.hl.oper.asgn.rhs, picode,
										ticode, &numHlIcodes);
                                    break;

                                  case HLI_JCOND:  case HLI_PUSH:  case HLI_RET:
                                    res = insertSubTreeLongReg (
                                        picode->ic.hl.oper.asgn.rhs,
                                        &ticode->ic.hl.oper.exp,
                                        picode->ic.hl.oper.asgn.lhs->expr.ident.idNode.longIdx);
                                    if (res)
									{
                                        invalidateIcode (picode);
										numHlIcodes--;
									}
                                    break;

                              	  case HLI_CALL:    /* register arguments */
                                	newRegArg (pProc, picode, ticode);
									invalidateIcode (picode);
									numHlIcodes--;
                                	break;
                                } /* eos */
                            }
                            break;

                          case HLI_POP:
                            if (picode->du1.idx[0][0] == picode->du1.idx[1][0])
                            {
                                ticode = pProc->Icode.GetIcode(picode->du1.idx[0][0]);
								if ((picode->du.lastDefRegi & duReg[regi]) &&
									((ticode->ic.hl.opcode != HLI_CALL) &&
								 	 (ticode->ic.hl.opcode != HLI_RET)))
								continue;

                                exp = popExpStk(); /* pop last exp pushed */
                            	switch (ticode->ic.hl.opcode) {
                              	case HLI_ASSIGN:
									forwardSubsLong (picode->ic.hl.oper.exp->expr.ident.idNode.longIdx,
										exp, picode, ticode, &numHlIcodes);
                                	break;
                              	case HLI_JCOND: case HLI_PUSH:
                                	res = insertSubTreeLongReg (exp,
                                    	&ticode->ic.hl.oper.exp,
										picode->ic.hl.oper.asgn.lhs->expr.ident.idNode.longIdx);
                                	if (res)
									{
                                    	invalidateIcode (picode);
										numHlIcodes--;
									}
                                	break;
								case HLI_CALL:	/*** missing ***/
									break;
                            	} /* eos */
							}
                            break;

                          case HLI_CALL:    /* check for function return */
							ticode = pProc->Icode.GetIcode(picode->du1.idx[0][0]);
                            switch (ticode->ic.hl.opcode) {
                              case HLI_ASSIGN:
								exp = idCondExpFunc (
											picode->ic.hl.oper.call.proc,
											picode->ic.hl.oper.call.args);
								ticode->ic.hl.oper.asgn.lhs = 
									idCondExpLong(&pProc->localId, DST, ticode,
												  HIGH_FIRST, j, E_DEF, 1);
                                ticode->ic.hl.oper.asgn.rhs = exp; 
                                invalidateIcode (picode); 
							    numHlIcodes--;
                                break;

                              case HLI_PUSH:  case HLI_RET:
                                exp = idCondExpFunc (
												picode->ic.hl.oper.call.proc,
												picode->ic.hl.oper.call.args);
                                ticode->ic.hl.oper.exp = exp; 
                                invalidateIcode (picode); 
							    numHlIcodes--;
                                break;

							  case HLI_JCOND:
							    exp = idCondExpFunc (
												picode->ic.hl.oper.call.proc,
												picode->ic.hl.oper.call.args);
								retVal = &picode->ic.hl.oper.call.proc->retVal;
                                res = insertSubTreeLongReg (exp,
                                    	&ticode->ic.hl.oper.exp,
										newLongRegId (&pProc->localId,
											retVal->type, retVal->id.longId.h,
											retVal->id.longId.l, j));
                                if (res)	/* was substituted */
								{
                              		invalidateIcode (picode);
									numHlIcodes--;
								}
								else	/* cannot substitute function */
								{
									lhs = idCondExpID(retVal,&pProc->localId,j);
									newAsgnHlIcode (picode, lhs, exp);
								}
							    break;
                            } /* eos */
                        } /* eos */
                    }
                }

                /* HLI_PUSH doesn't define any registers, only uses registers.
                 * Push the associated expression to the register on the local 
                 * expression stack */
                else if (picode->ic.hl.opcode == HLI_PUSH)
                {
                    pushExpStk (picode->ic.hl.oper.exp);
                    invalidateIcode (picode);
					numHlIcodes--;
                }

				/* For HLI_CALL instructions that use arguments from the stack,
				 * pop them from the expression stack and place them on the 
				 * procedure's argument list */
                if ((picode->ic.hl.opcode == HLI_CALL) &&
					! (picode->ic.hl.oper.call.proc->flg & REG_ARGS)) 
                { PPROC pp;
				  Int cb, numArgs;
				  boolT res;

					pp = picode->ic.hl.oper.call.proc;
                    if (pp->flg & CALL_PASCAL)
					{
						cb = pp->cbParam;	/* fixed # arguments */
						for (k = 0, numArgs = 0; k < cb; numArgs++)
						{
							exp = popExpStk();
							if (pp->flg & PROC_ISLIB)	/* library function */
							{
								if (pp->args.numArgs > 0)
									adjustActArgType(exp,
											pp->args.sym[numArgs].type, pProc);
								res = newStkArg (picode, exp, 
												 picode->ic.ll.opcode, pProc);
							}
							else			/* user function */
							{
								if (pp->args.numArgs >0)
									adjustForArgType (&pp->args, numArgs, 
												  	  expType (exp, pProc));
								res = newStkArg (picode, exp, 
												 picode->ic.ll.opcode, pProc);
							}
							if (res == FALSE)
								k += hlTypeSize (exp, pProc);
						}
					}
                    else		/* CALL_C */
					{
						cb = picode->ic.hl.oper.call.args->cb;
						numArgs = 0;
						if (cb) 
                        	for (k = 0; k < cb; numArgs++)
								processCArg (pp, pProc, picode, numArgs, &k);
						else if ((cb == 0) && (picode->ic.ll.flg & REST_STK))
							while (! emptyExpStk())
							{
								processCArg (pp, pProc, picode, numArgs, &k);
								numArgs++;
							}
					}
                } 

				/* If we could not substitute the result of a function, 
				 * assign it to the corresponding registers */
				if ((picode->ic.hl.opcode == HLI_CALL) &&
					((picode->ic.hl.oper.call.proc->flg & PROC_ISLIB) !=
					  PROC_ISLIB) && (picode->du1.idx[0][0] == 0) &&
					(picode->du1.numRegsDef > 0))
				{
					exp = idCondExpFunc (picode->ic.hl.oper.call.proc,
										 picode->ic.hl.oper.call.args);
					lhs = idCondExpID (&picode->ic.hl.oper.call.proc->retVal,
									   &pProc->localId, j);
					newAsgnHlIcode (picode, lhs, exp);
				}
            }
        }

		/* Store number of high-level icodes in current basic block */
		pbb->numHlIcodes = numHlIcodes;
    }
}


void dataFlow (PPROC pProc, dword liveOut)
/* Invokes procedures related with data flow analysis.  Works on a procedure
 * at a time basis. 
 * Note: indirect recursion in liveRegAnalysis is possible. */
{ boolT isAx, isBx, isCx, isDx;
  Int idx;

	/* Remove references to register variables */
	if (pProc->flg & SI_REGVAR)
		liveOut &= maskDuReg[rSI];
	if (pProc->flg & DI_REGVAR)
		liveOut &= maskDuReg[rDI];

    /* Function - return value register(s) */
    if (liveOut != 0)
    {
        pProc->flg |= PROC_IS_FUNC;
        isAx = (boolT)(liveOut & power2(rAX - rAX));
        isBx = (boolT)(liveOut & power2(rBX - rAX));
        isCx = (boolT)(liveOut & power2(rCX - rAX));
        isDx = (boolT)(liveOut & power2(rDX - rAX));

        if (isAx && isDx)       /* long or pointer */
        {
            pProc->retVal.type = TYPE_LONG_SIGN;
            pProc->retVal.loc = REG_FRAME;
            pProc->retVal.id.longId.h = rDX;
            pProc->retVal.id.longId.l = rAX;
			idx = newLongRegId (&pProc->localId, TYPE_LONG_SIGN, rDX, rAX, 0);
			propLongId (&pProc->localId, rAX, rDX, "\0"); 
        }
        else if (isAx || isBx || isCx || isDx)	/* word */
        {
            pProc->retVal.type = TYPE_WORD_SIGN;
            pProc->retVal.loc = REG_FRAME;
            if (isAx)
                pProc->retVal.id.regi = rAX;
            else if (isBx)
                pProc->retVal.id.regi = rBX;
            else if (isCx)
                pProc->retVal.id.regi = rCX;
            else 
                pProc->retVal.id.regi = rDX;
			idx = newByteWordRegId (&pProc->localId, TYPE_WORD_SIGN,
									pProc->retVal.id.regi);
        }
    }

    /* Data flow analysis */
    pProc->liveAnal = TRUE;
    elimCondCodes (pProc);
    genLiveKtes (pProc);
    liveRegAnalysis (pProc, liveOut);   /* calls dataFlow() recursively */
	if (! (pProc->flg & PROC_ASM))		/* can generate C for pProc		*/
	{
    	genDU1 (pProc);			/* generate def/use level 1 chain */
    	findExps (pProc); 		/* forward substitution algorithm */ 
	}
}

