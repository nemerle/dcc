/****************************************************************************
 *          dcc project procedure list builder
 * (C) Cristina Cifuentes, Mike van Emmerik, Jeff Ledermann
 ****************************************************************************/

#include "dcc.h"
#include <string.h>
#include <stdlib.h>		/* For exit() */

#ifdef __DOSWIN__
#include <stdio.h>
#endif

static void     FollowCtrl (PPROC pProc, PCALL_GRAPH pcallGraph, PSTATE pstate);
static boolT    process_JMP (PICODE pIcode, PPROC pProc, PSTATE pstate,
                             PCALL_GRAPH pcallGraph);
static boolT    process_CALL(PICODE pIcode, PPROC pProc, PCALL_GRAPH pcallGraph,
                             PSTATE pstate);
static void     process_operands(PICODE pIcode, PPROC pProc, PSTATE pstate,
                                 Int ix);
static void     setBits(int16 type, dword start, dword len);
static PSYM     updateGlobSym(dword operand, Int size, word duFlag);
static void     process_MOV(PICODE pIcode, PSTATE pstate);
static PSYM     lookupAddr (PMEM pm, PSTATE pstate, Int size, word duFlag);
        void    interactDis(PPROC initProc, Int ic);
static dword    SynthLab;


void parse (PCALL_GRAPH *pcallGraph)
/* Parses the program, builds the call graph, and returns the list of 
 * procedures found     */
{ STATE state;

    /* Set initial state */
    memset(&state, 0, sizeof(STATE));
    setState(&state, rES, 0);   /* PSP segment */
    setState(&state, rDS, 0);
    setState(&state, rCS, prog.initCS);
    setState(&state, rSS, prog.initSS);
    setState(&state, rSP, prog.initSP);
    state.IP = ((dword)prog.initCS << 4) + prog.initIP;
    SynthLab = SYNTHESIZED_MIN;


    /* Check for special settings of initial state, based on idioms of the
        startup code */
    checkStartup(&state);

    /* Make a struct for the initial procedure */
	pProcList = allocStruc(PROCEDURE);
	pProcList = (PPROC)memset(pProcList, 0, sizeof(PROCEDURE));
    if (prog.offMain != -1)
    {
        /* We know where main() is. Start the flow of control from there */
        pProcList->procEntry = prog.offMain;
        /* In medium and large models, the segment of main may (will?) not be
            the same as the initial CS segment (of the startup code) */
        setState(&state, rCS, prog.segMain);
        strcpy(pProcList->name, "main");
        state.IP = prog.offMain;
    }
    else
    {
        /* Create initial procedure at program start address */
        pProcList->procEntry = (dword)state.IP;
    }
    /* The state info is for the first procedure */
    memcpy(&(pProcList->state), &state, sizeof(STATE));
    pLastProc = pProcList;

    /* Set up call graph initial node */
	*pcallGraph = allocStruc(CALL_GRAPH);
	memset (*pcallGraph, 0, sizeof(CALL_GRAPH));
    (*pcallGraph)->proc = pProcList;

    /* This proc needs to be called to set things up for LibCheck(), which
        checks a proc to see if it is a know C (etc) library */
    SetupLibCheck();

    /* Recursively build entire procedure list */
    FollowCtrl (pProcList, *pcallGraph, &state);

    /* This proc needs to be called to clean things up from SetupLibCheck() */
    CleanupLibCheck();
}


static void updateSymType (dword symbol, hlType symType, Int size)
/* Updates the type of the symbol in the symbol table.  The size is updated
 * if necessary (0 means no update necessary).      */
{ Int i;

    for (i = 0; i < symtab.csym; i++)
      if (symtab.sym[i].label == symbol)  
      {
         symtab.sym[i].type = symType;
         if (size != 0)
            symtab.sym[i].size = size;
         break;
      }
}


Int strSize (byte *sym, char delim)
/* Returns the size of the string pointed by sym and delimited by delim. 
 * Size includes delimiter.     */
{ Int i;

    for (i = 0; *sym++ != delim; i++)  ;
    return (i+1);
}


static void FollowCtrl (PPROC pProc, PCALL_GRAPH pcallGraph, PSTATE pstate) 
/* FollowCtrl - Given an initial procedure, state information and symbol table
 *      builds a list of procedures reachable from the initial procedure
 *      using a depth first search.     */
{
    ICODE   Icode, *pIcode;     /* This gets copied to pProc->Icode[] later */
    ICODE   eIcode;             /* extra icodes for iDIV, iIDIV, iXCHG */
    PSYM    psym;
    dword   offset;
    Int     err;
    boolT   done = FALSE;
    Int     lab;

	if (strstr(pProc->name, "chkstk") != NULL)
	{
		// Danger! Dcc will likely fall over in this code.
		// So we act as though we have done with this proc
//		pProc->flg &= ~TERMINATES;			// Not sure about this
        done = TRUE;
        // And mark it as a library function, so structure() won't choke on it
		pProc->flg |= PROC_ISLIB;
		return;
	}
	if (option.VeryVerbose)
	{
		printf("Parsing proc %s at %lX\n", pProc->name, pstate->IP);
	}

    while (! done && ! (err = scan(pstate->IP, &Icode)))
    {
        pstate->IP += (dword)Icode.ic.ll.numBytes;
        setBits(BM_CODE, Icode.ic.ll.label, (dword)Icode.ic.ll.numBytes);

        process_operands(&Icode, pProc, pstate, pProc->Icode.GetNumIcodes());

        /* Keep track of interesting instruction flags in procedure */
        pProc->flg |= (Icode.ic.ll.flg & (NOT_HLL | FLOAT_OP));

        /* Check if this instruction has already been parsed */
		if (pProc->Icode.labelSrch(Icode.ic.ll.label, &lab))
        {   /* Synthetic jump */
            Icode.type = LOW_LEVEL;
            Icode.ic.ll.opcode = iJMP;
            Icode.ic.ll.flg = I | SYNTHETIC | NO_OPS;
            Icode.ic.ll.immed.op = pProc->Icode.GetLlLabel(lab);
            Icode.ic.ll.label = SynthLab++;
        }

        /* Copy Icode to Proc */
        if ((Icode.ic.ll.opcode == iDIV) || (Icode.ic.ll.opcode == iIDIV))
        {
            /* MOV rTMP, reg */
            memset (&eIcode, 0, sizeof (ICODE));
			eIcode.type = LOW_LEVEL;
            eIcode.ic.ll.opcode = iMOV;
            eIcode.ic.ll.dst.regi = rTMP;
            if (Icode.ic.ll.flg & B)
            {
                eIcode.ic.ll.flg |= B;
                eIcode.ic.ll.src.regi = rAX;
                setRegDU (&eIcode, rAX, E_USE);
            }
            else    /* implicit dx:ax */
            {
                eIcode.ic.ll.flg |= IM_SRC;
                setRegDU (&eIcode, rAX, E_USE);
                setRegDU (&eIcode, rDX, E_USE);
            }
            setRegDU (&eIcode, rTMP, E_DEF); 
            eIcode.ic.ll.flg |= SYNTHETIC;
            /* eIcode.ic.ll.label = SynthLab++; */
            eIcode.ic.ll.label = Icode.ic.ll.label;
			pIcode = pProc->Icode.addIcode(&eIcode);

            /* iDIV, iIDIV */
            pIcode = pProc->Icode.addIcode(&Icode);

            /* iMOD */
            memset (&eIcode, 0, sizeof (ICODE));
			eIcode.type = LOW_LEVEL;
            eIcode.ic.ll.opcode = iMOD;
            memcpy (&eIcode.ic.ll.src, &Icode.ic.ll.src, sizeof(ICODEMEM));
            memcpy (&eIcode.du, &Icode.du, sizeof(DU_ICODE));
            eIcode.ic.ll.flg = (Icode.ic.ll.flg | SYNTHETIC);
            eIcode.ic.ll.label = SynthLab++;
            pIcode = pProc->Icode.addIcode(&eIcode);
        }
        else if (Icode.ic.ll.opcode == iXCHG)
        {
            /* MOV rTMP, regDst */
            memset (&eIcode, 0, sizeof (ICODE));
            eIcode.type = LOW_LEVEL;
            eIcode.ic.ll.opcode = iMOV;
            eIcode.ic.ll.dst.regi = rTMP;
            eIcode.ic.ll.src.regi = Icode.ic.ll.dst.regi;
            setRegDU (&eIcode, rTMP, E_DEF); 
            setRegDU (&eIcode, eIcode.ic.ll.src.regi, E_USE); 
            eIcode.ic.ll.flg |= SYNTHETIC;
            /* eIcode.ic.ll.label = SynthLab++; */
            eIcode.ic.ll.label = Icode.ic.ll.label; 
            pIcode = pProc->Icode.addIcode(&eIcode);

            /* MOV regDst, regSrc */
            Icode.ic.ll.opcode = iMOV;
            Icode.ic.ll.flg |= SYNTHETIC;
            /* Icode.ic.ll.label = SynthLab++; */
            pIcode = pProc->Icode.addIcode(&Icode);
            Icode.ic.ll.opcode = iXCHG; /* for next case */

            /* MOV regSrc, rTMP */ 
            memset (&eIcode, 0, sizeof (ICODE));
            eIcode.type = LOW_LEVEL;
            eIcode.ic.ll.opcode = iMOV;
            eIcode.ic.ll.dst.regi = Icode.ic.ll.src.regi;
            eIcode.ic.ll.src.regi = rTMP;
            setRegDU (&eIcode, eIcode.ic.ll.dst.regi, E_DEF);
            setRegDU (&eIcode, rTMP, E_USE);
            eIcode.ic.ll.flg |= SYNTHETIC;
            eIcode.ic.ll.label = SynthLab++;
            pIcode = pProc->Icode.addIcode(&eIcode);
        } 
        else
            pIcode = pProc->Icode.addIcode(&Icode);

        switch (Icode.ic.ll.opcode) {
        /*** Conditional jumps ***/
        case iLOOP: case iLOOPE:    case iLOOPNE:
        case iJB:   case iJBE:      case iJAE:  case iJA:
        case iJL:   case iJLE:      case iJGE:  case iJG:
        case iJE:   case iJNE:      case iJS:   case iJNS:
        case iJO:   case iJNO:      case iJP:   case iJNP:
        case iJCXZ:
        {   STATE   StCopy;
			int		ip		= pProc->Icode.GetNumIcodes()-1;	/* Index of this jump */
		    PICODE  prev    = pProc->Icode.GetIcode(ip-1);		/* Previous icode */
            boolT   fBranch = FALSE;

            pstate->JCond.regi = 0;

            /* This sets up range check for indexed JMPs hopefully
             * Handles JA/JAE for fall through and JB/JBE on branch
            */
            if (ip > 0 && prev->ic.ll.opcode == iCMP && (prev->ic.ll.flg & I))
            {
				pstate->JCond.immed = (int16)prev->ic.ll.immed.op;
				if (Icode.ic.ll.opcode == iJA || Icode.ic.ll.opcode == iJBE)
					pstate->JCond.immed++;
				if (Icode.ic.ll.opcode == iJAE || Icode.ic.ll.opcode == iJA)
					pstate->JCond.regi = prev->ic.ll.dst.regi;
				fBranch = (boolT)
           			(Icode.ic.ll.opcode == iJB || Icode.ic.ll.opcode == iJBE);
            }

            memcpy(&StCopy, pstate, sizeof(STATE));

            /* Straight line code */
            FollowCtrl (pProc, pcallGraph, &StCopy);

            if (fBranch)                /* Do branching code */
			{
                pstate->JCond.regi = prev->ic.ll.dst.regi;
			}
			/* Next icode. Note: not the same as GetLastIcode() because of the call
				to FollowCtrl() */
            pIcode = pProc->Icode.GetIcode(ip);
        }		/* Fall through to do the jump path */

        /*** Jumps ***/
        case iJMP:
        case iJMPF: /* Returns TRUE if we've run into a loop */
            done = process_JMP (pIcode, pProc, pstate, pcallGraph);
            break;

        /*** Calls ***/
        case iCALL:
        case iCALLF:
            done = process_CALL (pIcode, pProc, pcallGraph, pstate);
            break;

        /*** Returns ***/
        case iRET:
        case iRETF:
            pProc->flg |= (Icode.ic.ll.opcode == iRET)? PROC_NEAR:PROC_FAR;
                                        /* Fall through */
        case iIRET:
            pProc->flg &= ~TERMINATES;
            done = TRUE;
            break;

        case iINT:
            if (Icode.ic.ll.immed.op == 0x21 && pstate->f[rAH])
            {
               Int funcNum = pstate->r[rAH];
               Int operand;
               Int size;

               /* Save function number */
               pProc->Icode.GetIcode(pProc->Icode.GetNumIcodes() - 1)->ic.ll.dst.off = 
                                                                (int16)funcNum;

               /* Program termination: int21h, fn 00h, 31h, 4Ch */
               done  = (boolT)(funcNum == 0x00 || funcNum == 0x31 ||
                        funcNum == 0x4C);

               /* String functions: int21h, fn 09h */
               if (pstate->f[rDX])      /* offset goes into DX */
					if (funcNum == 0x09)
					{
						operand = ((dword)(word)pstate->r[rDS]<<4) +
							(dword)(word)pstate->r[rDX];
                   		size = prog.fCOM ?
                            strSize (&prog.Image[operand], '$') :
                            strSize (&prog.Image[operand + 0x100], '$');
                   		updateSymType (operand, TYPE_STR, size);
                 	}
            }
            else if ((Icode.ic.ll.immed.op == 0x2F) && (pstate->f[rAH]))
			{
				pProc->Icode.GetIcode(pProc->Icode.GetNumIcodes() - 1)->ic.ll.dst.off =
					pstate->r[rAH];
			}
            else    /* Program termination: int20h, int27h */
				done = (boolT)(Icode.ic.ll.immed.op == 0x20 ||
                                            Icode.ic.ll.immed.op == 0x27);
            if (done)
                pIcode->ic.ll.flg |= TERMINATES;
            break;

        case iMOV:
            process_MOV(pIcode, pstate);
            break;

        /* case iXCHG:
            process_MOV (pIcode, pstate);

            break; **** HERE ***/

        case iSHL:
            if (pstate->JCond.regi == Icode.ic.ll.dst.regi)
               if ((Icode.ic.ll.flg & I) && Icode.ic.ll.immed.op == 1)
                pstate->JCond.immed *= 2;
               else
                pstate->JCond.regi = 0;
            break;

        case iLEA:
            if (Icode.ic.ll.src.regi == 0)      /* direct mem offset */
               setState (pstate, Icode.ic.ll.dst.regi, Icode.ic.ll.src.off);
            break;

        case iLDS: case iLES:
            if ((psym = lookupAddr(&Icode.ic.ll.src, pstate, 4, USE))
                /* && (Icode.ic.ll.flg & SEG_IMMED) */ ) {
               offset = LH(&prog.Image[psym->label]);
               setState(pstate, (Icode.ic.ll.opcode == iLDS)? rDS: rES,
                        LH(&prog.Image[psym->label + 2]));
               setState(pstate, Icode.ic.ll.dst.regi, (int16)offset);
               psym->type = TYPE_PTR;
            }
            break;
        }
    }

    if (err) {
        pProc->flg &= ~TERMINATES;

        if (err == INVALID_386OP || err == INVALID_OPCODE)
        {
           fatalError(err, prog.Image[Icode.ic.ll.label], Icode.ic.ll.label);
           pProc->flg |= PROC_BADINST;
        }
        else if (err == IP_OUT_OF_RANGE)
           fatalError (err, Icode.ic.ll.label);
        else
           reportError(err, Icode.ic.ll.label);
    }
}


static boolT process_JMP (PICODE pIcode, PPROC pProc, PSTATE pstate, 
                          PCALL_GRAPH pcallGraph)
/* process_JMP - Handles JMPs, returns TRUE if we should end recursion  */
{   static byte i2r[4] = {rSI, rDI, rBP, rBX};
    ICODE       Icode;
    dword       cs, offTable, endTable;
    dword       i, k, seg, target;
    Int         tmp;

    if (pIcode->ic.ll.flg & I)
    {
        if (pIcode->ic.ll.opcode == iJMPF)
           setState(pstate, rCS, LH(prog.Image + pIcode->ic.ll.label + 3));
        i = pstate->IP = pIcode->ic.ll.immed.op;
        if ((long)i < 0)
        {
        	exit(1);
        }

        /* Return TRUE if jump target is already parsed */
        return pProc->Icode.labelSrch(i, &tmp);
    }

    /* We've got an indirect JMP - look for switch() stmt. idiom of the form
     *   JMP  word ptr  word_offset[rBX | rSI | rDI]        */
    seg = (pIcode->ic.ll.src.seg)? pIcode->ic.ll.src.seg: rDS;

    /* Ensure we have a word offset & valid seg */
    if (pIcode->ic.ll.opcode == iJMP && (pIcode->ic.ll.flg & WORD_OFF) &&
        pstate->f[seg] &&
        (pIcode->ic.ll.src.regi == INDEXBASE + 4 ||
         pIcode->ic.ll.src.regi == INDEXBASE + 5 || /* Idx reg. BX, SI, DI */
         pIcode->ic.ll.src.regi == INDEXBASE + 7))
    {

        offTable = ((dword)(word)pstate->r[seg] << 4) + pIcode->ic.ll.src.off;

        /* Firstly look for a leading range check of the form:-
         *      CMP {BX | SI | DI}, immed
         *      JA | JAE | JB | JBE
         * This is stored in the current state as if we had just
         * followed a JBE branch (i.e. [reg] lies between 0 - immed).
        */
        if (pstate->JCond.regi == i2r[pIcode->ic.ll.src.regi-(INDEXBASE+4)])
            endTable = offTable + pstate->JCond.immed;
        else
            endTable = (dword)prog.cbImage;

        /* Search for first byte flagged after start of table */
        for (i = offTable; i <= endTable; i++)
            if (BITMAP(i, BM_CODE | BM_DATA))
                break;
        endTable = i & ~1;      /* Max. possible table size */

        /* Now do some heuristic pruning.  Look for ptrs. into the table
         * and for addresses that don't appear to point to valid code.
        */
        cs = (dword)(word)pstate->r[rCS] << 4;
        for (i = offTable; i < endTable; i += 2)
        {
            target = cs + LH(&prog.Image[i]);
            if (target < endTable && target >= offTable)
                endTable = target;
            else if (target >= (dword)prog.cbImage)
                endTable = i;
        }

        for (i = offTable; i < endTable; i += 2)
        {
            target = cs + LH(&prog.Image[i]);
            /* Be wary of 00 00 as code - it's probably data */
            if (! (prog.Image[target] || prog.Image[target+1]) ||
                scan(target, &Icode))
                endTable = i;
        }

        /* Now for each entry in the table take a copy of the current
         * state and recursively call FollowCtrl(). */
        if (offTable < endTable)
        {
            STATE   StCopy;
            Int     ip;
            dword  *psw;
            
            setBits(BM_DATA, offTable, endTable - offTable);

            pIcode->ic.ll.flg |= SWITCH;
            pIcode->ic.ll.caseTbl.numEntries = (endTable - offTable) / 2;
            psw = (dword*)allocMem(pIcode->ic.ll.caseTbl.numEntries*sizeof(dword));
            pIcode->ic.ll.caseTbl.entries = psw;

            for (i = offTable, k = 0; i < endTable; i += 2)
            {
                memcpy(&StCopy, pstate, sizeof(STATE));
                StCopy.IP = cs + LH(&prog.Image[i]);
                ip = pProc->Icode.GetNumIcodes();

                FollowCtrl (pProc, pcallGraph, &StCopy);

                pProc->Icode.GetIcode(ip)->ic.ll.caseTbl.numEntries = k++;
                pProc->Icode.GetIcode(ip)->ic.ll.flg |= CASE;
                *psw++ = pProc->Icode.GetLlLabel(ip);
            }
            return TRUE;
        }
    }

    /* Can't do anything with this jump */

    pProc->flg |= PROC_IJMP;
    pProc->flg &= ~TERMINATES;
    interactDis(pProc, pProc->Icode.GetNumIcodes()-1);
    return TRUE;
}


static boolT process_CALL (PICODE pIcode, PPROC pProc, PCALL_GRAPH pcallGraph,
                           PSTATE pstate)
/* Process procedure call. 
 * Note: We assume that CALL's will return unless there is good evidence to 
 *       the contrary - thus we return FALSE unless all paths in the called 
 *       procedure end in DOS exits.  This is reasonable since C procedures 
 *       will always include the epilogue after the call anyway and it's to 
 *       be assumed that if an assembler program contains a CALL that the 
 *       programmer expected it to come back - otherwise surely a JMP would 
 *       have been used.  */
{ PPROC p, pPrev;
  Int   ip = pProc->Icode.GetNumIcodes() - 1;
  STATE localState;     /* Local copy of the machine state */
  dword off;
  boolT indirect;

	/* For Indirect Calls, find the function address */
	indirect = FALSE;
	if (! (pIcode->ic.ll.flg & I)) 
	{
		/* Not immediate, i.e. indirect call */

		if (pIcode->ic.ll.dst.regi && (!option.Calls))
		{
			/* We have not set the brave option to attempt to follow
				the execution path through register indirect calls.
				So we just exit this function, and ignore the call.
				We probably should not have parsed this deep, anyway.
			*/
			return FALSE;
		}

		/* Offset into program image is seg:off of read input */
		/* Note: this assumes that the pointer itself is at
			es:0 where es:0 is the start of the image. This is
			usually wrong! Consider also CALL [BP+0E] in which the
			segment for the pointer is in SS! - Mike */

		off = (dword)(word)pIcode->ic.ll.dst.off +
			((dword)(word)pIcode->ic.ll.dst.segValue << 4);

		/* Address of function is given by 4 (CALLF) or 2 (CALL) bytes at 
		 * previous offset into the program image */
		if (pIcode->ic.ll.opcode == iCALLF)
			pIcode->ic.ll.immed.op = LH(&prog.Image[off]) + 
				((dword)(LH(&prog.Image[off+2])) << 4);
		else
			pIcode->ic.ll.immed.op = LH(&prog.Image[off]) + 
									((dword)(word)pProc->state.r[rCS] << 4); 
		pIcode->ic.ll.flg |= I;
		indirect = TRUE;
	}

	/* Process CALL.  Function address is located in pIcode->ic.ll.immed.op */
    if (pIcode->ic.ll.flg & I) 
	{
        /* Search procedure list for one with appropriate entry point */
        for (p = pProcList; p && p->procEntry != pIcode->ic.ll.immed.op;
             p = p->next)
            pPrev = p;

        /* Create a new procedure node and save copy of the state */
        if (! p) {
			p = allocStruc(PROCEDURE);
			p = (PPROC)memset(p, 0, sizeof(PROCEDURE));
            pPrev->next = p;
            p->prev = pPrev;
            p->procEntry = pIcode->ic.ll.immed.op;
            pLastProc = p;      /* Pointer to last node in the list */

            LibCheck(p);

            if (p->flg & PROC_ISLIB)
            {
                /* A library function. No need to do any more to it */
                insertCallGraph (pcallGraph, pProc, p);
                pProc->Icode.GetIcode(ip)->ic.ll.immed.proc.proc = p;
                return FALSE;
            }

			if (indirect)
				p->flg |= PROC_ICALL;

            if (p->name[0] == '\0')     /* Don't overwrite existing name */
            {
                sprintf(p->name, "proc_%ld", ++prog.cProcs);
            }
            p->depth = pProc->depth + 1;
            p->flg |= TERMINATES;

            /* Save machine state in localState, load up IP and CS.*/
            memcpy (&localState, pstate, sizeof(STATE));
            pstate->IP = pIcode->ic.ll.immed.op;
            if (pIcode->ic.ll.opcode == iCALLF)
               setState (pstate, rCS, LH(prog.Image + pIcode->ic.ll.label + 3));
            memcpy(&(p->state), pstate, sizeof(STATE));

            /* Insert new procedure in call graph */
            insertCallGraph (pcallGraph, pProc, p);

            /* Process new procedure */
            FollowCtrl (p, pcallGraph, pstate);
            
            /* Restore segment registers & IP from localState */
            pstate->IP = localState.IP;
            setState (pstate, rCS, localState.r[rCS]);
            setState (pstate, rDS, localState.r[rDS]);
            setState (pstate, rES, localState.r[rES]);
            setState (pstate, rSS, localState.r[rSS]);

        }
        else
            insertCallGraph (pcallGraph, pProc, p);

        pProc->Icode.GetIcode(ip)->ic.ll.immed.proc.proc = p;   /* ^ target proc */
        /* return ((p->flg & TERMINATES) != 0); */
		return FALSE;
    }
    return FALSE;				// Cristina, please check!!
}


static void process_MOV(PICODE pIcode, PSTATE pstate)
/* process_MOV - Handles state changes due to simple assignments    */
{ PSYM  psym, psym2;        /* Pointer to symbol in global symbol table */
  byte  dstReg = pIcode->ic.ll.dst.regi;
  byte  srcReg = pIcode->ic.ll.src.regi;

    if (dstReg > 0 && dstReg < INDEXBASE)
    {
        if (pIcode->ic.ll.flg & I)
            setState(pstate, dstReg, (int16)pIcode->ic.ll.immed.op);
        else if (srcReg == 0)   /* direct memory offset */
        {
            psym = lookupAddr(&pIcode->ic.ll.src, pstate, 2, USE);
            if (psym && ((psym->flg & SEG_IMMED) || (psym->duVal & VAL)))
               setState(pstate, dstReg, LH(&prog.Image[psym->label]));
        }
        else if (srcReg < INDEXBASE && pstate->f[srcReg])  /* reg */
        {
            setState(pstate, dstReg, pstate->r[srcReg]);

            /* Follow moves of the possible index register */
            if (pstate->JCond.regi == srcReg)
                pstate->JCond.regi = dstReg;
        }
    }
    else if (dstReg == 0) {     /* direct memory offset */
        psym = lookupAddr (&pIcode->ic.ll.dst, pstate, 2, DEF);
        if (psym && ! (psym->duVal & VAL))      /* no initial value yet */
           if (pIcode->ic.ll.flg & I)   {       /* immediate */
            prog.Image[psym->label] = (byte)pIcode->ic.ll.immed.op;
            prog.Image[psym->label+1] = (byte)(pIcode->ic.ll.immed.op>>8);
            psym->duVal |= VAL;
           }
           else if (srcReg == 0) {      /* direct mem offset */
            psym2 = lookupAddr (&pIcode->ic.ll.src, pstate, 2, USE);
            if (psym2 && ((psym->flg & SEG_IMMED) || (psym->duVal & VAL)))
            {
               prog.Image[psym->label] = (byte)prog.Image[psym2->label];
               prog.Image[psym->label+1] =
                                (byte)(prog.Image[psym2->label+1] >> 8);
               psym->duVal |= VAL;
            }
           }
           else if (srcReg < INDEXBASE && pstate->f[srcReg])  /* reg */
           {
            prog.Image[psym->label] = (byte)pstate->r[srcReg];
            prog.Image[psym->label+1] = (byte)(pstate->r[srcReg] >> 8);
            psym->duVal |= VAL;
           }
    }
}


/* Type of the symbol according to the number of bytes it uses */ 
static hlType cbType[] = {TYPE_UNKNOWN, TYPE_BYTE_UNSIGN, TYPE_WORD_SIGN,
						  TYPE_UNKNOWN, TYPE_LONG_SIGN};

static PSYM updateGlobSym (dword operand, Int size, word duFlag)
/* Creates an entry in the global symbol table (symtab) if the variable
 * is not there yet.  If it is part of the symtab, the size of the variable
 * is checked and updated if the old size was less than the new size (ie.
 * the maximum size is always saved).   */
{   Int i;

    /* Check for symbol in symbol table */
    for (i = 0; i < symtab.csym; i++)
        if (symtab.sym[i].label == operand) {
            if (symtab.sym[i].size < size)
                symtab.sym[i].size = size;
            break;
        }

    /* New symbol, not in symbol table */
    if (i == symtab.csym) {
        if (++symtab.csym > symtab.alloc) {
            symtab.alloc += 5;
            symtab.sym = (PSYM)reallocVar(symtab.sym, symtab.alloc * sizeof(SYM));
            memset (&symtab.sym[i], 0, 5 * sizeof(SYM));
        }
        sprintf (symtab.sym[i].name, "var%05lX", operand);
        symtab.sym[i].label = operand;
        symtab.sym[i].size  = size;
        symtab.sym[i].type = cbType[size];
        if (duFlag == USE)  /* must already have init value */
            symtab.sym[i].duVal = USEVAL;
        else
            symtab.sym[i].duVal = duFlag;
    }
    return (&symtab.sym[i]);
}


static void updateFrameOff (PSTKFRAME ps, int16 off, Int size, word duFlag)
/* Updates the offset entry to the stack frame table (arguments),
 * and returns a pointer to such entry. */
{ Int   i;

    /* Check for symbol in stack frame table */
    for (i = 0; i < ps->csym; i++)
	{
        if (ps->sym[i].off == off)
		{
            if (ps->sym[i].size < size)
			{
                ps->sym[i].size = size;
			}
            break;
        }
	}

    /* New symbol, not in table */
    if (i == ps->csym)
	{
        if (++ps->csym > ps->alloc)
		{
            ps->alloc += 5;
            ps->sym = (STKSYM*)reallocVar(ps->sym, ps->alloc * sizeof(STKSYM));
			memset(&ps->sym[i], 0, 5*sizeof(STKSYM));
        }
        sprintf (ps->sym[i].name, "arg%ld", i); 
        ps->sym[i].off = off;
        ps->sym[i].size  = size;
        ps->sym[i].type = cbType[size];
        if (duFlag == USE)  /* must already have init value */
            ps->sym[i].duVal = USEVAL;
        else
            ps->sym[i].duVal = duFlag;
		ps->cb += size;
		ps->numArgs++;
    }

    /* Save maximum argument offset */
    if ((dword)ps->maxOff < (off + (dword)size))
        ps->maxOff = off + (int16)size;
}


static PSYM lookupAddr (PMEM pm, PSTATE pstate, Int size, word duFlag)
/* lookupAddr - Looks up a data reference in the symbol table and stores it 
 *      if necessary.  Returns a pointer to the symbol in the
 *      symbol table, or Null if it's not a direct memory offset.  */
{   Int     i;
    PSYM    psym;
    dword   operand;

    if (pm->regi == 0)  {       /* Global var */
        if (pm->segValue) { /* there is a value in the seg field */
            operand = opAdr (pm->segValue, pm->off);
            psym = updateGlobSym (operand, size, duFlag);

            /* Check for out of bounds */
            if (psym->label >= (dword)prog.cbImage)
               return (NULL);
            return (psym);
        }
        else if (pstate->f[pm->seg]) {      /* new value */
            pm->segValue = pstate->r[pm->seg];
            operand = opAdr(pm->segValue, pm->off);
            i = symtab.csym;
            psym = updateGlobSym (operand, size, duFlag);

            /* Flag new memory locations that are segment values */
            if (symtab.csym > i) {
                if (size == 4)
                    operand += 2;   /* High word */
                for (i = 0; i < prog.cReloc; i++)
                    if (prog.relocTable[i] == operand) {
                        psym->flg = SEG_IMMED;
                        break;
                    }
            }
            
            /* Check for out of bounds */
            if (psym->label >= (dword)prog.cbImage)
               return (NULL);
            return (psym);
        }
    }
    return (NULL);
}


void setState(PSTATE pstate, word reg, int16 value)
/* setState - Assigns a value to a reg.     */
{
    value &= 0xFFFF;
    pstate->r[reg] = value;
    pstate->f[reg] = TRUE;
    switch (reg) {
    case rAX: case rCX: case rDX: case rBX:
        pstate->r[reg + rAL - rAX] = value & 0xFF;
        pstate->f[reg + rAL - rAX] = TRUE;
        pstate->r[reg + rAH - rAX] = (value >> 8) & 0xFF;
        pstate->f[reg + rAH - rAX] = TRUE;
        break;

    case rAL: case rCL: case rDL: case rBL:
        if (pstate->f[reg - rAL + rAH]) {
            pstate->r[reg - rAL + rAX] =
                (pstate->r[reg - rAL + rAH] << 8) + (value & 0xFF);
            pstate->f[reg - rAL + rAX] = TRUE;
        }
        break;

    case rAH: case rCH: case rDH: case rBH:
        if (pstate->f[reg - rAH + rAL]) {
            pstate->r[reg - rAH + rAX] =
                pstate->r[reg - rAH + rAL] + ((value & 0xFF) << 8);
            pstate->f[reg - rAH + rAX] = TRUE;
        }
        break;
    }
}


boolT labelSrch(PICODE pIcode, Int numIp, dword target, Int *pIndex)
/* labelSrchRepl - Searches Icode for instruction with label = target, and
    replaces *pIndex with an icode index */
{
    Int  i;

    for (i = 0; i < numIp; i++)
    {
        if (pIcode[i].ic.ll.label == target)
        {
            *pIndex = i;
            return TRUE;
        }
    }
    return FALSE;
}


static void setBits(int16 type, dword start, dword len)
/* setBits - Sets memory bitmap bits for BM_CODE or BM_DATA (additively) */
{
    dword   i;

    if (start < (dword)prog.cbImage)
	{
        if (start + len > (dword)prog.cbImage)
            len = (dword)(prog.cbImage - start);

        for (i = start + len - 1; i >= start; i--)
		{
            prog.map[i >> 2] |= type << ((i & 3) << 1);
			if (i == 0) break;		// Fixes inf loop!
		}
    }
}


/* DU bit definitions for each reg value - including index registers */
dword duReg[] = { 0x00, 
            0x11001, 0x22002, 0x44004, 0x88008, /* word regs    */
            0x10, 0x20, 0x40, 0x80, 
            0x100, 0x200, 0x400, 0x800,         /* seg regs     */
            0x1000, 0x2000, 0x4000, 0x8000,     /* byte regs    */ 
            0x10000, 0x20000, 0x40000, 0x80000, 
            0x100000,                           /* tmp reg      */
            0x48, 0x88, 0x60, 0xA0,             /* index regs   */
            0x40, 0x80, 0x20, 0x08 };


static void use (opLoc d, PICODE pIcode, PPROC pProc, PSTATE pstate, Int size,
                 Int ix)
/* Checks which registers where used and updates the du.u flag.  
 * Places local variables on the local symbol table.    
 * Arguments: d     : SRC or DST icode operand
 *            pIcode: ptr to icode instruction
 *            pProc : ptr to current procedure structure
 *            pstate: ptr to current procedure state
 *            size  : size of the operand
 *            ix    : current index into icode array    */
{ PMEM  pm   = (d == SRC)? &pIcode->ic.ll.src: &pIcode->ic.ll.dst;
  PSYM  psym;

    if (pm->regi == 0 || pm->regi >= INDEXBASE) 
    {
        if (pm->regi == INDEXBASE + 6)      /* indexed on bp */
        {
            if (pm->off >= 2)
                updateFrameOff (&pProc->args, pm->off, size, USE);
            else if (pm->off < 0) 
                newByteWordStkId (&pProc->localId, TYPE_WORD_SIGN, pm->off, 0);
        }

        else if (pm->regi == INDEXBASE + 2 || pm->regi == INDEXBASE + 3)
            newByteWordStkId (&pProc->localId, TYPE_WORD_SIGN, pm->off,
                              (byte)((pm->regi == INDEXBASE + 2) ? rSI : rDI));

        else if ((pm->regi >= INDEXBASE + 4) && (pm->regi <= INDEXBASE + 7))
        {
            if ((pm->seg == rDS) && (pm->regi == INDEXBASE + 7))    /* bx */
            {
                if (pm->off > 0)    /* global indexed variable */
                    newIntIdxId (&pProc->localId, pm->segValue, pm->off, rBX,
                                 ix, TYPE_WORD_SIGN);
            }
            pIcode->du.use |= duReg[pm->regi];
        }

        else if (psym = lookupAddr(pm, pstate, size, USE))
        {
            setBits (BM_DATA, psym->label, (dword)size);
            pIcode->ic.ll.flg |= SYM_USE;
            pIcode->ic.ll.caseTbl.numEntries = psym - symtab.sym;
        }
    }

    /* Use of register */
    else if ((d == DST) || ((d == SRC) && (pIcode->ic.ll.flg & I) != I))
        pIcode->du.use |= duReg[pm->regi];
}


static void def (opLoc d, PICODE pIcode, PPROC pProc, PSTATE pstate, Int size,
                 Int ix)
/* Checks which registers were defined (ie. got a new value) and updates the
 * du.d flag.
 * Places local variables in the local symbol table.    */
{ PMEM  pm   = (d == SRC)? &pIcode->ic.ll.src: &pIcode->ic.ll.dst;
  PSYM  psym;

    if (pm->regi == 0 || pm->regi >= INDEXBASE) 
    {
        if (pm->regi == INDEXBASE + 6)      /* indexed on bp */
        {
            if (pm->off >= 2)
                updateFrameOff (&pProc->args, pm->off, size, DEF);
            else if (pm->off < 0)
                newByteWordStkId (&pProc->localId, TYPE_WORD_SIGN, pm->off, 0);
        }

        else if (pm->regi == INDEXBASE + 2 || pm->regi == INDEXBASE + 3)
        {
            newByteWordStkId (&pProc->localId, TYPE_WORD_SIGN, pm->off,
                              (byte)((pm->regi == INDEXBASE + 2) ? rSI : rDI));
        }

        else if ((pm->regi >= INDEXBASE + 4) && (pm->regi <= INDEXBASE + 7))
        {
            if ((pm->seg == rDS) && (pm->regi == INDEXBASE + 7))    /* bx */
            {
                if (pm->off > 0)        /* global var */
                    newIntIdxId (&pProc->localId, pm->segValue, pm->off, rBX,
                                 ix, TYPE_WORD_SIGN);
            }
            pIcode->du.use |= duReg[pm->regi];
        }

        else if (psym = lookupAddr(pm, pstate, size, DEF))
        {
            setBits(BM_DATA, psym->label, (dword)size);
            pIcode->ic.ll.flg |= SYM_DEF;
            pIcode->ic.ll.caseTbl.numEntries = psym - symtab.sym;
        }
    }

    /* Definition of register */
    else if ((d == DST) || ((d == SRC) && (pIcode->ic.ll.flg & I) != I))
    {
        pIcode->du.def |= duReg[pm->regi];
        pIcode->du1.numRegsDef++; 
    }
}


static void use_def(opLoc d, PICODE pIcode, PPROC pProc, PSTATE pstate, Int cb,
                    Int ix)
/* use_def - operand is both use and def'd.
 * Note: the destination will always be a register, stack variable, or global
 *       variable.      */
{
    PMEM    pm = (d == SRC)? &pIcode->ic.ll.src: &pIcode->ic.ll.dst;

    use (d, pIcode, pProc, pstate, cb, ix);

    if (pm->regi < INDEXBASE)                   /* register */
    {
        pIcode->du.def |= duReg[pm->regi];
        pIcode->du1.numRegsDef++; 
    }
}


static void process_operands(PICODE pIcode, PPROC pProc, PSTATE pstate, Int ix)
/* Set DU vector, local variables and arguments, and DATA bits in the
 * bitmap       */
{ Int   i;
  Int   sseg = (pIcode->ic.ll.src.seg)? pIcode->ic.ll.src.seg: rDS;
  Int   cb   = (pIcode->ic.ll.flg & B) ? 1: 2;
  flags32 Imm  = (pIcode->ic.ll.flg & I);

    switch (pIcode->ic.ll.opcode) {
        case iAND:  case iOR:   case iXOR:
        case iSAR:  case iSHL:  case iSHR:
        case iRCL:  case iRCR:  case iROL:  case iROR:
        case iADD:  case iADC:  case iSUB:  case iSBB:
            if (! Imm) {
                use(SRC, pIcode, pProc, pstate, cb, ix);
            }
        case iINC:  case iDEC:  case iNEG:  case iNOT:
        case iAAA:  case iAAD:  case iAAM:  case iAAS:
        case iDAA:  case iDAS:
            use_def(DST, pIcode, pProc, pstate, cb, ix);
            break;

        case iXCHG:
			/* This instruction is replaced by 3 instructions, only need
			 * to define the src operand and use the destination operand
			 * in the mean time.	*/
			use(SRC, pIcode, pProc, pstate, cb, ix);
			def(DST, pIcode, pProc, pstate, cb, ix);
            break;

        case iTEST: case iCMP:
            if (! Imm)
                use(SRC, pIcode, pProc, pstate, cb, ix);
            use(DST, pIcode, pProc, pstate, cb, ix);
            break;

        case iDIV:  case iIDIV:
            use(SRC, pIcode, pProc, pstate, cb, ix);
            if (cb == 1) 
                pIcode->du.use |= duReg[rTMP];
            break;

        case iMUL:  case iIMUL:
            use(SRC, pIcode, pProc, pstate, cb, ix);
            if (! Imm)
            {
                use (DST, pIcode, pProc, pstate, cb, ix);
                if (cb == 1)
                {
                    pIcode->du.def |= duReg[rAX]; 
                    pIcode->du1.numRegsDef++; 
                }
                else
                {
                    pIcode->du.def |= (duReg[rAX] | duReg[rDX]);
                    pIcode->du1.numRegsDef += 2; 
                }
            }
            else
                def (DST, pIcode, pProc, pstate, cb, ix);
            break;

        case iSIGNEX:
            cb = (pIcode->ic.ll.flg & SRC_B) ? 1 : 2;
            if (cb == 1)                    /* byte */
            {
                pIcode->du.def |= duReg[rAX];
                pIcode->du1.numRegsDef++; 
                pIcode->du.use |= duReg[rAL];
            }
            else                            /* word */
            {
                pIcode->du.def |= (duReg[rDX] | duReg[rAX]);
                pIcode->du1.numRegsDef += 2; 
                pIcode->du.use |= duReg[rAX];
            }
            break;

        case iCALLF:            /* Ignore def's on CS for now */
            cb = 4;
        case iCALL:  case iPUSH:  case iPOP:
            if (! Imm) {
                if (pIcode->ic.ll.opcode == iPOP)
                    def(DST, pIcode, pProc, pstate, cb, ix);
                else
                    use(DST, pIcode, pProc, pstate, cb, ix);
            }
            break;

        case iESC:  /* operands may be larger */
            use(DST, pIcode, pProc, pstate, cb, ix);
            break;

        case iLDS:  case iLES:
            pIcode->du.def |= duReg[(pIcode->ic.ll.opcode == iLDS) ?
                rDS : rES];
            pIcode->du1.numRegsDef++; 
            cb = 4;
        case iMOV:
            use(SRC, pIcode, pProc, pstate, cb, ix);
            def(DST, pIcode, pProc, pstate, cb, ix);
            break;

        case iLEA:
            use(SRC, pIcode, pProc, pstate, 2, ix);
            def(DST, pIcode, pProc, pstate, 2, ix);
            break;

        case iBOUND:
            use(SRC, pIcode, pProc, pstate, 4, ix);
            use(DST, pIcode, pProc, pstate, cb, ix);
            break;

        case iJMPF:
            cb = 4;
        case iJMP:
            if (! Imm)
                use(SRC, pIcode, pProc, pstate, cb, ix);
            break;

        case iLOOP:  case iLOOPE:  case iLOOPNE:
            pIcode->du.def |= duReg[rCX];
            pIcode->du1.numRegsDef++; 
        case iJCXZ:
            pIcode->du.use |= duReg[rCX];
            break;

        case iREPNE_CMPS: case iREPE_CMPS:  case iREP_MOVS:
            pIcode->du.def |= duReg[rCX];
            pIcode->du1.numRegsDef++; 
            pIcode->du.use |= duReg[rCX];
        case iCMPS:  case iMOVS:
            pIcode->du.def |= duReg[rSI] | duReg[rDI];
            pIcode->du1.numRegsDef += 2; 
            pIcode->du.use |= duReg[rSI] | duReg[rDI] | duReg[rES] |
                                   duReg[sseg];
            break;
            
        case iREPNE_SCAS: case iREPE_SCAS:  case iREP_STOS:  case iREP_INS:
            pIcode->du.def |= duReg[rCX];
            pIcode->du1.numRegsDef++; 
            pIcode->du.use |= duReg[rCX];
        case iSCAS:  case iSTOS:  case iINS:
            pIcode->du.def |= duReg[rDI];
            pIcode->du1.numRegsDef++; 
            if (pIcode->ic.ll.opcode == iREP_INS || pIcode->ic.ll.opcode== iINS)
            {
                pIcode->du.use |= duReg[rDI] | duReg[rES] | duReg[rDX];
            }
            else
            {
                pIcode->du.use |= duReg[rDI] | duReg[rES] |
                                       duReg[(cb == 2)? rAX: rAL];
            }
            break;

        case iREP_LODS:
            pIcode->du.def |= duReg[rCX];
            pIcode->du1.numRegsDef++; 
            pIcode->du.use |= duReg[rCX];
        case iLODS:
            pIcode->du.def |= duReg[rSI] | duReg[(cb==2)? rAX: rAL];
            pIcode->du1.numRegsDef += 2; 
            pIcode->du.use |= duReg[rSI] | duReg[sseg];
            break;

        case iREP_OUTS:
            pIcode->du.def |= duReg[rCX];
            pIcode->du1.numRegsDef++; 
            pIcode->du.use |= duReg[rCX];
        case iOUTS:
            pIcode->du.def |= duReg[rSI];
            pIcode->du1.numRegsDef++; 
            pIcode->du.use |= duReg[rSI] | duReg[rDX] | duReg[sseg];
            break;

        case iIN:  case iOUT:
            def(DST, pIcode, pProc, pstate, cb, ix);
            if (! Imm)
            {
                pIcode->du.use |= duReg[rDX];
            }
            break;
    }

    for (i = rSP; i <= rBH; i++)        /* Kill all defined registers */
        if (pIcode->ic.ll.flagDU.d & (1 << i))
            pstate->f[i] = FALSE;
}

