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
static const char *szPtr[2]   = { " word ptr ", " byte ptr " };


static void  dis1Line  (Int i, boolT fWin, char attr, Int pass);
void  dis1LineOp(Int i, boolT fWin, char attr, word *len, Function * pProc);
static void  formatRM(char *p, flags32 flg, ICODEMEM* pm);
static char *strDst(flags32 flg, ICODEMEM *pm);
static char *strSrc(ICODE * pc);
static char *strHex(dword d);
static Int   checkScanned(dword pcCur);
static void  setProc(Function * proc);
static void  dispData(word dataSeg);
static void  flops(ICODE * pi);
boolT callArg(word off, char *temp);  /* Check for procedure name */

static  FILE   *fp;
static  ICODE * pc;
static  char    buf[200], *p;
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

#if _CONSOLE
HANDLE	hConsole;		/* All 32 bit console style routines need this handle */
#endif

void attrSet(char attrib)
{
#ifdef _CONSOLE
    switch (attrib)
    {
        case A_NORMAL:
            SetConsoleTextAttribute(hConsole,FOREGROUND_BLUE | FOREGROUND_GREEN | FOREGROUND_RED);
            break;
        case A_REVERSE:
            SetConsoleTextAttribute(hConsole,BACKGROUND_BLUE | BACKGROUND_GREEN | BACKGROUND_RED);
            break;
        case A_BOLD:
            SetConsoleTextAttribute(hConsole,FOREGROUND_BLUE | FOREGROUND_GREEN | FOREGROUND_RED |FOREGROUND_INTENSITY);
            break;
    }
#else
    /* Set the attribute, using VT100 codes */
    switch (attrib)
    {
        case A_NORMAL:
            printf("\033[0m");
            break;
        case A_REVERSE:
            printf("\033[7m");
            break;
        case A_BOLD:
            printf("\033[1m");
            break;
    }
#endif
}

#ifdef _CONSOLE
void initConsole()
{
    hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
}
#endif

void erase(void)
{
#ifdef _CONSOLE
    COORD coordScreen = { 0, 0 };    /* here's where we'll home the
                                       cursor */
    DWORD cCharsWritten;
    CONSOLE_SCREEN_BUFFER_INFO csbi; /* to get buffer info */
    DWORD dwConSize;                 /* number of character cells in
                                       the current buffer */

    /* get the number of character cells in the current buffer */
    GetConsoleScreenBufferInfo( hConsole, &csbi );
    dwConSize = csbi.dwSize.X * csbi.dwSize.Y;

    /* fill the entire screen with blanks */
    FillConsoleOutputCharacter( hConsole, (TCHAR) ' ',dwConSize, coordScreen, &cCharsWritten );

    /* get the current text attribute */
    //   GetConsoleScreenBufferInfo( hConsole, &csbi );

    /* now set the buffer's attributes accordingly */
    FillConsoleOutputAttribute( hConsole, csbi.wAttributes,dwConSize, coordScreen, &cCharsWritten );

    /* put the cursor at (0, 0) */
    SetConsoleCursorPosition( hConsole, coordScreen );

#else
    // Assume that ANSI is supported
    printf("\033[2J");
#endif
}

void move(int r, int c)
{
#ifdef _CONSOLE
    COORD pos;
    pos.X = c;
    pos.Y = r;
    SetConsoleCursorPosition( hConsole, pos );
#else
    printf("\033[%d;%dH", r+1, c+1);
#endif
}

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
    allocIcode = numIcode = pProc->Icode.GetNumIcodes();
    if ((cb = allocIcode * sizeof(ICODE)) == 0)
    {
        return;  /* No Icode */
    }

    /* Open the output file (.a1 or .a2 only) */
    if (pass != 3)
    {
        p = (pass == 1)? asm1_name: asm2_name;
        fp = fopen(p, "a+");
        if (!fp)
        {
            fatalError(CANNOT_OPEN, p);
        }
    }

    /* Create temporary code array */
    // Mike: needs objectising!
    pc = (ICODE *)memcpy(allocMem(cb), pProc->Icode.GetFirstIcode(), (size_t)cb);

    if (pass == 1)
    {
        /* Bind jump offsets to labels */
        for (i = 0; i < numIcode; i++)
        {
            if ((pc[i].ic.ll.flg & I) && !(pc[i].ic.ll.flg & JMP_ICODE) &&
                    JmpInst(pc[i].ic.ll.opcode))
            {
                /* Replace the immediate operand with an icode index */
                if (labelSrch(pc,numIcode, pc[i].ic.ll.immed.op,(Int *)&pc[i].ic.ll.immed.op))
                {
                    /* This icode is the target of a jump */
                    pc[pc[i].ic.ll.immed.op].ic.ll.flg |= TARGET;
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
        fprintf(fp, "\t\t%s  PROC  %s\n", pProc->name, (pProc->flg & PROC_FAR)? "FAR": "NEAR");

    /* Loop over array printing each record */
    for (i = nextInst = 0; i < numIcode; i++)
    {
        dis1Line(i, FALSE, 0, pass);
    }

    /* Write procedure epilogue */
    if (pass != 3)
    {
        fprintf(fp, "\n\t\t%s  ENDP\n\n", pProc->name);
        fclose(fp);
    }

    free(pc);
    destroySymTables();
}

/****************************************************************************
 * dis1Line() - disassemble one line to stream fp                           *                                   *
 * i is index into Icode for this proc                                      *
 * It is assumed that icode i is already scanned                            *
 ****************************************************************************/
static void
dis1Line(Int i, boolT fWindow, char attr, Int pass)
{
    ICODE * pIcode = &pc[i];

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

    /* p points to the current position in buf[] */
    p = (char*)memset(buf, ' ', sizeof(buf));

    if (pIcode->ic.ll.flg & (TARGET | CASE))
    {
        if (fWindow)                        /* Printing to disassem window? */
            dis_newline();					/* Yes */
        else if (pass == 3)
            cCode.appendCode("\n"); /* No, print to c code buffer */
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
            for (j = 0; j < cb; j++, p += 2)
                sprintf(p, "%02X", prog.Image[pIcode->ic.ll.label + j]);
            *p = ' ';
        }
    }

    /* Check if there is a symbol here */
    selectTable(Label);
    if (readVal(&buf[POS_LAB], pIcode->ic.ll.label, 0))
    {
        buf[strlen(buf)] = ':';             /* Also removes the null */
    }

    else if (pIcode->ic.ll.flg & TARGET)    /* Symbols override Lnn labels */
    {   /* Print label */
        if (! pl[i])
        {
            pl[i] = ++lab;
        }
        if (pass == 3)
            sprintf(buf, "L%ld", pl[i]);
        else
            sprintf(&buf[15], "L%ld", pl[i]);
        buf[strlen(buf)] = ':';             /* Also removes the null */
    }

    if (pIcode->ic.ll.opcode == iSIGNEX && (pIcode->ic.ll.flg & B))
    {
        pIcode->ic.ll.opcode = iCBW;
    }

    if (pass == 3)
    {
        strcpy (&buf[8], szOps[pIcode->ic.ll.opcode]);
        buf[eop = strlen(buf)] = ' ';
        p = buf + 8 + (POS_OPR - POS_OPC);
    }
    else
    {
        strcpy(&buf[POS_OPC], szOps[pIcode->ic.ll.opcode]);
        buf[eop = strlen(buf)] = ' ';
        p = buf + POS_OPR;
    }

    switch (pIcode->ic.ll.opcode)
    {
        case iADD:  case iADC:  case iSUB:  case iSBB:  case iAND:  case iOR:
        case iXOR:  case iTEST: case iCMP:  case iMOV:  case iLEA:  case iXCHG:
            strcpy(p, strDst(pIcode->ic.ll.flg, &pIcode->ic.ll.dst));
            strcat(p, strSrc(pIcode));
            break;

        case iESC:
            flops(pIcode);
            break;

        case iSAR:  case iSHL:  case iSHR:  case iRCL:  case iRCR:  case iROL:
        case iROR:
            strcpy(p, strDst(pIcode->ic.ll.flg | I, &pIcode->ic.ll.dst));
            strcat(p, (pIcode->ic.ll.flg & I)? strSrc(pIcode): ", cl");
            break;

        case iINC:  case iDEC:  case iNEG:  case iNOT:  case iPOP:
            strcpy(p, strDst(pIcode->ic.ll.flg | I, &pIcode->ic.ll.dst));
            break;

        case iPUSH:
            if (pIcode->ic.ll.flg & I)
            {
                strcpy(p + WID_PTR, strHex(pIcode->ic.ll.immed.op));
            }
            else
            {
                strcpy(p, strDst(pIcode->ic.ll.flg | I, &pIcode->ic.ll.dst));
            }
            break;

        case iDIV:  case iIDIV:  case iMUL: case iIMUL: case iMOD:
            if (pIcode->ic.ll.flg & I)
            {
                strcat(strcpy(p, strDst(pIcode->ic.ll.flg, &pIcode->ic.ll.dst)),", ");
                formatRM(p + strlen(p), pIcode->ic.ll.flg, &pIcode->ic.ll.src);
                strcat(p, strSrc(pIcode));
            }
            else
                strcpy(p, strDst(pIcode->ic.ll.flg | I, &pIcode->ic.ll.src));
            break;

        case iLDS:  case iLES:  case iBOUND:
            strcpy(p, strDst(pIcode->ic.ll.flg, &pIcode->ic.ll.dst));
            strcat(strcat(p, ", dword ptr"), strSrc(pIcode)+1);
            break;

        case iJB:  case iJBE:  case iJAE:  case iJA:
        case iJL:  case iJLE:  case iJGE:  case iJG:
        case iJE:  case iJNE:  case iJS:   case iJNS:
        case iJO:  case iJNO:  case iJP:   case iJNP:
        case iJCXZ:case iLOOP: case iLOOPE:case iLOOPNE:
        case iJMP: case iJMPF:

            /* Check if there is a symbol here */
            selectTable(Label);
            if ((pIcode->ic.ll.immed.op < (dword)numIcode) &&  /* Ensure in range */
                    readVal(p+WID_PTR, pc[pIcode->ic.ll.immed.op].ic.ll.label, 0))
            {
                break;                          /* Symbolic label. Done */
            }

            if (pIcode->ic.ll.flg & NO_LABEL)
            {
                strcpy(p + WID_PTR, strHex(pIcode->ic.ll.immed.op));
            }
            else if (pIcode->ic.ll.flg & I)
            {
                j = pIcode->ic.ll.immed.op;
                if (! pl[j])       /* Forward jump */
                {
                    pl[j] = ++lab;
                }
                if (pIcode->ic.ll.opcode == iJMPF)
                {
                    sprintf(p, " far ptr L%ld", pl[j]);
                }
                else
                {
                    sprintf(p + WID_PTR, "L%ld", pl[j]);
                }
            }
            else if (pIcode->ic.ll.opcode == iJMPF)
            {
                strcat(strcpy(p-1, "dword ptr"), strSrc(pIcode)+1);
            }
            else
            {
                strcpy(p, strDst(I, &pIcode->ic.ll.src));
            }
            break;

        case iCALL: case iCALLF:
            if (pIcode->ic.ll.flg & I)
            {
                sprintf(p, "%s ptr %s",(pIcode->ic.ll.opcode == iCALL) ?" near":"  far",(pIcode->ic.ll.immed.proc.proc)->name);
            }
            else if (pIcode->ic.ll.opcode == iCALLF)
            {
                strcat(strcpy(p, "dword ptr"),strSrc(pIcode)+1);
            }
            else
            {
                strcpy(p, strDst(I, &pIcode->ic.ll.src));
            }
            break;

        case iENTER:
            strcat(strcpy(p + WID_PTR, strHex(pIcode->ic.ll.dst.off)), ", ");
            strcat(p, strHex(pIcode->ic.ll.immed.op));
            break;

        case iRET:  case iRETF:  case iINT:
            if (pIcode->ic.ll.flg & I)
            {
                strcpy(p + WID_PTR, strHex(pIcode->ic.ll.immed.op));
            }
            else
            {
                buf[eop] = '\0';
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
                (pIcode->ic.ll.opcode == iOUTS || pIcode->ic.ll.opcode == iREP_OUTS)
                        ? strcat(strcpy(p+WID_PTR,"dx, "), szPtr[pIcode->ic.ll.flg & B])
                        : strcpy(&buf[eop+1], szPtr[pIcode->ic.ll.flg & B]);
                if (pIcode->ic.ll.opcode == iLODS ||
                        pIcode->ic.ll.opcode == iREP_LODS ||
                        pIcode->ic.ll.opcode == iOUTS ||
                        pIcode->ic.ll.opcode == iREP_OUTS)
                {
                    strcat(p, szWreg[pIcode->ic.ll.src.segOver-rAX]);
                }
                else
                {
                    strcat(strcat(p, "es:[di], "),szWreg[pIcode->ic.ll.src.segOver - rAX]);
                }
                strcat(p, ":[si]");
            }
            else    strcpy(&buf[eop], (pIcode->ic.ll.flg & B)? "B": "W");
            break;

        case iXLAT:
            if (pIcode->ic.ll.src.segOver)
            {
                strcpy(&buf[eop+1], szPtr[1]);
                strcat(strcat(p, szWreg[pIcode->ic.ll.src.segOver-rAX]), ":[bx]");
            }
            else    buf[eop] = '\0';
            break;

        case iIN:
            strcpy(p+WID_PTR, (pIcode->ic.ll.flg & B)?"al, ": "ax, ");
            strcat(p+WID_PTR, (pIcode->ic.ll.flg & I)? strHex(pIcode->ic.ll.immed.op): "dx");
            break;

        case iOUT:
            strcpy(p+WID_PTR, (pIcode->ic.ll.flg & I)? strHex(pIcode->ic.ll.immed.op): "dx");
            strcat(p+WID_PTR, (pIcode->ic.ll.flg & B)?", al": ", ax");
            break;

        default:
            buf[eop] = '\0';
            break;
    }

    /* Comments */
    if (pIcode->ic.ll.flg & SYNTHETIC)
    {
        fImpure = FALSE;
    }
    else
    {
        for (j = pIcode->ic.ll.label, fImpure = 0; j > 0 && j < (Int)nextInst;
             j++)
        {
            fImpure |= BITMAP(j, BM_DATA);
        }
    }


    /* Check for user supplied comment */
    selectTable(Comment);
    if (readVal(cbuf, pIcode->ic.ll.label, 0))
    {
        buf[strlen(buf)] = ' ';             /* Removes the null */
        buf[POS_CMT] = ';';
        strcpy(buf+POS_CMT+1, cbuf);
    }

    else if (fImpure || (pIcode->ic.ll.flg & (SWITCH | CASE | SEG_IMMED |
                                              IMPURE | SYNTHETIC | TERMINATES)))
    {
        buf[strlen(buf)] = ' ';
        buf[POS_CMT] = '\0';
        if (pIcode->ic.ll.flg & CASE)
        {
            sprintf(buf+POS_CMT, ";Case l%ld", pIcode->ic.ll.caseTbl.numEntries);
        }
        if (pIcode->ic.ll.flg & SWITCH)
        {
            strcat(buf, ";Switch ");
        }
        if (fImpure)
        {
            strcat(buf, ";Accessed as data ");
        }
        if (pIcode->ic.ll.flg & IMPURE)
        {
            strcat(buf, ";Impure operand ");
        }
        if (pIcode->ic.ll.flg & SEG_IMMED)
        {
            strcat(buf, ";Segment constant");
        }
        if (pIcode->ic.ll.flg & TERMINATES)
        {
            strcat(buf, ";Exit to DOS");
        }
    }

    /* Comment on iINT icodes */
    if (pIcode->ic.ll.opcode == iINT)
        pIcode->writeIntComment (buf);

    /* Display output line */
    if (! (pIcode->ic.ll.flg & SYNTHETIC))
    {
        if (fWindow)
        {
            word off;
            char szOffset[6];

            off = (word)(pIcode->ic.ll.label - ((dword)pProc->state.r[rCS] << 4));
            attrSet(attr);

            sprintf(szOffset, "%04X ", off);
            printfd(szOffset);
            printfd(buf);
            dis_newline();
            attrSet(A_NORMAL);
        }
        else if (pass == 3)		/* output to .b code buffer */
            cCode.appendCode("%s\n", buf);
        else					/* output to .a1 or .a2 file */
            fprintf (fp, "%03ld %06lX %s\n", i, pIcode->ic.ll.label, buf);
    }
    else		/* SYNTHETIC instruction */
    {
        strcat (buf, ";Synthetic inst");
        if (fWindow)
        {
            printfd("     ");
            printfd(buf);
            dis_newline();
        }
        else if (pass == 3)		/* output to .b code buffer */
        {
            cCode.appendCode("%s\n", buf);
        }
        else					/* output to .a1 or .a2 file */
        {
            fprintf (fp, "%03ld        %s\n", i, buf);
        }
    }
}



/****************************************************************************
 * formatRM
 ***************************************************************************/
static void formatRM(char *p, flags32 flg, ICODEMEM *pm)
{
    char    seg[4];

    if (pm->segOver)
    {
        strcat(strcpy(seg, szWreg[pm->segOver - rAX]), ":");
    }
    else    *seg = '\0';

    if (pm->regi == 0)
    {
        sprintf(p,"%s[%s]", seg, strHex((dword)pm->off));
    }

    else if (pm->regi == (INDEXBASE - 1))
    {
        strcpy (p, "tmp");
    }

    else if (pm->regi < INDEXBASE)
    {
        strcpy(p, (flg & B)? szBreg[pm->regi - rAL]: szWreg[pm->regi - rAX]);
    }

    else if (pm->off)
    {
        if (pm->off < 0)
        {
            sprintf(p,"%s[%s-%s]", seg, szIndex[pm->regi - INDEXBASE],strHex((dword)(- pm->off)));
        }
        else
        {
            sprintf(p,"%s[%s+%s]", seg, szIndex[pm->regi - INDEXBASE],strHex((dword)pm->off));
        }
    }
    else    sprintf(p,"%s[%s]", seg, szIndex[pm->regi - INDEXBASE]);
}


/*****************************************************************************
 * strDst
 ****************************************************************************/
static char *strDst(flags32 flg, ICODEMEM *pm)
{
    static char buf[30];

    /* Immediates to memory require size descriptor */
    if ((flg & I) && (pm->regi == 0 || pm->regi >= INDEXBASE))
    {
        memcpy(buf, szPtr[flg & B], WID_PTR);
    }
    else
    {
        memset(buf, ' ', WID_PTR);
    }

    formatRM(buf + WID_PTR, flg, pm);
    return buf;
}


/****************************************************************************
 * strSrc                                                                   *
 ****************************************************************************/
static char *strSrc(ICODE *pc)
{
    static char buf[30] = {", "};

    if (pc->ic.ll.flg & I)
        strcpy(buf + 2, strHex(pc->ic.ll.immed.op));
    else if (pc->ic.ll.flg & IM_SRC)		/* level 2 */
        strcpy (buf + 2, "dx:ax");
    else
        formatRM(buf + 2, pc->ic.ll.flg, &pc->ic.ll.src);

    return buf;
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




/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *\
|           Interactive Disassembler and Associated Routines              |
\* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */



dword   pcTop;                  /* Image offset of top line */
Int     icTop;                  /* Icode index  of top line */
dword   pcCur;                  /* Image offset of cursor */
static dword	oldPcCur;       /* As above, before latest command */
Int     icCur;                  /* Icode index  of cursor */
dword   pcBot;                  /* Image offset of bottom line */
Int     icBot;                  /* Icode index  of bottom line */
dword   pcLast;                 /* Image offset of last instr in proc */
int     NSCROLL;                /* Number of limes to scroll. Pseudo constant */

/* Paint the title line */
void dispTitle(void)
{
    char buf[80];

    move(0, 0);					/* Must move before setting attributes */
    attrSet(A_BOLD);
    sprintf(buf, "Proc %s at %06lX (%04X:%04X): %d bytes of parameters ",pProc->name, pProc->Icode.GetFirstIcode()->ic.ll.label,
            pProc->state.r[rCS],(word)(pProc->Icode.GetFirstIcode()->ic.ll.label - ((dword)(pProc->state.r[rCS]) << 4)),
            pProc->cbParam);
    printfd(buf);
    if (pProc->flg & PROC_ISLIB) printfd(" LIBRARY");
    attrSet(A_NORMAL);
}


/****************************************************************************
*           updateScr - update the screen                                    *
 ****************************************************************************/
/* bNew is true if must recalculate the top line */
void updateScr(boolT bNew)
{
    int y, x;
    Int i, ic;

    bNew |= (pcCur > pcBot) || (pcCur < pcTop);
    if (bNew)
    {
        /* We need to redo the screen completely */
        erase();
        dispTitle();
        icTop = icCur;
        for (x=0; x < NSCROLL; x++)
        {
            if (icTop && pc[icTop-1].ic.ll.label +
                    (dword)pc[icTop-1].ic.ll.numBytes == pc[icTop].ic.ll.label)
            {
                /* Then this instruction is contiguous with the current */
                icTop--;
            }
            else break;
        }
        pcTop = pc[icTop].ic.ll.label;
    }
    else
    {
        dispTitle();
    }

    move(1, 0);
    nextInst = pcTop;
    for (y=1, ic=icTop; y < LINES-1; ic++, y++)
    {
        if ((ic >= numIcode) || (nextInst != pc[ic].ic.ll.label))
        {
            if (labelSrch(pc,numIcode, nextInst, &i))
            {
                ic = i;
            }
            else
            {
                pcLast = pc[ic-1].ic.ll.label;    /* Remember end of proc */
                break;                      /* Must be past last */
            }
        }

        /* Save pc of current line. Last assignment will be pc of bott line */
        pcBot = nextInst;
        icBot = ic;

        // Only have to repaint if screen is new, or repainting formerly highlighted
        // line, or newly highlighted line
        if (bNew || (pcCur == nextInst) || (oldPcCur == nextInst))
            dis1Line(ic, TRUE, (char)((pcCur == nextInst) ? A_REVERSE : A_NORMAL), 0);

        if (ic == numIcode-1)
        {
            switch (pc[ic].ic.ll.opcode)
            {
                case iJMP:  case iJMPF:
                case iRET:  case iRETF:
                case iIRET:
                    break;

                default:
                    /* We have other than a break of control flow instruction
                    at the end of the proc. Parse more instructions to
                    complete the basic block
                */
                    if ((ic = checkScanned(nextInst)) == -1)
                    {
                        /* Some error. */
                        pcLast = pcCur;    /* Remember end of proc */
                        break;             /* Must be past last */
                    }

            }
        }
    }
    dis_show();			/* Make it happen */
}

#if 0
/* An opcode based version of updateScr() */
/****************************************************************************
*           updateScrOp - update the screen                                  *
 ****************************************************************************/
/* bNew is true if must recalculate the top line */
void
updateScrOp(boolT bNew)
{
    int y, x;
    dword pc;
    word len;

    dispTitle();
    if (bNew || (pcCur > pcBot) || (pcCur < pcTop))
    {
        /* We need to redo the screen completely */
        pcTop = pcCur;
    }

    move(1, 0);
    for (y=1, pc = pcTop; y < LINES-1;)
    {
        /* Save pc of current line. Last assignment will be pc of bott line */
        pcBot = pc;

        dis1LineOp(pc, TRUE, (pcCur == pc) ? A_REVERSE : A_NORMAL, &len,pProc);
        pc += len;
        getyx(stdscr, y, x);
    }

    refresh();
}

#endif

void pushPosStack(void)
{
    /* Push the current position on the position stack */
    posStack[iPS].ic = icCur;
    posStack[iPS++].pProc = pProc;
}

static void popPosStack(void)
{
    /* Push the current position on the position stack */
    /* Note: relies on the byte wraparound. Beware! */
    //    if ((Int)(posStack[--iPS].pProc) != -1)
    if ((intptr_t)(posStack[--iPS].pProc) != intptr_t(-1))
    {
        if (posStack[iPS].pProc != pProc)
        {
            setProc(posStack[iPS].pProc);
        }
        icCur = posStack[iPS].ic;
        pcCur = pc[icCur].ic.ll.label;
    }
    else iPS++;                             /* Stack empty.. don't pop */
}


/* Check to see if there is an icode for given image offset.
    Scan it if necessary, adjusting the allocation of pc[] and pl[]
    if necessary. Returns -1 if an error, otherwise the icode offset
*/
static Int checkScanned(dword pcCur)
{
    Int i;

    /* First we check if the current icode is in range */
    /* A sanity check first */
    if (pcCur >= (dword)prog.cbImage)
    {
        /* Couldn't be! */
        return -1;
    }

    if (!labelSrch(pc,numIcode, pcCur, &i))
    {
        /* This icode does not exist yet. Tack it on the end of the existing */
        if (numIcode >= allocIcode)
        {
            allocIcode = numIcode + DELTA_ICODE;      /* Make space for this one, and a few more */
            pc = (ICODE *)reallocVar(pc, allocIcode * sizeof(ICODE));
            /* It is important to clear the new icodes, to ensure that the type
                is set to NOT_SCANNED */
            memset(&pc[numIcode], 0, (size_t)(allocIcode-numIcode)*sizeof(ICODE));
            pl.resize(allocIcode);
            memset(&pl[numIcode], 0, (size_t)(allocIcode-numIcode)*sizeof(Int));
        }
        i = numIcode++;
    }

    if (pc[i].type == NOT_SCANNED)
    {
        /* This is a new icode not even scanned yet. Scan it now */
        /* Ignore most errors... at this stage */
        if (scan(pcCur, &pc[i]) == IP_OUT_OF_RANGE)
        {
            /* Something went wrong... just forget it */
            return -1;
        }
    }

    return i;
}





/* Set up to use the procedure proc */
/* This includes some important initialisations, allocations, etc that are
    normally done in disassem() */
static void setProc(Function * proc)
{
    Int i;

    pProc = proc;                           /* Keep in a static */

    /* Free old arrays, if any */
    if (pc) free(pc);
    pl.clear();


    /* Create temporary code array */
    numIcode = pProc->Icode.GetNumIcodes();
    cb = numIcode * sizeof(ICODE);
    // Mike: needs objectising
    pc = (ICODE *)memcpy(allocMem(cb), pProc->Icode.GetFirstIcode(), (size_t)cb);

    /* Create label array to keep track of location => label name */
    pl.clear();
    pl.resize(numIcode,0);

    /* Bind jump offsets to labels */
    for (i = 0; i < numIcode; i++)
    {
        if ((pc[i].ic.ll.flg & I) && !(pc[i].ic.ll.flg & JMP_ICODE) &&
                JmpInst(pc[i].ic.ll.opcode))
        {
            /* Immediate jump instructions. Make dest an icode index */
            if (labelSrch(pc,numIcode, pc[i].ic.ll.immed.op, (Int *)&pc[i].ic.ll.immed.op))
            {
                /* This icode is the target of a jump */
                pc[pc[i].ic.ll.immed.op].ic.ll.flg |= TARGET;
                pc[i].ic.ll.flg |= JMP_ICODE;   /* So its not done twice */
            }
            else
            {
                /* This jump cannot be linked to a label */
                pc[i].ic.ll.flg |= NO_LABEL;
            }
        }
    }

    /* Window initially scrolled with entry point on top */
    pcCur = pcTop = pProc->procEntry;
    labelSrch(pc,numIcode, pcCur, &icCur);
    /* pcLast is set properly in updateScr(), at least for now */
    pcLast = (dword)-1;

}

/****************************************************************************
 *          interactDis - interactive disassembler                          *
 ****************************************************************************/
void interactDis(Function * initProc, Int initIC)
{


#ifdef __UNIX__
    printf("Sorry - interactive disasassembler option not available for Unix\n");
    return;
#else
    boolT   fInteract;
    int		nEsc = 0;		/* This cycles 0 1 2 for Esc [ X under Unix */
    /* and 0 1 for NULL X under Dos */
    int     ch;
    Int     i;
    pProc = initProc;                       /* Keep copy of init proc */
    NSCROLL = max(3, LINES >> 3);           /* Number of lines to scroll */

    /* Allocate the position stack */
    posStack = (POSSTACK_ENTRY*)allocMem(256 * sizeof(POSSTACK_ENTRY));
    iPS = 0;
    memset(posStack, -1, 256 * sizeof(POSSTACK_ENTRY));


    /* Initialise the console interface, if required */
    initConsole();

    /* Initially, work on the given proc */
    setProc(initProc);
    if (initIC)
    {
        icCur = initIC;
        pcCur = pc[icCur].ic.ll.label;
    }

    /* Initialise the symbol table */
    createSymTables();

    strcpy(cbuf, "label");                  /* Provide a default label string */

    updateScr(TRUE);

    fInteract = TRUE;
    while (fInteract)
    {
        ch = ::_getch();			// Mike: need a Unix equivalent of getch()!
#ifdef __MSDOS__
        if (nEsc)
        {
            ch += EXT;		/* Got the NULL before, so this is extended */
            nEsc = 0;
        }
        else if (ch == 0)
        {
            nEsc = 1;		/* Got one escape (actually, NULL) char */
            break;
        }
#endif
#ifdef __UNIX__
        switch (nEsc)
        {
            case 1:			/* Already got one escape */
                if (ch == '[')
                {
                    nEsc++;		/* Got 2 chars in the escape sequence */
                    break;
                }
                else
                {
                    /* Escape something else. Ignore */
                    nEsc = 0;
                }
                break;
            case 2:
                /* Already got Esc [ ... */
                ch += EXT;		/* Make it an extended key */
                nEsc = 0;		/* Reset the escape state */
                break;
            case 0:
                /* No escapes... yet */
                if (ch == 0x1B)
                {
                    nEsc++;		/* That's one escape... */
                    break;
                }
        }
#endif

        // For consoles, we get a 0xE0 then KEY_DOWN for the normal down arrow character.
        // We simply ignore the 0xE0; this has the effect that the numeric keypad keys
        // work as well (regardless of numlock state).
        oldPcCur = pcCur;
        switch (ch)
        {
            case KEY_DOWN:

                if (pcCur >= pcLast) continue;  /* Ignore it */
                pcCur += pc[icCur].ic.ll.numBytes;
                labelSrch(pc,numIcode, pcCur, &icCur);
                if (pcCur >= pcBot)
                {
                    int j;

                    /* We have gone past the bottom line. Scroll a few lines */
                    for (j=0; j < NSCROLL; j++)
                    {
                        if (pcTop >= pcLast)
                        {
                            break;
                        }
                        pcTop += pc[icTop].ic.ll.numBytes;
                        if (labelSrch(pc,numIcode, pcTop, &i))
                            icTop = i;
                        else break;         /* Some problem... no more scroll */
                    }
                }
                updateScr(FALSE);
                break;

            case KEY_UP:
                /* First simply try the prev icode */
                if ((icCur == 0) ||
                        pc[--icCur].ic.ll.label + (dword)pc[icCur].ic.ll.numBytes != pcCur)
                {
                    for (i = 0; i < numIcode; i++)
                    {
                        if (pc[i].ic.ll.label + (dword)pc[i].ic.ll.numBytes == pcCur)
                        {
                            break;          /* This is the one! */
                        }
                    }
                    if (pc[i].ic.ll.label + pc[i].ic.ll.numBytes != pcCur)
                        break;              /* Not found. Sorry! */
                    icCur = i;
                }
                pcCur = pc[icCur].ic.ll.label;
                updateScr(FALSE);
                break;


            case '2':		/* Think up a better key... */
                /* As for right arrow, but considers source operand first */
                if  (pc[icCur].ic.ll.src.off != 0)
                {
                    pushPosStack();
                    pcCur = pc[icCur].ic.ll.src.off;
                    if (!labelSrch(pc,numIcode, pcCur, &icCur))
                        break;
                    updateScr(FALSE);
                }
                /* Fall through to KEY_RIGHT processing */

            case KEY_RIGHT:
                if (pc[icCur].ic.ll.flg & I)
                {
                    if ((pc[icCur].ic.ll.opcode >= iJB) &&
                            (pc[icCur].ic.ll.opcode <= iJMPF))
                    {
                        /* An immediate jump op. Jump to it */
                        pushPosStack();
                        if (pc[icCur].ic.ll.flg & JMP_ICODE)
                        {
                            /* immed.op is an icode offset */
                            icCur = pc[icCur].ic.ll.immed.op;
                            pcCur = pc[icCur].ic.ll.label;
                        }
                        else
                        {
                            /* immed.op is still an image offset.
                                Quite likely we need to scan */
                            pcCur = pc[icCur].ic.ll.immed.op;
                            if ((icCur = checkScanned(pcCur)) == -1)
                                break;
                        }
                    }
                    else if ((pc[icCur].ic.ll.opcode == iCALL) ||
                             (pc[icCur].ic.ll.opcode == iCALLF))
                    {
                        /* The dest is a pointer to a proc struct */
                        // First check that the procedure has icodes (e.g. may be
                        // a library function, or just not disassembled yet)
                        Function * pp = (Function *)pc[icCur].ic.ll.immed.op;
                        if (pp->Icode.GetFirstIcode() != NULL)
                        {
                            pushPosStack();
                            setProc(pp);
                        }
                    }
                    else
                    {
                        /* Other immediate */
                        pushPosStack();
                        pcCur = pc[icCur].ic.ll.immed.op;
                        dispData(pProc->state.r[rDS]);
                        break;
                    }
                }
                else if (pc[icCur].ic.ll.dst.off != 0)
                {
                    pushPosStack();
                    pcCur = pc[icCur].ic.ll.dst.off;
                    if (!labelSrch(pc,numIcode, pcCur, &icCur))
                    {
                        dispData(pProc->state.r[rDS]);
                        break;
                    }
                }
                else if (pc[icCur].ic.ll.src.off != 0)
                {
                    pushPosStack();
                    pcCur = pc[icCur].ic.ll.src.off;
                    if (!labelSrch(pc,numIcode, pcCur, &icCur))
                    {
                        dispData(pProc->state.r[rDS]);
                        break;
                    }
                }
                updateScr(TRUE);
                break;

            case KEY_LEFT:
                popPosStack();
                pcCur = pc[icCur].ic.ll.label;
                updateScr(TRUE);
                break;


            case KEY_NPAGE:
                pcCur = pcTop = pcBot;      /* Put bottom line at top now */
                icCur = icTop = icBot;
                updateScr(FALSE);
                break;

            case KEY_PPAGE:
                pcTop -= (LINES-2) * 2; /* Average of 2 bytes per inst */
                for (i = 0; i < numIcode; i++)
                {
                    if  ((pc[i].ic.ll.label <= pcTop) &&
                         (pc[i].ic.ll.label + (dword)pc[i].ic.ll.numBytes >= pcTop))
                    {
                        break;          /* This is the spot! */
                    }
                }
                if (i >= numIcode)
                {
                    /* Something went wrong. Goto to first icode */
                    i = 0;
                }
                icCur = icTop = i;
                pcCur = pcTop = pc[i].ic.ll.label;
                updateScr(FALSE);
                break;

            case 'l':                       /* Add a symbolic label here */
            {
                char    *pStr;

                move(LINES, 0);
                printf("Enter symbol: ");
                gets(cbuf);         /* Get a string to buf */
                move (LINES, 0);
                printf("%50c", ' ');

                if (strlen(cbuf) >= SYMLEN)
                {
                    /* Name too ling. Truncate */
                    cbuf[SYMLEN-1] = '\0';
                }
                pStr = addStrTbl(cbuf);     /* Add to the string table */

                selectTable(Label);         /* Select the label table */
                /* Add the symbol to both value- and symbol- hashed tables */
                enterSym(pStr, pcCur, pProc, TRUE);

                if (icCur == 0)
                {
                    /* We are at the first icode of a function.
                        Assume it is the entry point, and rename the function */
                    strcpy(pProc->name, cbuf);
                }

                updateScr(FALSE);
                break;
            }

            case ';':
            {
                char    *pStr;
                word w;

                if (findVal(pcCur, 0, &w))
                {
                    readVal(cbuf, pcCur, 0);/* Make it the default string */
                    deleteVal(pcCur, 0, FALSE);
                }
                else
                {
                    cbuf[0] = '\0';             /* Remove prev string */
                }

                /* Enter a comment here, from a window */
                move(LINES, 0);
                printf("Enter comment: ");
                gets(cbuf);			        /* Get a string to buf */
                move(LINES, 0);
                printf("%50c", ' ');

                pStr = addStrTbl(cbuf);     /* Add to the string table */

                selectTable(Comment);
                enterSym(pStr, pcCur, pProc, FALSE);/* Add the symbol */

                updateScr(FALSE);
                break;
            }


            case 'X' & 0x1F:                /* Control X; can't use Alt with Unix */
                fInteract = FALSE;          /* Exit interactive mode */
                attrSet(A_NORMAL);			/* Normal attributes */
                break;
        }
    }

    free(posStack);
    destroySymTables();
#endif			// #ifdef unix
}


/****************************************************************************
 *          Display the current image position as data                      *
 ****************************************************************************/
static void
dispData(word dataSeg)
{
    int y, c, i;
    Int pc, pcStart;
    Int off = (Int)dataSeg << 4;
    char szOffset[6], szByte[4];

    if (pcCur >= (dword)prog.cbImage)
    {
        /* We're at an invalid address. Use 0x100 instead */
        pcCur = 0;
    }
    erase();
    dispTitle();

    pcStart = pc = pcCur;           /* pc at start of line */
    for (y=1; y < LINES-1; y++)
    {
        move (y, 1);
        sprintf(szOffset, "%04lX ", pc);
        printfd(szOffset);
        for (i=0; i < 16; i++)
        {
            sprintf(szByte, "%02X ", prog.Image[pc++ + off]);
            printfd(szByte);
            if ((pc + off) > prog.cbImage) break;
        }
        pc = pcStart;
        for (i=0; i < 16; i++)
        {
            c = prog.Image[pc++ + off];
            if ((c < 0x20) || (c > 0x7E))
            {
                c = '.';
            }
            szByte[0] = (char)c;
            szByte[1] = '\0';
            printfd(szByte);
            if ((pc + off) > prog.cbImage) break;
        }
        dis_newline();
        pcStart = pc;

        if ((pc + off) > prog.cbImage) break;

        /*       getyx(stdscr, y, x);	*/
    }

}


boolT callArg(word off, char *sym)
{
    dword   imageOff;

    imageOff = off + ((dword)pProc->state.r[rCS] << 4);
    /* Search procedure list for one with appropriate entry point */
    std::list<Function>::iterator iter= std::find_if(pProcList.begin(),pProcList.end(),
        [imageOff](const Function &f) -> bool { return f.procEntry==imageOff; });
    if(iter==pProcList.end())
    {
        /* No existing proc entry */
    //ERROR: dereferencing NULL !?!
        //LibCheck(*iter);
        Function x;
        x.procEntry=imageOff;
        LibCheck(x);
        if (x.flg & PROC_ISLIB)
        {
            /* No entry for this proc, but it is a library function.
                Create an entry for it */
            pProcList.push_back(x);
            iter = (++pProcList.rbegin()).base();
        }
    }
    if(iter==pProcList.end())
        return false;
    /* We have a proc entry for this procedure. Copy the name */
    strcpy(sym, iter->name);
    return true;
}

/* Handle the floating point opcodes (icode iESC) */
static void flops(ICODE *pIcode)
{
    char bf[30];
    byte op = (byte)pIcode->ic.ll.immed.op;

    /* Note that op is set to the escape number, e.g.
        esc 0x38 is FILD */

    if ((pIcode->ic.ll.dst.regi == 0) || (pIcode->ic.ll.dst.regi >= INDEXBASE))
    {
        /* The mod/rm mod bits are not set to 11 (i.e. register).
           This is the normal floating point opcode */
        strcpy(&buf[POS_OPC], szFlops1[op]);
        buf[strlen(buf)] = ' ';

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

        formatRM(bf + 10, pIcode->ic.ll.flg, &pIcode->ic.ll.dst);
        strcpy(p, bf);
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
                strcpy(&buf[POS_OPC], szFlops0C[pIcode->ic.ll.dst.regi - rAX]);
                break;
            case 0x0D:
                strcpy(&buf[POS_OPC], szFlops0D[pIcode->ic.ll.dst.regi - rAX]);
                break;
            case 0x0E:
                strcpy(&buf[POS_OPC], szFlops0E[pIcode->ic.ll.dst.regi - rAX]);
                break;
            case 0x0F:
                strcpy(&buf[POS_OPC], szFlops0F[pIcode->ic.ll.dst.regi - rAX]);
                break;
            case 0x15:
                strcpy(&buf[POS_OPC], szFlops15[pIcode->ic.ll.dst.regi - rAX]);
                break;
            case 0x1C:
                strcpy(&buf[POS_OPC], szFlops1C[pIcode->ic.ll.dst.regi - rAX]);
                break;
            case 0x33:
                strcpy(&buf[POS_OPC], szFlops33[pIcode->ic.ll.dst.regi - rAX]);
                break;
            case 0x3C:
                strcpy(&buf[POS_OPC], szFlops3C[pIcode->ic.ll.dst.regi - rAX]);
                break;
            default:
                strcpy(&buf[POS_OPC], szFlops2[op]);
                buf[strlen(buf)] = ' ';
                if ((op >= 0x20) && (op <= 0x27))
                {
                    /* This is the ST(i), ST form. */
                    sprintf(&buf[POS_OPR2], "ST(%d),ST", pIcode->ic.ll.dst.regi - rAX);
                }
                else
                {
                    /* ST, ST(i) */
                    sprintf(&buf[POS_OPR2], "ST,ST(%d)", pIcode->ic.ll.dst.regi - rAX);
                }

                break;
        }
    }
}


