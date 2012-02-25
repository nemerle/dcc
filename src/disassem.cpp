/****************************************************************************
 *          dcc project disassembler
 * (C) Cristina Cifuentes, Mike van Emmerik, Jeff Ledermann
 ****************************************************************************/
#include <stdint.h>
#include "dcc.h"
#include "symtab.h"
#include <stdio.h>
#include <string.h>
#include <malloc.h>		/* For free() */
#include <vector>
#include <sstream>
#include <iomanip>
#ifdef _CONSOLE
#include <windows.h>	/* For console mode routines */
#endif
#include "disassem.h"
// Note: for the time being, there is no interactive disassembler
// for unix
#ifndef __UNIX__
#include <conio.h>	// getch() etc
#endif
using namespace std;


#define POS_LAB     15              /* Position of label */
#define POS_OPC     20              /* Position of opcode */
#define POS_OPR     25              /* Position of operand */
#define	WID_PTR		10				/* Width of the "xword ptr" lingo */
#define POS_OPR2    POS_OPR+WID_PTR /* Position of operand after "xword ptr" */
#define POS_CMT     54              /* Position of comment */


#define DELTA_ICODE 16              /* Number of icodes to realloc by each time */

static const char *szOps[] =
{
    "CBW",  "AAA",      "AAD",      "AAM",      "AAS",      "ADC",  "ADD",  "AND",
    "BOUND","CALL",     "CALL",     "CLC",      "CLD",      "CLI",  "CMC",  "CMP",
    "CMPS", "REPNE CMPS","REPE CMPS","DAA",     "DAS",      "DEC",  "DIV",  "ENTER",
    "ESC",  "HLT",      "IDIV",     "IMUL",     "IN",       "INC",  "INS",  "REP INS",
    "INT",  "IRET",     "JB",       "JBE",      "JAE",      "JA",   "JE",   "JNE",
    "JL",   "JGE",      "JLE",      "JG",       "JS",       "JNS",  "JO",   "JNO",
    "JP",   "JNP",      "JCXZ",     "JMP",      "JMP",      "LAHF", "LDS",  "LEA",
    "LEAVE","LES",      "LOCK",     "LODS",     "REP LODS", "LOOP", "LOOPE","LOOPNE",
    "MOV",  "MOVS",     "REP MOVS", "MUL",      "NEG",      "NOT",  "OR",   "OUT",
    "OUTS", "REP OUTS", "POP",      "POPA",     "POPF",     "PUSH", "PUSHA","PUSHF",
    "RCL",  "RCR",      "ROL",      "ROR",      "RET",      "RETF", "SAHF", "SAR",
    "SHL",  "SHR",      "SBB",      "SCAS",     "REPNE SCAS","REPE SCAS",   "CWD",  "STC",
    "STD",  "STI",      "STOS",     "REP STOS", "SUB",      "TEST", "WAIT", "XCHG",
    "XLAT", "XOR",      "INTO",     "NOP",		"REPNE",	"REPE",	"MOD"
};

/* The following opcodes are for mod != 3 */
static const char *szFlops1[] =
{
    /* 0        1        2       3        4        5        6        7  */
    "FADD",  "FMUL",  "FCOM", "FCOMP", "FSUB",  "FSUBR", "FDIV",  "FDIVR",  /* 00 */
    "FLD",   "???",   "FST",  "???",   "FLDENV","FLDCW", "FSTENV","FSTSW",  /* 08 */
    "FIADD", "FIMUL", "FICOM","FICOMP","FISUB", "FISUBR","FIDIV", "FIDIVR", /* 10 */
    "FILD",  "???",   "FIST", "FISTP", "???",   "???",   "???",   "FSTP",   /* 18 */
    "FADD",  "FMUL",  "FCOM", "FCOMP", "FSUB",  "FSUBR", "FDIV",  "FDIVR",  /* 20 */
    "FLD",   "FLD",   "FST",  "FSTP",  "FRESTOR","???",  "FSAVE", "FSTSW",  /* 28 */
    "FIADD", "FIMUL", "FICOM","FICOMP","FISUB", "FISUBR","FIDIV", "FIDIVR", /* 30 */
    "FILD",  "???",   "FIST", "FISTP", "FBLD",  "???",   "FBSTP", "FISTP"   /* 38 */
};

/* The following opcodes are for mod == 3 */
static const char *szFlops2[] =
{
    /* 0        1        2       3        4        5        6        7  */
    "FADD",  "FMUL",  "FCOM", "FCOMP", "FSUB",  "FSUBR", "FDIV",  "FDIVR",  /* 00 */
    "FLD",   "FXCH",  "FNOP", "???",   "",      "",      "",      "",       /* 08 */
    "FIADD", "FIMUL", "FICOM","FICOMP","FISUB", "",      "FIDIV", "FIDIVR", /* 10 */
    "FILD",  "???",   "FIST", "FISTP", "???",   "???",   "???",   "FSTP",   /* 18 */
    "FADD",  "FMUL",  "FCOM", "FCOMP", "FSUB",  "FSUBR", "FDIV",  "FDIVR",  /* 20 */
    "FFREE", "FSTP",  "FST",  "???",   "FUCOM", "FUCOMP","???",   "???",    /* 28 */
    "FADDP", "FMULP", "FICOM","",      "FSUBRP","FISUBR","FDIVRP","FDIVP",  /* 30 */
    "FILD",  "???",   "FIST", "FISTP", "",      "???",   "FBSTP", "FISTP"   /* 38 */
};

static const char *szFlops0C[] =
{
    "FCHS",  "FABS",  "???",   "???",   "FTST", "FXAM",  "???",   "???"
};

static const char *szFlops0D[] =
{
    "FLD1",  "FLDL2T","FLDL2E","FLDP1", "FLDLG2","FLDLN2","FLDZ", "???"
};

static const char *szFlops0E[] =
{
    "F2XM1", "FYL2X", "FPTAN", "FPATAN","FXTRACT","FPREM1","FDECSTP","FINCSTP"
};

static const char *szFlops0F[] =
{
    "FPREM", "FYLXP1","FSQRT", "FSINCOS","FRNDINT","FSCALE","FSIN","FCOS"
};

static const char *szFlops15[] =
{
    "???",  "FUCOMPP",  "???", "???", "???", "???",  "???",   "???"
};

static const char *szFlops1C[] =
{
    "???",  "???",  "FCLEX", "FINIT", "FTST", "FXAM",  "???",   "???"
};

static const char *szFlops33[] =
{
    "???",  "FCOMPP",  "???", "???", "???", "???",  "???",   "???"
};

static const char *szFlops3C[] =
{
    "FSTSWAX","???",  "???", "???", "???", "???",  "???",   "???"
};


static const char *szIndex[8] = {"bx+si", "bx+di", "bp+si", "bp+di", "si", "di","bp","bx" };
static const char *szBreg[8]  = { "al", "cl", "dl", "bl", "ah", "ch", "dh", "bh" };
static const char *szWreg[12] = { "ax", "cx", "dx", "bx", "sp", "bp", "si", "di",
                                  "es", "cs", "ss", "ds" };
static const char *szPtr[2]   = { "word ptr ", "byte ptr " };


static void  dis1Line  (Int i, Int pass);
void  dis1LineOp(Int i, boolT fWin, char attr, word *len, Function * pProc);
static void  formatRM(ostringstream &p, flags32 flg, LLOperand *pm);
static ostringstream &strDst(ostringstream &os, flags32 flg, LLOperand *pm);
static ostringstream &strSrc(ostringstream &os, ICODE *pc, bool skip_comma=false);
static char *strHex(dword d);
static Int   checkScanned(dword pcCur);
static void  setProc(Function * proc);
static void  dispData(word dataSeg);
void flops(ICODE *pIcode,std::ostringstream &out);
boolT callArg(word off, char *temp);  /* Check for procedure name */

static  FILE   *fp;
static  CIcodeRec pc;
static std::ostringstream buf;
static  Int     cb, j, numIcode, allocIcode, eop;
static  vector<int> pl;
static  dword   nextInst;
static  boolT    fImpure;
static  Int     lab, prevPass;
static  Function *   pProc;          /* Points to current proc struct */

struct POSSTACK_ENTRY
{
    Int     ic;                 /* An icode offset */
    Function *   pProc;              /* A pointer to a PROCEDURE structure */
} ;
vector<POSSTACK_ENTRY> posStack; /* position stack */
byte              iPS;          /* Index into the stack */

static  char    cbuf[256];      /* Has to be 256 for wgetstr() to work */

// These are "curses equivalent" functions. (Used to use curses for all this,
// but it was too much of a distribution hassle

#define printfd(x) printf(x)
#define dis_newline() printf("\n")
#define dis_show()					// Nothing to do unless using Curses


/*****************************************************************************
 * disassem - Prints a disassembled listing of a procedure.
 *			  pass == 1 generates output on file .a1
 *			  pass == 2 generates output on file .a2
 *			  pass == 3 generates output on file .b
 ****************************************************************************/
void disassem(Int pass, Function * ppProc)
{
    Int         i;

    pProc = ppProc;             /* Save the passes pProc */
    if (pass != prevPass)
    {
        prevPass = pass;
        lab = 0; 	/* Restart label numbers */
    }
    createSymTables();
    allocIcode = numIcode = pProc->Icode.size();
    cb = allocIcode * sizeof(ICODE);
    if (numIcode == 0)
    {
        return;  /* No Icode */
    }

    /* Open the output file (.a1 or .a2 only) */
    if (pass != 3)
    {
        auto p = (pass == 1)? asm1_name: asm2_name;
        fp = fopen(p, "a+");
        if (!fp)
        {
            fatalError(CANNOT_OPEN, p);
        }
    }
    pc=pProc->Icode;
    /* Create temporary code array */
    // Mike: needs objectising!
    //pc = (ICODE *)memcpy(allocMem(cb), pProc->Icode.GetFirstIcode(), (size_t)cb);

    if (pass == 1)
    {
        /* Bind jump offsets to labels */
        for (i = 0; i < numIcode; i++)
        {
            if ((pc[i].ic.ll.flg & I) && !(pc[i].ic.ll.flg & JMP_ICODE) &&
                    JmpInst(pc[i].ic.ll.opcode))
            {
                /* Replace the immediate operand with an icode index */
                dword labTgt;
                if (pc.labelSrch(pc[i].ic.ll.src.op(),labTgt))
                {
                    pc[i].ic.ll.src.SetImmediateOp(labTgt);
                    /* This icode is the target of a jump */
                    pc[labTgt].ic.ll.flg |= TARGET;
                    pc[i].ic.ll.flg |= JMP_ICODE;   /* So its not done twice */
                }
                else
                {
                    /* This jump cannot be linked to a label */
                    pc[i].ic.ll.flg |= NO_LABEL;
                }
            }
        }
    }

    /* Create label array to keep track of location => label name */
    pl.clear();
    pl.resize(numIcode,0);

    /* Write procedure header */
    if (pass != 3)
        fprintf(fp, "\t\t%s  PROC  %s\n", pProc->name.c_str(), (pProc->flg & PROC_FAR)? "FAR": "NEAR");

    /* Loop over array printing each record */
    for (i = nextInst = 0; i < numIcode; i++)
    {
        dis1Line(i, pass);
    }

    /* Write procedure epilogue */
    if (pass != 3)
    {
        fprintf(fp, "\n\t\t%s  ENDP\n\n", pProc->name.c_str());
        fclose(fp);
    }

    pc.clear();
    destroySymTables();
}

/****************************************************************************
 * dis1Line() - disassemble one line to stream fp                           *                                   *
 * i is index into Icode for this proc                                      *
 * It is assumed that icode i is already scanned                            *
 ****************************************************************************/
static void dis1Line(Int i, Int pass)
{
    ostringstream oper_stream;
    ostringstream hex_bytes;
    ostringstream result_stream;
    ICODE * pIcode = &pc[i];
    oper_stream << uppercase;
    hex_bytes << uppercase;
    /* Disassembly stage 1 --
         * Do not try to display NO_CODE entries or synthetic instructions,
         * other than JMPs, that have been introduced for def/use analysis. */
    if ((option.asm1) &&
            ((pIcode->ic.ll.flg & NO_CODE) ||
             ((pIcode->ic.ll.flg & SYNTHETIC) && (pIcode->ic.ll.opcode != iJMP))))
    {
        return;
    }
    else if (pIcode->ic.ll.flg & NO_CODE)
    {
        return;
    }
    if (pIcode->ic.ll.flg & (TARGET | CASE))
    {
        if (pass == 3)
            cCode.appendCode("\n"); /* Print to c code buffer */
        else
            fprintf(fp, "\n");              /* No, print to the stream */
    }

    /* Find next instruction label and print hex bytes */
    if (pIcode->ic.ll.flg & SYNTHETIC)
        nextInst = pIcode->ic.ll.label;
    else
    {
        cb = (dword) pIcode->ic.ll.numBytes;
        nextInst = pIcode->ic.ll.label + cb;

        /* Output hexa code in program image */
        if (pass != 3)
        {
            for (j = 0; j < cb; j++)
            {
                hex_bytes << hex << setw(2) << setfill('0') << uint16_t(prog.Image[pIcode->ic.ll.label + j]);
            }
            hex_bytes << ' ';
        }
    }
    oper_stream << setw(POS_LAB) <<  left<< hex_bytes.str();
    /* Check if there is a symbol here */
    selectTable(Label);
    oper_stream << setw(5)<<left; // align for the labels
    {
        ostringstream lab_contents;
        if (readVal(lab_contents, pIcode->ic.ll.label, 0))
        {
            lab_contents << ':';             /* Also removes the null */
        }
        else if (pIcode->ic.ll.flg & TARGET)    /* Symbols override Lnn labels */
        {
            /* Print label */
            if (! pl[i])
            {
                pl[i] = ++lab;
            }
            lab_contents<< "L"<<pl[i]<<':';
        }
        oper_stream<< lab_contents.str();
    }
    if (pIcode->ic.ll.opcode == iSIGNEX && (pIcode->ic.ll.flg & B))
    {
        pIcode->ic.ll.opcode = iCBW;
    }
    oper_stream << setw(15) << left <<szOps[pIcode->ic.ll.opcode];

    switch (pIcode->ic.ll.opcode)
    {
        case iADD:  case iADC:  case iSUB:  case iSBB:  case iAND:  case iOR:
        case iXOR:  case iTEST: case iCMP:  case iMOV:  case iLEA:  case iXCHG:
            strDst(oper_stream,pIcode->ic.ll.flg, &pIcode->ic.ll.dst);
            strSrc(oper_stream,pIcode);
            break;

        case iESC:
            flops(pIcode,oper_stream);
            break;

        case iSAR:  case iSHL:  case iSHR:  case iRCL:  case iRCR:  case iROL:
        case iROR:
            strDst(oper_stream,pIcode->ic.ll.flg | I, &pIcode->ic.ll.dst);
            if(pIcode->ic.ll.flg & I)
                strSrc(oper_stream,pIcode);
            else
                oper_stream<<", cl";
            break;

        case iINC:  case iDEC:  case iNEG:  case iNOT:  case iPOP:
            strDst(oper_stream,pIcode->ic.ll.flg | I, &pIcode->ic.ll.dst);
            break;

        case iPUSH:
            if (pIcode->ic.ll.flg & I)
            {
                oper_stream<<strHex(pIcode->ic.ll.src.op());
//                strcpy(p + WID_PTR, strHex(pIcode->ic.ll.immed.op));
            }
            else
            {
                strDst(oper_stream,pIcode->ic.ll.flg | I, &pIcode->ic.ll.dst);
            }
            break;

        case iDIV:  case iIDIV:  case iMUL: case iIMUL: case iMOD:
            if (pIcode->ic.ll.flg & I)
            {
                strDst(oper_stream,pIcode->ic.ll.flg, &pIcode->ic.ll.dst) <<", ";
                formatRM(oper_stream, pIcode->ic.ll.flg, &pIcode->ic.ll.src);
                strSrc(oper_stream,pIcode);
            }
            else
                strDst(oper_stream,pIcode->ic.ll.flg | I, &pIcode->ic.ll.src);
            break;

        case iLDS:  case iLES:  case iBOUND:
            strDst(oper_stream,pIcode->ic.ll.flg, &pIcode->ic.ll.dst)<<", dword ptr";
            strSrc(oper_stream,pIcode,true);
            break;

        case iJB:  case iJBE:  case iJAE:  case iJA:
        case iJL:  case iJLE:  case iJGE:  case iJG:
        case iJE:  case iJNE:  case iJS:   case iJNS:
        case iJO:  case iJNO:  case iJP:   case iJNP:
        case iJCXZ:case iLOOP: case iLOOPE:case iLOOPNE:
        case iJMP: case iJMPF:

            /* Check if there is a symbol here */
            selectTable(Label);
            if ((pIcode->ic.ll.src.op() < (dword)numIcode) &&  /* Ensure in range */
                    readVal(oper_stream, pc[pIcode->ic.ll.src.op()].ic.ll.label, 0))
            {
                break;                          /* Symbolic label. Done */
            }

            if (pIcode->ic.ll.flg & NO_LABEL)
            {
                //strcpy(p + WID_PTR, strHex(pIcode->ic.ll.immed.op));
                oper_stream<<strHex(pIcode->ic.ll.src.op());
            }
            else if (pIcode->ic.ll.flg & I)
            {
                j = pIcode->ic.ll.src.op();
                if (! pl[j])       /* Forward jump */
                {
                    pl[j] = ++lab;
                }
                if (pIcode->ic.ll.opcode == iJMPF)
                {
                    oper_stream<<" far ptr ";
                }
                oper_stream<<"L"<<pl[j];
            }
            else if (pIcode->ic.ll.opcode == iJMPF)
            {
                oper_stream<<"dword ptr";
                strSrc(oper_stream,pIcode,true);
            }
            else
            {
                strDst(oper_stream,I, &pIcode->ic.ll.src);
            }
            break;

        case iCALL: case iCALLF:
            if (pIcode->ic.ll.flg & I)
            {
                if((pIcode->ic.ll.opcode == iCALL))
                    oper_stream<< "near";
                else
                    oper_stream<< " far";
                oper_stream<<" ptr "<<(pIcode->ic.ll.src.proc.proc)->name;
            }
            else if (pIcode->ic.ll.opcode == iCALLF)
            {
                oper_stream<<"dword ptr ";
                strSrc(oper_stream,pIcode,true);
            }
            else
                strDst(oper_stream,I, &pIcode->ic.ll.src);
            break;

        case iENTER:
            oper_stream<<strHex(pIcode->ic.ll.dst.off)<<", ";
            oper_stream<<strHex(pIcode->ic.ll.src.op());
            break;

        case iRET:  case iRETF:  case iINT:
            if (pIcode->ic.ll.flg & I)
            {
                oper_stream<<strHex(pIcode->ic.ll.src.op());
            }
            break;

        case iCMPS:  case iREPNE_CMPS:  case iREPE_CMPS:
        case iSCAS:  case iREPNE_SCAS:  case iREPE_SCAS:
        case iSTOS:  case iREP_STOS:
        case iLODS:  case iREP_LODS:
        case iMOVS:  case iREP_MOVS:
        case iINS:   case iREP_INS:
        case iOUTS:  case iREP_OUTS:
            if (pIcode->ic.ll.src.segOver)
            {
                bool is_dx_src=(pIcode->ic.ll.opcode == iOUTS || pIcode->ic.ll.opcode == iREP_OUTS);
                if(is_dx_src)
                    oper_stream<<"dx, "<<szPtr[pIcode->ic.ll.flg & B];
                else
                    oper_stream<<szPtr[pIcode->ic.ll.flg & B];
                if (pIcode->ic.ll.opcode == iLODS ||
                        pIcode->ic.ll.opcode == iREP_LODS ||
                        pIcode->ic.ll.opcode == iOUTS ||
                        pIcode->ic.ll.opcode == iREP_OUTS)
                {
                    oper_stream<<szWreg[pIcode->ic.ll.src.segOver-rAX];
                }
                else
                {
                    oper_stream<<"es:[di], "<<szWreg[pIcode->ic.ll.src.segOver - rAX];
                }
                oper_stream<<":[si]";
            }
            else
                oper_stream<<(pIcode->ic.ll.flg & B)? "B": "W";
            break;

        case iXLAT:
            if (pIcode->ic.ll.src.segOver)
            {
                oper_stream<<" "<<szPtr[1];
                oper_stream<<szWreg[pIcode->ic.ll.src.segOver-rAX]<<":[bx]";
            }
            break;

        case iIN:
            oper_stream<<(pIcode->ic.ll.flg & B)?"al, ": "ax, ";
            oper_stream<<(pIcode->ic.ll.flg & I)? strHex(pIcode->ic.ll.src.op()): "dx";
            break;

        case iOUT:
            oper_stream<<(pIcode->ic.ll.flg & I)? strHex(pIcode->ic.ll.src.op()): "dx";
            oper_stream<<(pIcode->ic.ll.flg & B)?", al": ", ax";
            break;

        default:
            break;
    }

    /* Comments */
    if (pIcode->ic.ll.flg & SYNTHETIC)
    {
        fImpure = FALSE;
    }
    else
    {
        for (j = pIcode->ic.ll.label, fImpure = 0; j > 0 && j < (Int)nextInst; j++)
        {
            fImpure |= BITMAP(j, BM_DATA);
        }
    }

    result_stream << setw(54) << left << oper_stream.str();
    /* Check for user supplied comment */
    selectTable(Comment);
    ostringstream cbuf;
    if (readVal(cbuf, pIcode->ic.ll.label, 0))
    {
        result_stream <<"; "<<cbuf.str();
    }
    else if (fImpure || (pIcode->ic.ll.flg & (SWITCH | CASE | SEG_IMMED | IMPURE | SYNTHETIC | TERMINATES)))
    {
        if (pIcode->ic.ll.flg & CASE)
        {
            result_stream << ";Case l"<< pIcode->ic.ll.caseTbl.numEntries;
        }
        if (pIcode->ic.ll.flg & SWITCH)
        {
            result_stream << ";Switch ";
        }
        if (fImpure)
        {
            result_stream << ";Accessed as data ";
        }
        if (pIcode->ic.ll.flg & IMPURE)
        {
            result_stream << ";Impure operand ";
        }
        if (pIcode->ic.ll.flg & SEG_IMMED)
        {
            result_stream << ";Segment constant";
        }
        if (pIcode->ic.ll.flg & TERMINATES)
        {
            result_stream << ";Exit to DOS";
        }
    }

    /* Comment on iINT icodes */
    if (pIcode->ic.ll.opcode == iINT)
        pIcode->writeIntComment (result_stream);

    /* Display output line */
    if(pass==3)
    {
        /* output to .b code buffer */
        if (pIcode->isLlFlag(SYNTHETIC))
            result_stream<<";Synthetic inst";
        if (pass == 3)		/* output to .b code buffer */
            cCode.appendCode("%s\n", result_stream.str().c_str());

    }
    else
    {
        if (! (pIcode->ic.ll.flg & SYNTHETIC))
        {
            /* output to .a1 or .a2 file */
            fprintf (fp, "%03ld %06lX %s\n", i, pIcode->ic.ll.label, result_stream.str().c_str());
        }
        else		/* SYNTHETIC instruction */
        {
            result_stream<<";Synthetic inst";
            fprintf (fp, "%03ld        %s\n", i, result_stream.str().c_str());
        }
    }
}



/****************************************************************************
 * formatRM
 ***************************************************************************/
static void formatRM(std::ostringstream &p, flags32 flg, LLOperand *pm)
{
    char    seg[4];

    if (pm->segOver)
    {
        strcat(strcpy(seg, szWreg[pm->segOver - rAX]), ":");
    }
    else    *seg = '\0';

    if (pm->regi == 0)
    {
        p<<seg<<"["<<strHex((dword)pm->off)<<"]";
    }

    else if (pm->regi == (INDEXBASE - 1))
    {
        p<<"tmp";
    }

    else if (pm->regi < INDEXBASE)
    {
        if(flg & B)
            p << szBreg[pm->regi - rAL];
        else
            p << szWreg[pm->regi - rAX];
    }

    else if (pm->off)
    {
        if (pm->off < 0)
        {
            p <<seg<<"["<<szIndex[pm->regi - INDEXBASE]<<"-"<<strHex((dword)(- pm->off))<<"]";
        }
        else
        {
            p <<seg<<"["<<szIndex[pm->regi - INDEXBASE]<<"+"<<strHex((dword)(pm->off))<<"]";
        }
    }
    else
        p <<seg<<"["<<szIndex[pm->regi - INDEXBASE]<<"]";
}


/*****************************************************************************
 * strDst
 ****************************************************************************/
static ostringstream & strDst(ostringstream &os,flags32 flg, LLOperand *pm)
{
    /* Immediates to memory require size descriptor */
    //os << setw(WID_PTR);
    if ((flg & I) && (pm->regi == 0 || pm->regi >= INDEXBASE))
        os << szPtr[flg & B];
    formatRM(os, flg, pm);
    return os;
}


/****************************************************************************
 * strSrc                                                                   *
 ****************************************************************************/
static ostringstream &strSrc(ostringstream &os,ICODE *pc,bool skip_comma)
{
    static char buf[30] = {", "};
    if(false==skip_comma)
        os<<", ";
    if (pc->ic.ll.flg & I)
        os<<strHex(pc->ic.ll.src.op());
    else if (pc->ic.ll.flg & IM_SRC)		/* level 2 */
        os<<"dx:ax";
    else
        formatRM(os, pc->ic.ll.flg, &pc->ic.ll.src);

    return os;
}


/****************************************************************************
 * strHex                                                                   *
 ****************************************************************************/
static char *strHex(dword d)
{
    static char buf[10];

    d &= 0xFFFF;
    sprintf(buf, "0%lX%s", d, (d > 9)? "h": "");
    return (buf + (buf[1] <= '9'));
}

/****************************************************************************
 *          interactDis - interactive disassembler                          *
 ****************************************************************************/
void interactDis(Function * initProc, Int initIC)
{
    printf("Sorry - interactive disasassembler option not available for Unix\n");
    return;
}

/* Handle the floating point opcodes (icode iESC) */
void flops(ICODE *pIcode,std::ostringstream &out)
{
    char bf[30];
    byte op = (byte)pIcode->ic.ll.src.op();

    /* Note that op is set to the escape number, e.g.
        esc 0x38 is FILD */

    if ((pIcode->ic.ll.dst.regi == 0) || (pIcode->ic.ll.dst.regi >= INDEXBASE))
    {
        /* The mod/rm mod bits are not set to 11 (i.e. register). This is the normal floating point opcode */
        out<<szFlops1[op]<<' ';
        out <<setw(10);
        if ((op == 0x29) || (op == 0x1F))
        {
            strcpy(bf, "tbyte ptr ");
        }
        else switch (op & 0x30)
        {
            case 0x00:
            case 0x10:
                strcpy(bf, "dword ptr ");
                break;
            case 0x20:
                strcpy(bf, "qword ptr ");
                break;
            case 0x30:
                switch (op)
                {
                    case 0x3C:       /* FBLD */
                    case 0x3E:       /* FBSTP */
                        strcpy(bf, "tbyte ptr ");
                        break;
                    case 0x3D:       /* FILD 64 bit */
                    case 0x3F:       /* FISTP 64 bit */
                        strcpy(bf, "qword ptr ");
                        break;

                    default:
                        strcpy(bf, "word  ptr ");
                        break;
                }
        }

        formatRM(out, pIcode->ic.ll.flg, &pIcode->ic.ll.dst);
    }
    else
    {
        /* The mod/rm mod bits are set to 11 (i.e. register).
           Could be specials (0x0C-0x0F, etc), or the st(i) versions of
                   normal opcodes. Because the opcodes are slightly different for
                   this case (e.g. op=04 means FSUB if reg != 3, but FSUBR for
                   reg == 3), a separate table is used (szFlops2). */

        switch (op)
        {
            case 0x0C:
                out << szFlops0C[pIcode->ic.ll.dst.regi - rAX];
                break;
            case 0x0D:
                out << szFlops0D[pIcode->ic.ll.dst.regi - rAX];
                break;
            case 0x0E:
                out << szFlops0E[pIcode->ic.ll.dst.regi - rAX];
                break;
            case 0x0F:
                out << szFlops0F[pIcode->ic.ll.dst.regi - rAX];
                break;
            case 0x15:
                out << szFlops15[pIcode->ic.ll.dst.regi - rAX];
                break;
            case 0x1C:
                out << szFlops1C[pIcode->ic.ll.dst.regi - rAX];
                break;
            case 0x33:
                out << szFlops33[pIcode->ic.ll.dst.regi - rAX];
                break;
            case 0x3C:
                out << szFlops3C[pIcode->ic.ll.dst.regi - rAX];
                break;
            default:
                out << szFlops2[op];
                if ((op >= 0x20) && (op <= 0x27))
                {
                    /* This is the ST(i), ST form. */
                    out << "ST("<<pIcode->ic.ll.dst.regi - rAX<<"),ST";
                }
                else
                {
                    /* ST, ST(i) */
                    out << "ST,ST("<<pIcode->ic.ll.dst.regi - rAX;
                }

                break;
        }
    }
}


