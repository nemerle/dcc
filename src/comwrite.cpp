/*****************************************************************************
 * File: comwrite.c
 * Purpose: writes comments about C programs and descriptions about dos
 *	    interrupts in the string line given.
 * Project: dcc
 * (C) Cristina Cifuentes
 ****************************************************************************/

#include "dcc.h"
#include "machine_x86.h"
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
    "Get DBCS lead uint8_t table",
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
void LLInst::writeIntComment (std::ostringstream &s)
{
    uint32_t src_immed=src().getImm2();
    s<<"\t/* ";
    if (src_immed == 0x21)
    {
        s <<int21h[m_dst.off];
    }
    else if (src_immed > 0x1F && src_immed < 0x2F)
    {
        s <<intOthers[src_immed - 0x20];
    }
    else if (src_immed == 0x2F)
    {
        switch (m_dst.off)
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
    std::ostringstream ostr;
    writeProcComments(ostr);
    cCode.appendDecl(ostr.str());
}

void Function::writeProcComments(std::ostream &ostr)
{
    int i;
    ID *id;			/* Pointer to register argument identifier */
    STKSYM * psym;		/* Pointer to register argument symbol */

    /* About the parameters */
    if (this->cbParam)
        ostr << "/* Takes "<<this->cbParam<<" bytes of parameters.\n";
    else if (this->flg & REG_ARGS)
    {
        ostr << "/* Uses register arguments:\n";
        for (i = 0; i < this->args.numArgs; i++)
        {
            psym = &this->args[i];
            ostr << " *     "<<psym->name<<" = ";
            if (psym->regs->ident.type() == REGISTER)
            {
                id = &this->localId.id_arr[((RegisterNode *)psym->regs)->regiIdx];
                ostr << Machine_X86::regName(id->id.regi);
            }
            else		/* long register */
            {
                id = &this->localId.id_arr[psym->regs->ident.idNode.longIdx];
                ostr << Machine_X86::regName(id->longId().h()) << ":";
                ostr << Machine_X86::regName(id->longId().l());
            }
            ostr << ".\n";

        }
    }
    else
        ostr << "/* Takes no parameters.\n";

    /* Type of procedure */
    if (this->flg & PROC_RUNTIME)
        ostr << " * Runtime support routine of the compiler.\n";
    if (this->flg & PROC_IS_HLL)
        ostr << " * High-level language prologue code.\n";
    if (this->flg & PROC_ASM)
    {
        ostr << " * Untranslatable routine.  Assembler provided.\n";
        if (this->flg & PROC_IS_FUNC)
            switch (this->retVal.type) { // TODO: Functions return value in various regs
                case TYPE_BYTE_SIGN: case TYPE_BYTE_UNSIGN:
                    ostr << " * Return value in register al.\n";
                    break;
                case TYPE_WORD_SIGN: case TYPE_WORD_UNSIGN:
                    ostr << " * Return value in register ax.\n";
                    break;
                case TYPE_LONG_SIGN: case TYPE_LONG_UNSIGN:
                    ostr << " * Return value in registers dx:ax.\n";
                    break;
                default:
                    fprintf(stderr,"Unknown retval type %d",this->retVal.type);
                    break;
            } /* eos */
    }

    /* Calling convention */
    if (this->flg & CALL_PASCAL)
        ostr << " * Pascal calling convention.\n";
    else if (this->flg & CALL_C)
        ostr << " * C calling convention.\n";
    else if (this->flg & CALL_UNKNOWN)
        ostr << " * Unknown calling convention.\n";

    /* Other flags */
    if (this->flg & (PROC_BADINST | PROC_IJMP))
    {
        ostr << " * Incomplete due to an ";
        if(this->flg & PROC_BADINST)
            ostr << "untranslated opcode.\n";
        else
            ostr << "indirect JMP.\n";
    }
    if (this->flg & PROC_ICALL)
        ostr << " * Indirect call procedure.\n";
    if (this->flg & IMPURE)
        ostr << " * Contains impure code.\n";
    if (this->flg & NOT_HLL)
        ostr << " * Contains instructions not normally used by compilers.\n";
    if (this->flg & FLOAT_OP)
        ostr << " * Contains coprocessor instructions.\n";

    /* Graph reducibility */
    if (this->flg & GRAPH_IRRED)
        ostr << " * Irreducible control flow graph.\n";
    ostr << " */\n{\n";
}
