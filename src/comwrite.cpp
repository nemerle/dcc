/*****************************************************************************
 * File: comwrite.c
 * Purpose: writes comments about C programs and descriptions about dos
 *	    interrupts in the string line given.
 * Project: dcc
 * (C) Cristina Cifuentes
 ****************************************************************************/

#include "dcc.h"
#include <string.h>
#define intSize		40

static char *int21h[] =
       {"Terminate process",
	"Character input with echo",
	"Character output",
	"Auxiliary input",
	"Auxiliary output",
	"Printer output",
	"Direct console i/o",
	"Unfiltered char i w/o echo",
	"Character input without echo",
	"Display string",
	"Buffered keyboard input",
	"Check input status",
	"Flush input buffer and then input",
	"Disk reset",
	"Select disk",
	"Open file",
	"Close file",
	"Find first file",
	"Find next file",
	"Delete file",
	"Sequential read",
	"Sequential write",
	"Create file",
	"Rename file",
	"Reserved",
	"Get current disk",
	"Set DTA address",
	"Get default drive data",
	"Get drive data",
	"Reserved",
	"Reserved",
	"Reserved",
	"Reserved",
	"Random read",
	"Random write",
	"Get file size",
	"Set relative record number",
	"Set interrupt vector",
	"Create new PSP",
	"Random block read",
	"Random block write",
	"Parse filename",
	"Get date",
	"Set date",
	"Get time",
	"Set time",
	"Set verify flag",
	"Get DTA address",
	"Get MSDOS version number",
	"Terminate and stay resident",
	"Reserved",
	"Get or set break flag",
	"Reserved",
	"Get interrupt vector",
	"Get drive allocation info",
	"Reserved",
	"Get or set country info",
	"Create directory",
	"Delete directory",
	"Set current directory",
	"Create file",
	"Open file",
	"Close file",
	"Read file or device",
	"Write file or device",
	"Delete file",
	"Set file pointer",
	"Get or set file attributes",
	"IOCTL (i/o control)",
	"Duplicate handle",
	"Redirect handle",
	"Get current directory",
	"Alloate memory block",
	"Release memory block",
	"Resize memory block",
	"Execute program (exec)",
	"Terminate process with return code",
	"Get return code",
	"Find first file",
	"Find next file",
	"Reserved",
	"Reserved",
	"Reserved",
	"Reserved",
	"Get verify flag",
	"Reserved",
	"Rename file",
	"Get or set file date & time",
	"Get or set allocation strategy",
	"Get extended error information",
	"Create temporary file",
	"Create new file",
	"Lock or unlock file region",
	"Reserved",
	"Get machine name",
	"Device redirection",
	"Reserved",
	"Reserved",
	"Get PSP address",
	"Get DBCS lead byte table",
	"Reserved",
	"Get extended country information",
	"Get or set code page",
	"Set handle count",
	"Commit file",
	"Reserved",
	"Reserved",
	"Reserved",
	"Extended open file"
};


static char *intOthers[] = {
	"Exit",					/* 0x20 */
	"",					/* other table */
	"Terminate handler address",		/* 0x22 */
	"Ctrl-C handler address",		/* 0x23 */
	"Critical-error handler address",	/* 0x24 */
	"Absolute disk read",			/* 0x25 */
	"Absolute disk write",			/* 0x26 */
	"Terminate and stay resident",		/* 0x27 */
	"Reserved",				/* 0x28 */
	"Reserved",				/* 0x29 */
	"Reserved",				/* 0x2A */
	"Reserved",				/* 0x2B */
	"Reserved",				/* 0x2C */
	"Reserved",				/* 0x2D */
	"Reserved"				/* 0x2E */
};


void writeIntComment (PICODE icode, char *s)
/* Writes the description of the current interrupt. Appends it to the 
 * string s.	*/
{ char *t;

	t = (char *)allocMem(intSize * sizeof(char));
	if (icode->ic.ll.immed.op == 0x21)
	{  sprintf (t, "\t/* %s */\n", int21h[icode->ic.ll.dst.off]);
	   strcat (s, t);
	}
	else if (icode->ic.ll.immed.op > 0x1F && icode->ic.ll.immed.op < 0x2F)
	{
	   sprintf (t, "\t/* %s */\n", intOthers[icode->ic.ll.immed.op - 0x20]);
	   strcat (s, t);
    }
	else if (icode->ic.ll.immed.op == 0x2F)
	   switch (icode->ic.ll.dst.off) {
		case 0x01 : strcat (s, "\t/* Print spooler */\n");
			break;
		case 0x02:  strcat (s, "\t/* Assign */\n");
			break;
		case 0x10:  strcat (s, "\t/* Share */\n");
			break;
		case 0xB7:  strcat (s, "\t/* Append */\n");
	   }
	else
	   strcat (s, "\n");
}


void writeProcComments (PPROC p, strTable *strTab)
{ int i;
  ID *id;			/* Pointer to register argument identifier */
  PSTKSYM psym;		/* Pointer to register argument symbol */

	/* About the parameters */
	if (p->cbParam)	
		appendStrTab (strTab, "/* Takes %d bytes of parameters.\n",
			p->cbParam);
	else if (p->flg & REG_ARGS)
	{
		appendStrTab (strTab, "/* Uses register arguments:\n");
		for (i = 0; i < p->args.numArgs; i++)
		{
			psym = &p->args.sym[i];
			if (psym->regs->expr.ident.idType == REGISTER)
			{
				id = &p->localId.id[psym->regs->expr.ident.idNode.regiIdx];
				if (psym->regs->expr.ident.regiType == WORD_REG)
					appendStrTab (strTab, " *     %s = %s.\n", psym->name, 
								  wordReg[id->id.regi - rAX]);	
				else		/* BYTE_REG */
					appendStrTab (strTab, " *     %s = %s.\n", psym->name, 
								  byteReg[id->id.regi - rAL]);	
			}
			else		/* long register */
			{
				id = &p->localId.id[psym->regs->expr.ident.idNode.longIdx];
				appendStrTab (strTab, " *     %s = %s:%s.\n", psym->name, 
							  wordReg[id->id.longId.h - rAX],
							  wordReg[id->id.longId.l - rAX]);
			}
			
		}
	}
	else
		appendStrTab (strTab, "/* Takes no parameters.\n");

	/* Type of procedure */
	if (p->flg & PROC_RUNTIME)
		appendStrTab (strTab," * Runtime support routine of the compiler.\n");
	if (p->flg & PROC_IS_HLL) 
		appendStrTab (strTab," * High-level language prologue code.\n");
	if (p->flg & PROC_ASM)
	{
		appendStrTab (strTab,
					  " * Untranslatable routine.  Assembler provided.\n");
		if (p->flg & PROC_IS_FUNC)
			switch (p->retVal.type) {
			  case TYPE_BYTE_SIGN: case TYPE_BYTE_UNSIGN:
				appendStrTab (strTab, " * Return value in register al.\n");
				break;
			  case TYPE_WORD_SIGN: case TYPE_WORD_UNSIGN:
				appendStrTab (strTab, " * Return value in register ax.\n");
				break;
			  case TYPE_LONG_SIGN: case TYPE_LONG_UNSIGN:
				appendStrTab (strTab, " * Return value in registers dx:ax.\n");
				break;
			} /* eos */
	}

	/* Calling convention */
	if (p->flg & CALL_PASCAL)
		appendStrTab (strTab, " * Pascal calling convention.\n");
	else if (p->flg & CALL_C)
		appendStrTab (strTab, " * C calling convention.\n");
	else if (p->flg & CALL_UNKNOWN)
		appendStrTab (strTab, " * Unknown calling convention.\n");

	/* Other flags */
	if (p->flg & (PROC_BADINST | PROC_IJMP)) 
		appendStrTab (strTab, " * Incomplete due to an %s.\n",
					(p->flg & PROC_BADINST)? "untranslated opcode":
											 "indirect JMP");
	if (p->flg & PROC_ICALL)
		appendStrTab (strTab, " * Indirect call procedure.\n");
	if (p->flg & IMPURE)
		appendStrTab (strTab, " * Contains impure code.\n");
	if (p->flg & NOT_HLL)
		appendStrTab (strTab,
			" * Contains instructions not normally used by compilers.\n");
	if (p->flg & FLOAT_OP)
			appendStrTab (strTab," * Contains coprocessor instructions.\n");

	/* Graph reducibility */
	if (p->flg & GRAPH_IRRED)
		appendStrTab (strTab," * Irreducible control flow graph.\n");
	appendStrTab (strTab, " */\n{\n");
}

