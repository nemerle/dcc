/*****************************************************************************
 * File: comwrite.c
 * Purpose: writes comments about C programs and descriptions about dos
 *	    interrupts in the string line given.
 * Project: dcc
 * (C) Cristina Cifuentes
 ****************************************************************************/

#include "dcc.h"
#include <string.h>
#include <sstream>
using namespace std;
#define intSize		40

static const char *int21h[] =
{
    "Terminate process",
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


static const char *intOthers[] = {
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


/* Writes the description of the current interrupt. Appends it to the
 * string s.	*/
void ICODE::writeIntComment (std::ostringstream &s)
{
    s<<"\t/* ";
    if (ic.ll.immed.op == 0x21)
    {
        s <<int21h[ic.ll.dst.off];
    }
    else if (ic.ll.immed.op > 0x1F && ic.ll.immed.op < 0x2F)
    {
        s <<intOthers[ic.ll.immed.op - 0x20];
    }
    else if (ic.ll.immed.op == 0x2F)
    {
        switch (ic.ll.dst.off)
        {
            case 0x01 :
                s << "Print spooler";
                break;
            case 0x02:
                s << "Assign";
                break;
            case 0x10:
                s << "Share";
                break;
            case 0xB7:
                s << "Append";
        }
    }
    else
        s<<"Unknown int";
    s<<" */\n";
}


//, &cCode.decl
void Function::writeProcComments()
{
    int i;
    ID *id;			/* Pointer to register argument identifier */
    STKSYM * psym;		/* Pointer to register argument symbol */

    /* About the parameters */
    if (this->cbParam)
        cCode.appendDecl("/* Takes %d bytes of parameters.\n",this->cbParam);
    else if (this->flg & REG_ARGS)
    {
        cCode.appendDecl("/* Uses register arguments:\n");
        for (i = 0; i < this->args.numArgs; i++)
        {
            psym = &this->args.sym[i];
            if (psym->regs->expr.ident.idType == REGISTER)
            {
                id = &this->localId.id_arr[psym->regs->expr.ident.idNode.regiIdx];
                if (psym->regs->expr.ident.regiType == WORD_REG)
                    cCode.appendDecl(" *     %s = %s.\n", psym->name,
                                     wordReg[id->id.regi - rAX]);
                else		/* BYTE_REG */
                    cCode.appendDecl(" *     %s = %s.\n", psym->name,
                                     byteReg[id->id.regi - rAL]);
            }
            else		/* long register */
            {
                id = &this->localId.id_arr[psym->regs->expr.ident.idNode.longIdx];
                cCode.appendDecl(" *     %s = %s:%s.\n", psym->name,
                                 wordReg[id->id.longId.h - rAX],
                                 wordReg[id->id.longId.l - rAX]);
            }

        }
    }
    else
        cCode.appendDecl("/* Takes no parameters.\n");

    /* Type of procedure */
    if (this->flg & PROC_RUNTIME)
        cCode.appendDecl(" * Runtime support routine of the compiler.\n");
    if (this->flg & PROC_IS_HLL)
        cCode.appendDecl(" * High-level language prologue code.\n");
    if (this->flg & PROC_ASM)
    {
        cCode.appendDecl(" * Untranslatable routine.  Assembler provided.\n");
        if (this->flg & PROC_IS_FUNC)
            switch (this->retVal.type) {
                case TYPE_BYTE_SIGN: case TYPE_BYTE_UNSIGN:
                    cCode.appendDecl(" * Return value in register al.\n");
                    break;
                case TYPE_WORD_SIGN: case TYPE_WORD_UNSIGN:
                    cCode.appendDecl(" * Return value in register ax.\n");
                    break;
                case TYPE_LONG_SIGN: case TYPE_LONG_UNSIGN:
                    cCode.appendDecl(" * Return value in registers dx:ax.\n");
                    break;
            } /* eos */
    }

    /* Calling convention */
    if (this->flg & CALL_PASCAL)
        cCode.appendDecl(" * Pascal calling convention.\n");
    else if (this->flg & CALL_C)
        cCode.appendDecl(" * C calling convention.\n");
    else if (this->flg & CALL_UNKNOWN)
        cCode.appendDecl(" * Unknown calling convention.\n");

    /* Other flags */
    if (this->flg & (PROC_BADINST | PROC_IJMP))
        cCode.appendDecl(" * Incomplete due to an %s.\n",
                         (this->flg & PROC_BADINST)? "untranslated opcode":
                                                     "indirect JMP");
    if (this->flg & PROC_ICALL)
        cCode.appendDecl(" * Indirect call procedure.\n");
    if (this->flg & IMPURE)
        cCode.appendDecl(" * Contains impure code.\n");
    if (this->flg & NOT_HLL)
        cCode.appendDecl(" * Contains instructions not normally used by compilers.\n");
    if (this->flg & FLOAT_OP)
        cCode.appendDecl(" * Contains coprocessor instructions.\n");

    /* Graph reducibility */
    if (this->flg & GRAPH_IRRED)
        cCode.appendDecl(" * Irreducible control flow graph.\n");
    cCode.appendDecl(" */\n{\n");
}

