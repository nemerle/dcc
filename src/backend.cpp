/*****************************************************************************
 * Project: 	dcc
 * File:	backend.c
 * Purpose:	Back-end module.  Generates C code for each procedure.
 * (C) Cristina Cifuentes
 ****************************************************************************/

#include "dcc.h"
#include <string.h>
#include <stdio.h>

bundle cCode;			/* Procedure declaration and code */

/* Indentation buffer */
#define indSize	81		/* size of the indentation buffer.  Each indentation
						 * is of 4 spaces => max. 20 indentation levels */
static char indentBuf[indSize] = 
	"                                                                                ";


static char *indent (Int indLevel)
/* Indentation according to the depth of the statement */
{ 
	return (&indentBuf[indSize-(indLevel*4)-1]); 
}


static Int getNextLabel()
/* Returns a unique index to the next label */
{ static Int labelIdx = 1;	/* index of the next label		*/

	return (labelIdx++);
}


static void displayStats (PPROC pProc)
/* displays statistics on the subroutine */
{
   printf("\nStatistics - Subroutine %s\n", pProc->name);
   printf ("Number of Icode instructions:\n");
   printf ("  Low-level : %4d\n", stats.numLLIcode);
   if (! (pProc->flg & PROC_ASM))
   {
	  printf ("  High-level: %4d\n", stats.numHLIcode);
      printf ("  Percentage reduction: %2.2f%%\n", 100.0 - (stats.numHLIcode *
           100.0) / stats.numLLIcode);
   }
}


/**** this proc is not required any more?? ****/
#if 0
static void fixupLabels (PPROC pProc)
/* Checks the graph (pProc->cfg) for any nodes that have labels, and gives
 * a unique label number for it.  This label is placed in the associated
 * icode for the node (pProc->Icode).  The procedure is done in sequential
 * order of dsfLast numbering.	*/
{ Int i;				/* index into the dfsLast array */
  PBB *dfsLast;			/* pointer to the dfsLast array */

	dfsLast = pProc->dfsLast;
	for (i = 0; i < pProc->numBBs; i++)
		if (dfsLast[i]->flg/* & BB_HAS_LABEL*/) {
			pProc->Icode.icode[dfsLast[i]->start].ic.ll.flg |= HLL_LABEL;
			pProc->Icode.icode[dfsLast[i]->start].ic.ll.hllLabNum = getNextLabel();
		}
}
#endif


char *cChar (byte c)
/* Returns the corresponding C string for the given character c.  Character
 * constants such as carriage return and line feed, require 2 C characters. */
{ static char res[3];

	switch (c) {
	  case 0x8:		/* backspace */
					sprintf (res, "\\b");
					break;
	  case 0x9:		/* horizontal tab */
					sprintf (res, "\\t");
					break;
	  case 0x0A:	/* new line */
					sprintf (res, "\\n");
					break;
	  case 0x0C:	/* form feed */
					sprintf (res, "\\f");
					break;
	  case 0x0D:	/* carriage return */
					sprintf (res, "\\r");
					break;
	  default: 		/* any other character*/
					sprintf (res, "%c", c);
	}
	return (res);
}


static void printGlobVar (PSYM psym)
/* Prints the variable's name and initial contents on the file.  
 * Note: to get to the value of the variable:
 *		com file: prog.Image[operand]
 *		exe file: prog.Image[operand+0x100] 	*/
{ Int j;
  dword relocOp = prog.fCOM ? psym->label : psym->label + 0x100;
  char *strContents;		/* initial contents of variable */

	switch (psym->size) {
		 case 1: appendStrTab (&cCode.decl, "byte\t%s = %ld;\n",
				 psym->name, prog.Image[relocOp]);
				 break;
		 case 2: appendStrTab (&cCode.decl, "word\t%s = %ld;\n",
				 psym->name, LH(prog.Image+relocOp));
				 break;
		 case 4: if (psym->type == TYPE_PTR)  /* pointer */
					appendStrTab (&cCode.decl, "word\t*%s = %ld;\n",
								  psym->name, LH(prog.Image+relocOp));
				 else 			/* char */
					appendStrTab (&cCode.decl,
								"char\t%s[4] = \"%c%c%c%c\";\n",
								psym->name, prog.Image[relocOp],
								prog.Image[relocOp+1], prog.Image[relocOp+2],
								prog.Image[relocOp+3]);
				 break;
		 default:strContents = (char *)allocMem((psym->size*2+1) *sizeof(char));
				 strContents[0] = '\0';
				 for (j=0; j < psym->size; j++)
					strcat (strContents, cChar(prog.Image[relocOp + j]));
				 appendStrTab (&cCode.decl, "char\t*%s = \"%s\";\n",
								psym->name, strContents);
	   }
}


// Note: Not called at present.
static void writeGlobSymTable()
/* Writes the contents of the symbol table, along with any variable
 * initialization. */
{ Int idx;
  char type[10];
  PSYM pSym;

	if (symtab.csym) 
	{
		appendStrTab (&cCode.decl, "/* Global variables */\n");
		for (idx = 0; idx < symtab.csym; idx++) 
		{
	       pSym = &symtab.sym[idx];
	       if (symtab.sym[idx].duVal & USEVAL)	/* first used */
	          printGlobVar (&(symtab.sym[idx]));
	       else {					/* first defined */
	          switch (pSym->size) {
	          case 1:  strcpy (type, "byte\t"); break;
	          case 2:  strcpy (type, "int\t"); break;
	          case 4:  if (pSym->type == TYPE_PTR)
						strcpy (type, "int\t*");
					   else
						strcpy (type, "char\t*");
					   break;
			  default: strcpy (type, "char\t*");
			  }
			  appendStrTab (&cCode.decl, "%s%s;\t/* size = %ld */\n",
			 				type, pSym->name, pSym->size);
		   }
	   }
	   appendStrTab (&cCode.decl, "\n");
	}
}


static void writeHeader (FILE *fp, char *fileName)
/* Writes the header information and global variables to the output C file
 * fp. */
{ 
	/* Write header information */
	newBundle (&cCode);
	appendStrTab (&cCode.decl, "/*\n");
	appendStrTab (&cCode.decl, " * Input file\t: %s\n", fileName);
	appendStrTab (&cCode.decl, " * File type\t: %s\n", (prog.fCOM)?"COM":"EXE");
	appendStrTab (&cCode.decl, " */\n\n#include \"dcc.h\"\n\n"); 

	/* Write global symbol table */
	/** writeGlobSymTable(); *** need to change them into locident fmt ***/
	writeBundle (fp, cCode);
	freeBundle (&cCode); 
}


static void writeBitVector (dword regi)
/* Writes the registers that are set in the bitvector */
{ Int j;

    for (j = 0; j < INDEXBASE; j++)
    {
        if ((regi & power2(j)) != 0)
            printf ("%s ", allRegs[j]);
    }
}


static void emitGotoLabel (PICODE pt, Int indLevel)
/* Checks the given icode to determine whether it has a label associated
 * to it.  If so, a goto is emitted to this label; otherwise, a new label
 * is created and a goto is also emitted.	
 * Note: this procedure is to be used when the label is to be backpatched
 *       onto code in cCode.code */
{
	if (! (pt->ic.ll.flg & HLL_LABEL)) /* node hasn't got a lab */
	{
		/* Generate new label */
		pt->ic.ll.hllLabNum = getNextLabel();
		pt->ic.ll.flg |= HLL_LABEL;

		/* Node has been traversed already, so backpatch this label into
		 * the code */
		addLabelBundle (&cCode.code, pt->codeIdx, pt->ic.ll.hllLabNum);
	}
	appendStrTab (&cCode.code, "%sgoto L%ld;\n", indent(indLevel), 
				  pt->ic.ll.hllLabNum);
	stats.numHLIcode++;
}


// Note: Not currently called!
static void emitFwdGotoLabel (PICODE pt, Int indLevel)
/* Checks the given icode to determine whether it has a label associated
 * to it.  If so, a goto is emitted to this label; otherwise, a new label
 * is created and a goto is also emitted.	
 * Note: this procedure is to be used when the label is to be forward on
 *		 the code; that is, the target code has not been traversed yet. */
{
	if (! (pt->ic.ll.flg & HLL_LABEL)) /* node hasn't got a lab */
	{
		/* Generate new label */
		pt->ic.ll.hllLabNum = getNextLabel();
		pt->ic.ll.flg |= HLL_LABEL;
	}
	appendStrTab (&cCode.code, "%sgoto l%ld;\n", indent(indLevel),
				  pt->ic.ll.hllLabNum);
}


static void writeBB (PBB pBB, PICODE hli, Int lev, PPROC pProc, Int *numLoc)
/* Writes the code for the current basic block.
 * Args: pBB: pointer to the current basic block.
 *		 Icode: pointer to the array of icodes for current procedure.
 *		 lev: indentation level - used for formatting.	*/
{ Int	i, last;
  char *line;           /* Pointer to the HIGH-LEVEL line   */

	/* Save the index into the code table in case there is a later goto
	 * into this instruction (first instruction of the BB) */
	hli[pBB->start].codeIdx = nextBundleIdx (&cCode.code);

	/* Generate code for each hlicode that is not a HLI_JCOND */
	for (i = pBB->start, last = i + pBB->length; i < last; i++)
		if ((hli[i].type == HIGH_LEVEL) && (hli[i].invalid == FALSE))
		{
			line = write1HlIcode (hli[i].ic.hl, pProc, numLoc);
			if (line[0] != '\0')
			{
				appendStrTab (&cCode.code, "%s%s", indent(lev), line);
				stats.numHLIcode++;
			}
			if (option.verbose)
				writeDU (&hli[i], i);
		}
//if (hli[i].invalid)
//printf("Invalid icode: %d!\n", hli[i].invalid);
}


static void writeCode (PBB pBB, Int indLevel, PPROC pProc , Int *numLoc,
					   Int latchNode, Int ifFollow)
/* Recursive procedure that writes the code for the given procedure, pointed
 * to by pBB.
 * Parameters:	pBB:	pointer to the cfg.
 *				Icode:	pointer to the Icode array for the cfg graph of the
 *						current procedure.
 *				indLevel: indentation level - used for formatting.
 *				numLoc: last # assigned to local variables 				*/
{
  Int follow,						/* ifFollow						*/
      loopType, 					/* Type of loop, if any 		*/ 
  	  nodeType;						/* Type of node 				*/
  PBB succ, latch;					/* Successor and latching node 	*/
  PICODE picode;					/* Pointer to HLI_JCOND instruction	*/
  char *l;							/* Pointer to HLI_JCOND expression	*/
  boolT emptyThen,					/* THEN clause is empty			*/
		repCond;					/* Repeat condition for while() */

	/* Check if this basic block should be analysed */
	if (!pBB) return;
	if ((ifFollow != UN_INIT) && (pBB == pProc->dfsLast[ifFollow])) 
		return;

	if (pBB->traversed == DFS_ALPHA)
		return;
	pBB->traversed = DFS_ALPHA;

	/* Check for start of loop */
	repCond = FALSE;
	latch = NULL;
	loopType = pBB->loopType;
	if (loopType) 
	{
		latch = pProc->dfsLast[pBB->latchNode];
	    switch (loopType) {
	      case WHILE_TYPE:
			picode = pProc->Icode.GetIcode(pBB->start + pBB->length - 1);

			/* Check for error in while condition */
			if (picode->ic.hl.opcode != HLI_JCOND)
				reportError (WHILE_FAIL);

			/* Check if condition is more than 1 HL instruction */
			if (pBB->numHlIcodes > 1)
			{
				/* Write the code for this basic block */
				writeBB(pBB, pProc->Icode.GetFirstIcode(), indLevel, pProc, numLoc);
				repCond = TRUE;
			}

			/* Condition needs to be inverted if the loop body is along
			 * the THEN path of the header node */
			if (pBB->edges[ELSE].BBptr->dfsLastNum == pBB->loopFollow)
				inverseCondOp (&picode->ic.hl.oper.exp);
			appendStrTab (&cCode.code, "\n%swhile (%s) {\n", indent(indLevel), 
						walkCondExpr (picode->ic.hl.oper.exp, pProc, numLoc));
			invalidateIcode (picode);
			break;

		  case REPEAT_TYPE:
			appendStrTab (&cCode.code, "\n%sdo {\n", indent(indLevel));
			picode = pProc->Icode.GetIcode(latch->start+latch->length-1);
			invalidateIcode (picode);
			break;

		  case ENDLESS_TYPE:
			appendStrTab (&cCode.code, "\n%sfor (;;) {\n", indent(indLevel));
	    }
		stats.numHLIcode += 1;
	    indLevel++;
	}

	/* Write the code for this basic block */
	if (repCond == FALSE)
		writeBB (pBB, pProc->Icode.GetFirstIcode(), indLevel, pProc, numLoc);

	/* Check for end of path */
	nodeType = pBB->nodeType;
	if (nodeType == RETURN_NODE || nodeType == TERMINATE_NODE ||
	    nodeType == NOWHERE_NODE || (pBB->dfsLastNum == latchNode))
		return;

	/* Check type of loop/node and process code */
	if (loopType)	/* there is a loop */
	{
		if (pBB != latch)		/* loop is over several bbs */
		{
			if (loopType == WHILE_TYPE)
			{
				succ = pBB->edges[THEN].BBptr;
				if (succ->dfsLastNum == pBB->loopFollow)
					succ = pBB->edges[ELSE].BBptr;
			}
			else
				succ = pBB->edges[0].BBptr;
			if (succ->traversed != DFS_ALPHA)
				writeCode (succ, indLevel, pProc, numLoc, latch->dfsLastNum, 
						   ifFollow);
			else	/* has been traversed so we need a goto */
				emitGotoLabel (pProc->Icode.GetIcode(succ->start), indLevel);
		}

		/* Loop epilogue: generate the loop trailer */
		indLevel--;
		if (loopType == WHILE_TYPE)
		{
			/* Check if there is need to repeat other statements involved
			 * in while condition, then, emit the loop trailer */
			if (repCond)
				writeBB (pBB, pProc->Icode.GetFirstIcode(), indLevel+1, pProc, numLoc); 
			appendStrTab (&cCode.code, "%s}	/* end of while */\n",
					indent(indLevel));
		}
		else if (loopType == ENDLESS_TYPE)
			appendStrTab (&cCode.code, "%s}	/* end of loop */\n",
				indent(indLevel));
		else if (loopType == REPEAT_TYPE) 
		{
			if (picode->ic.hl.opcode != HLI_JCOND)
				reportError (REPEAT_FAIL);
		   	appendStrTab (&cCode.code, "%s} while (%s);\n", indent(indLevel),
				walkCondExpr (picode->ic.hl.oper.exp, pProc, numLoc));
		}

		/* Recurse on the loop follow */
		if (pBB->loopFollow != MAX) 
		{
			succ = pProc->dfsLast[pBB->loopFollow];
			if (succ->traversed != DFS_ALPHA)
				writeCode (succ, indLevel, pProc, numLoc, latchNode, ifFollow);
			else		/* has been traversed so we need a goto */
				emitGotoLabel (pProc->Icode.GetIcode(succ->start), indLevel);
		}
	}

	else		/* no loop, process nodeType of the graph */
	{
		if (nodeType == TWO_BRANCH)		/* if-then[-else] */
		{
		  stats.numHLIcode++;
		  indLevel++;
		  emptyThen = FALSE;

		  if (pBB->ifFollow != MAX)		/* there is a follow */
		  {
			/* process the THEN part */
			follow = pBB->ifFollow;
			succ = pBB->edges[THEN].BBptr;
			if (succ->traversed != DFS_ALPHA)	/* not visited */
			{
				if (succ->dfsLastNum != follow)	/* THEN part */
				{
					l = writeJcond (
						pProc->Icode.GetIcode(pBB->start + pBB->length -1)->ic.hl, 
						pProc, numLoc);
					appendStrTab (&cCode.code, "\n%s%s", indent(indLevel-1), l);
					writeCode (succ, indLevel, pProc, numLoc, latchNode, 
							   follow); 
				}
				else		/* empty THEN part => negate ELSE part */
				{
					l = writeJcondInv (
						pProc->Icode.GetIcode(pBB->start + pBB->length -1)->ic.hl, 
						pProc, numLoc);
					appendStrTab (&cCode.code, "\n%s%s", indent(indLevel-1), l);
					writeCode (pBB->edges[ELSE].BBptr, indLevel, pProc, numLoc,
							   latchNode, follow); 
					emptyThen = TRUE;
				}
			}
			else	/* already visited => emit label */
				emitGotoLabel (pProc->Icode.GetIcode(succ->start), indLevel);

			/* process the ELSE part */
			succ = pBB->edges[ELSE].BBptr;
			if (succ->traversed != DFS_ALPHA)		/* not visited */
			{
				if (succ->dfsLastNum != follow)		/* ELSE part */
				{
					appendStrTab (&cCode.code, "%s}\n%selse {\n",
								  indent(indLevel-1), indent(indLevel - 1));
					writeCode (succ, indLevel, pProc, numLoc, latchNode, 
							   follow); 
				}
				/* else (empty ELSE part) */
			}
			else if (! emptyThen) 	/* already visited => emit label */
			{
				appendStrTab (&cCode.code, "%s}\n%selse {\n",
							  indent(indLevel-1), indent(indLevel - 1));
				emitGotoLabel (pProc->Icode.GetIcode(succ->start), indLevel);
			}
			appendStrTab (&cCode.code, "%s}\n", indent(--indLevel));

			/* Continue with the follow */
			succ = pProc->dfsLast[follow];
			if (succ->traversed != DFS_ALPHA)
				writeCode (succ, indLevel, pProc, numLoc, latchNode, 
						   ifFollow);
		  }
		  else		/* no follow => if..then..else */
		  {
			l = writeJcond (
					pProc->Icode.GetIcode(pBB->start + pBB->length -1)->ic.hl, 
					pProc, numLoc);
			appendStrTab (&cCode.code, "%s%s", indent(indLevel-1), l);
			writeCode (pBB->edges[THEN].BBptr, indLevel, pProc, numLoc, 
					   latchNode, ifFollow); 
			appendStrTab (&cCode.code, "%s}\n%selse {\n", indent(indLevel-1), 
						  indent(indLevel - 1));
			writeCode (pBB->edges[ELSE].BBptr, indLevel, pProc, numLoc, 
					   latchNode, ifFollow); 
			appendStrTab (&cCode.code, "%s}\n", indent(--indLevel));
		  }
		}

		else 	/* fall, call, 1w */
		{
			succ = pBB->edges[0].BBptr;		/* fall-through edge */
			if (succ->traversed != DFS_ALPHA)
			   writeCode (succ, indLevel, pProc,numLoc, latchNode,ifFollow);
		}
	}
}


static void codeGen (PPROC pProc, FILE *fp)
/* Writes the procedure's declaration (including arguments), local variables,
 * and invokes the procedure that writes the code of the given record *hli */
{ Int i, numLoc;
  PSTKFRAME args;       /* Procedure arguments              */
  char buf[200],        /* Procedure's definition           */
       arg[30];         /* One argument                     */
  ID *locid;            /* Pointer to one local identifier  */
  BB *pBB;              /* Pointer to basic block           */

    /* Write procedure/function header */
	newBundle (&cCode);
    if (pProc->flg & PROC_IS_FUNC)      /* Function */
		appendStrTab (&cCode.decl, "\n%s %s (", hlTypes[pProc->retVal.type], 
					  pProc->name);
    else                                /* Procedure */
        appendStrTab (&cCode.decl, "\nvoid %s (", pProc->name);

    /* Write arguments */
    args = &pProc->args;
	memset (buf, 0, sizeof(buf));
    for (i = 0; i < args->csym; i++)
    {
		if (args->sym[i].invalid == FALSE)
		{
			sprintf (arg,"%s %s",hlTypes[args->sym[i].type], args->sym[i].name);
        	strcat (buf, arg);
        	if (i < (args->numArgs - 1))
            	strcat (buf, ", ");
		}
    }
    strcat (buf, ")\n");
    appendStrTab (&cCode.decl, "%s", buf);

	/* Write comments */
	writeProcComments (pProc, &cCode.decl);

    /* Write local variables */
	if (! (pProc->flg & PROC_ASM))
	{
    	locid = &pProc->localId.id[0];
    	numLoc = 0;
    	for (i = 0; i < pProc->localId.csym; i++, locid++)
    	{
        	/* Output only non-invalidated entries */   
        	if (locid->illegal == FALSE)
        	{
            	if (locid->loc == REG_FRAME)
            	{
                	/* Register variables are assigned to a local variable */
                	if (((pProc->flg & SI_REGVAR) && (locid->id.regi == rSI)) ||
                    	((pProc->flg & DI_REGVAR) && (locid->id.regi == rDI)))
                	{
                		sprintf (locid->name, "loc%ld", ++numLoc);
                    	appendStrTab (&cCode.decl, "int %s;\n", locid->name);
                	}
					/* Other registers are named when they are first used in
				  	 * the output C code, and appended to the proc decl. */
            	}

            	else if (locid->loc == STK_FRAME)
            	{
                	/* Name local variables and output appropriate type */
                	sprintf (locid->name, "loc%ld", ++numLoc);
					appendStrTab (&cCode.decl, "%s %s;\n", 
								  hlTypes[locid->type], locid->name);
            	}
        	}
    	}
	}
    /* Write procedure's code */
	if (pProc->flg & PROC_ASM)		/* generate assembler */
		disassem (3, pProc);
	else							/* generate C */
		writeCode (pProc->cfg, 1, pProc, &numLoc, MAX, UN_INIT);

	appendStrTab (&cCode.code, "}\n\n");
	writeBundle (fp, cCode);
	freeBundle (&cCode);

    /* Write Live register analysis information */
	if (option.verbose) 
    	for (i = 0; i < pProc->numBBs; i++)
    	{
        	pBB = pProc->dfsLast[i];
			if (pBB->flg & INVALID_BB)	continue;	/* skip invalid BBs */
        	printf ("BB %d\n", i);
			printf ("  Start = %d, end = %d\n", pBB->start, pBB->start + 
					pBB->length - 1);
        	printf ("  LiveUse = ");
        	writeBitVector (pBB->liveUse);
        	printf ("\n  Def = ");
        	writeBitVector (pBB->def);
        	printf ("\n  LiveOut = ");
        	writeBitVector (pBB->liveOut);
        	printf ("\n  LiveIn = ");
        	writeBitVector (pBB->liveIn);
        	printf ("\n\n");
    	}
}


static void backBackEnd (char *filename, PCALL_GRAPH pcallGraph, FILE *fp)
/* Recursive procedure. Displays the procedure's code in depth-first order
 * of the call graph.	*/
{ Int i;

//	IFace.Yield();			/* This is a good place to yield to other apps */

	/* Check if this procedure has been processed already */
	if ((pcallGraph->proc->flg & PROC_OUTPUT) || 
		(pcallGraph->proc->flg & PROC_ISLIB))
		return;
	pcallGraph->proc->flg |= PROC_OUTPUT;

	/* Dfs if this procedure has any successors */
	if (pcallGraph->numOutEdges > 0)
	{
		for (i = 0; i < pcallGraph->numOutEdges; i++)
		{
			backBackEnd (filename, pcallGraph->outEdges[i], fp);
		}
	}

	/* Generate code for this procedure */
	stats.numLLIcode = pcallGraph->proc->Icode.GetNumIcodes();
	stats.numHLIcode = 0;
	codeGen (pcallGraph->proc, fp);

	/* Generate statistics */
	if (option.Stats)
		displayStats (pcallGraph->proc);
	if (! (pcallGraph->proc->flg & PROC_ASM))
	{
		stats.totalLL += stats.numLLIcode;
		stats.totalHL += stats.numHLIcode;
	}
}


void BackEnd (char *fileName, PCALL_GRAPH pcallGraph)
/* Invokes the necessary routines to produce code one procedure at a time. */
{
 	char*	outName, *ext;
	FILE*	fp;				/* Output C file 	*/

	/* Get output file name */
	outName = strcpy ((char*)allocMem(strlen(fileName)+1), fileName);
	if ((ext = strrchr (outName, '.')) != NULL)
	   *ext = '\0';
	strcat (outName, ".b");		/* b for beta */

	/* Open output file */
	fp = fopen (outName, "wt");
	if (!fp)
	   fatalError (CANNOT_OPEN, outName);
	printf ("dcc: Writing C beta file %s\n", outName);

	/* Header information */
	writeHeader (fp, fileName);

	/* Initialize total Icode instructions statistics */
	stats.totalLL = 0;
	stats.totalHL = 0;

	/* Process each procedure at a time */
	backBackEnd (fileName, pcallGraph, fp);

	/* Close output file */
	fclose (fp);
	printf ("dcc: Finished writing C beta file\n");
}


