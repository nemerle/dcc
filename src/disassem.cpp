/****************************************************************************
 *          dcc project disassembler
 * (C) Cristina Cifuentes, Mike van Emmerik, Jeff Ledermann
 ****************************************************************************/
#include <stdint.h>
#include <vector>
#include <map>
#include <sstream>
#include <iomanip>
#include <stdio.h>
#include <string.h>
#include <malloc.h>		/* For free() */

#include "dcc.h"
#include "symtab.h"
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


void  dis1LineOp(int i, boolT fWin, char attr, uint16_t *len, Function * pProc);
static void  formatRM(ostringstream &p, uint32_t flg, const LLOperand &pm);
static ostringstream &strDst(ostringstream &os, uint32_t flg, LLOperand &pm);

static char *strHex(uint32_t d);
static int   checkScanned(uint32_t pcCur);
static void  setProc(Function * proc);
static void  dispData(uint16_t dataSeg);
boolT callArg(uint16_t off, char *temp);  /* Check for procedure name */

static  FILE   *fp;
static  CIcodeRec pc;

static  int     cb, j, numIcode, allocIcode;
static  map<int,int> pl;
static  uint32_t   nextInst;
static  boolT    fImpure;
static  int     g_lab, prevPass;
static  Function *   pProc;          /* Points to current proc struct */

struct POSSTACK_ENTRY
{
    int     ic;                 /* An icode offset */
    Function *   pProc;              /* A pointer to a PROCEDURE structure */
} ;
vector<POSSTACK_ENTRY> posStack; /* position stack */
uint8_t              iPS;          /* Index into the stack */

//static  char    cbuf[256];      /* Has to be 256 for wgetstr() to work */

// These are "curses equivalent" functions. (Used to use curses for all this,
// but it was too much of a distribution hassle

#define printfd(x) printf(x)
#define dis_newline() printf("\n")
#define dis_show()					// Nothing to do unless using Curses


void LLInst::findJumpTargets(CIcodeRec &_pc)
{
    if (testFlags(I) && ! testFlags(JMP_ICODE) && isJmpInst())
    {
        /* Replace the immediate operand with an icode index */
        iICODE labTgt=_pc.labelSrch(src.op());
        if (labTgt!=_pc.end())
        {
            src.SetImmediateOp(labTgt->loc_ip);
            /* This icode is the target of a jump */
            labTgt->ll()->setFlags(TARGET);
            setFlags(JMP_ICODE);   /* So its not done twice */
        }
        else
        {
            /* This jump cannot be linked to a label */
            setFlags(NO_LABEL);
        }
    }

}
/*****************************************************************************
 * disassem - Prints a disassembled listing of a procedure.
 *			  pass == 1 generates output on file .a1
 *			  pass == 2 generates output on file .a2
 *			  pass == 3 generates output on file .b
 ****************************************************************************/
void disassem(int pass, Function * ppProc)
{


    pProc = ppProc;             /* Save the passes pProc */
    if (pass != prevPass)
    {
        prevPass = pass;
        g_lab = 0; 	/* Restart label numbers */
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
        //for (i = 0; i < numIcode; i++)
#ifdef _lint
        for (auto i=pc.begin(); i!=pc.end(); ++i)
        {
            ICODE &icode(*i);
#else
        for( ICODE &icode : pc)
        {
#endif
            LLInst *ll=icode.ll();
            ll->findJumpTargets(pc);
        }
    }

    /* Create label array to keep track of location => label name */
    pl.clear();

    /* Write procedure header */
    if (pass != 3)
        fprintf(fp, "\t\t%s  PROC  %s\n", pProc->name.c_str(), (pProc->flg & PROC_FAR)? "FAR": "NEAR");

    /* Loop over array printing each record */
    nextInst = 0;
#ifdef _lint
    for (auto i=pc.begin(); i!=pc.end(); ++i)
    {
        ICODE &icode(*i);
#else
    for( ICODE &icode : pc)
    {
#endif
        icode.ll()->dis1Line(icode.loc_ip,pass);
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
void LLInst::dis1Line(int loc_ip, int pass)
{
    ostringstream oper_stream;
    ostringstream hex_bytes;
    ostringstream result_stream;
    ostringstream opcode_with_mods;
    ostringstream operands_s;

    oper_stream << uppercase;
    hex_bytes << uppercase;
    /* Disassembly stage 1 --
         * Do not try to display NO_CODE entries or synthetic instructions,
         * other than JMPs, that have been introduced for def/use analysis. */
    if ((option.asm1) &&
            ( testFlags(NO_CODE) ||
             (testFlags(SYNTHETIC) && (opcode != iJMP))))
    {
        return;
    }
    else if (testFlags(NO_CODE))
    {
        return;
    }
    if (testFlags(TARGET | CASE))
    {
        if (pass == 3)
            cCode.appendCode("\n"); /* Print to c code buffer */
        else
            fprintf(fp, "\n");              /* No, print to the stream */
    }

    /* Find next instruction label and print hex bytes */
    if (testFlags(SYNTHETIC))
        nextInst = label;
    else
    {
        cb = (uint32_t) numBytes;
        nextInst = label + cb;

        /* Output hexa code in program image */
        if (pass != 3)
        {
            for (j = 0; j < cb; j++)
            {
                hex_bytes << hex << setw(2) << setfill('0') << uint16_t(prog.Image[label + j]);
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
        if (readVal(lab_contents, label, 0))
        {
            lab_contents << ':';             /* Also removes the null */
        }
        else if (testFlags(TARGET))    /* Symbols override Lnn labels */
        {
            /* Print label */
            if (pl.count(loc_ip)==0)
            {
                pl[loc_ip] = ++g_lab;
            }
            lab_contents<< "L"<<pl[loc_ip]<<':';
        }
        oper_stream<< lab_contents.str();
    }
    if (opcode == iSIGNEX && testFlags(B))
    {
        opcode = iCBW;
    }
    opcode_with_mods<<szOps[opcode];

    switch (opcode)
    {
        case iADD:  case iADC:  case iSUB:  case iSBB:  case iAND:  case iOR:
        case iXOR:  case iTEST: case iCMP:  case iMOV:  case iLEA:  case iXCHG:
            strDst(operands_s,getFlag(), dst);
            strSrc(operands_s);
            break;

        case iESC:
            flops(operands_s);
            break;

        case iSAR:  case iSHL:  case iSHR:  case iRCL:  case iRCR:  case iROL:
        case iROR:
            strDst(operands_s,getFlag() | I, dst);
            if(testFlags(I))
                strSrc(operands_s);
            else
                operands_s<<", cl";
            break;

        case iINC:  case iDEC:  case iNEG:  case iNOT:  case iPOP:
            strDst(operands_s,getFlag() | I, dst);
            break;

        case iPUSH:
            if (testFlags(I))
            {
                operands_s<<strHex(src.op());
//                strcpy(p + WID_PTR, strHex(pIcode->ll()->immed.op));
            }
            else
            {
                strDst(operands_s,getFlag() | I, dst);
            }
            break;

        case iDIV:  case iIDIV:  case iMUL: case iIMUL: case iMOD:
            if (testFlags(I))
            {
                strDst(operands_s,getFlag(), dst) <<", ";
                formatRM(operands_s, getFlag(), src);
                strSrc(operands_s);
            }
            else
                strDst(operands_s,getFlag() | I, src);
            break;

        case iLDS:  case iLES:  case iBOUND:
            strDst(operands_s,getFlag(), dst)<<", dword ptr";
            strSrc(operands_s,true);
            break;

        case iJB:  case iJBE:  case iJAE:  case iJA:
        case iJL:  case iJLE:  case iJGE:  case iJG:
        case iJE:  case iJNE:  case iJS:   case iJNS:
        case iJO:  case iJNO:  case iJP:   case iJNP:
        case iJCXZ:case iLOOP: case iLOOPE:case iLOOPNE:
        case iJMP: case iJMPF:

            /* Check if there is a symbol here */
    {
        ICODE *lab=pc.GetIcode(src.op());
            selectTable(Label);
        if ((src.op() < (uint32_t)numIcode) &&  /* Ensure in range */
                readVal(operands_s, lab->ll()->label, 0))
            {
                break;                          /* Symbolic label. Done */
        }
            }

            if (testFlags(NO_LABEL))
            {
                //strcpy(p + WID_PTR, strHex(pIcode->ll()->immed.op));
                operands_s<<strHex(src.op());
            }
            else if (testFlags(I) )
            {
                j = src.op();
                if (pl.count(j)==0)       /* Forward jump */
                {
                    pl[j] = ++g_lab;
                }
                if (opcode == iJMPF)
                {
                    operands_s<<" far ptr ";
                }
                operands_s<<"L"<<pl[j];
            }
            else if (opcode == iJMPF)
            {
                operands_s<<"dword ptr";
                strSrc(operands_s,true);
            }
            else
            {
                strDst(operands_s,I, src);
            }
            break;

        case iCALL: case iCALLF:
            if (testFlags(I))
            {
                if((opcode == iCALL))
                    operands_s<< "near";
                else
                    operands_s<< " far";
                operands_s<<" ptr "<<(src.proc.proc)->name;
            }
            else if (opcode == iCALLF)
            {
                operands_s<<"dword ptr ";
                strSrc(operands_s,true);
            }
            else
                strDst(operands_s,I, src);
            break;

        case iENTER:
            operands_s<<strHex(dst.off)<<", ";
            operands_s<<strHex(src.op());
            break;

        case iRET:  case iRETF:  case iINT:
            if (testFlags(I))
            {
                operands_s<<strHex(src.op());
            }
            break;

        case iCMPS:  case iREPNE_CMPS:  case iREPE_CMPS:
        case iSCAS:  case iREPNE_SCAS:  case iREPE_SCAS:
        case iSTOS:  case iREP_STOS:
        case iLODS:  case iREP_LODS:
        case iMOVS:  case iREP_MOVS:
        case iINS:   case iREP_INS:
        case iOUTS:  case iREP_OUTS:
            if (src.segOver)
            {
                bool is_dx_src=(opcode == iOUTS || opcode == iREP_OUTS);
                if(is_dx_src)
                    operands_s<<"dx, "<<szPtr[getFlag() & B];
                else
                    operands_s<<szPtr[getFlag() & B];
                if (opcode == iLODS ||
                    opcode == iREP_LODS ||
                    opcode == iOUTS ||
                    opcode == iREP_OUTS)
                {
                    operands_s<<szWreg[src.segOver-rAX];
                }
                else
                {
                    operands_s<<"es:[di], "<<szWreg[src.segOver - rAX];
                }
                operands_s<<":[si]";
            }
            else
            {
                (getFlag() & B)? opcode_with_mods<< "B": opcode_with_mods<< "W";
            }
        break;

        case iXLAT:
            if (src.segOver)
            {
                operands_s<<" "<<szPtr[1];
                operands_s<<szWreg[src.segOver-rAX]<<":[bx]";
            }
            break;

        case iIN:
            (getFlag() & B)? operands_s<<"al, " : operands_s<< "ax, ";
            (testFlags(I))? operands_s << strHex(src.op()) : operands_s<< "dx";
            break;

        case iOUT:
        {
            std::string d1=((testFlags(I))? strHex(src.op()): "dx");
            std::string d2=((getFlag() & B) ? ", al": ", ax");
            operands_s<<d1 << d2;
        }
        break;

        default:
            break;
    }
    oper_stream << setw(15) << left <<opcode_with_mods.str();
    oper_stream << operands_s.str();
    /* Comments */
    if (testFlags(SYNTHETIC))
    {
        fImpure = FALSE;
    }
    else
    {
        for (j = label, fImpure = 0; j > 0 && j < (int)nextInst; j++)
        {
            fImpure |= BITMAP(j, BM_DATA);
        }
    }

    result_stream << setw(54) << left << oper_stream.str();
    /* Check for user supplied comment */
    selectTable(Comment);
    ostringstream cbuf;
    if (readVal(cbuf, label, 0))
    {
        result_stream <<"; "<<cbuf.str();
    }
    else if (fImpure || (testFlags(SWITCH | CASE | SEG_IMMED | IMPURE | SYNTHETIC | TERMINATES)))
    {
        if (testFlags(CASE))
        {
            result_stream << ";Case l"<< caseTbl.numEntries;
        }
        if (testFlags(SWITCH))
        {
            result_stream << ";Switch ";
        }
        if (fImpure)
        {
            result_stream << ";Accessed as data ";
        }
        if (testFlags(IMPURE))
        {
            result_stream << ";Impure operand ";
        }
        if (testFlags(SEG_IMMED))
        {
            result_stream << ";Segment constant";
        }
        if (testFlags(TERMINATES))
        {
            result_stream << ";Exit to DOS";
        }
    }

    /* Comment on iINT icodes */
    if (opcode == iINT)
        writeIntComment(result_stream);

    /* Display output line */
    if(pass==3)
    {
        /* output to .b code buffer */
        if (testFlags(SYNTHETIC))
            result_stream<<";Synthetic inst";
        if (pass == 3)		/* output to .b code buffer */
            cCode.appendCode("%s\n", result_stream.str().c_str());

    }
    else
    {
        if (not testFlags(SYNTHETIC) )
        {
            /* output to .a1 or .a2 file */
            fprintf (fp, "%03ld %06lX %s\n", loc_ip, label, result_stream.str().c_str());
        }
        else		/* SYNTHETIC instruction */
        {
            result_stream<<";Synthetic inst";
            fprintf (fp, "%03ld        %s\n", loc_ip, result_stream.str().c_str());
        }
    }
}



/****************************************************************************
 * formatRM
 ***************************************************************************/
static void formatRM(std::ostringstream &p, uint32_t flg, const LLOperand &pm)
{
    char    seg[4];

    if (pm.segOver)
    {
        strcat(strcpy(seg, szWreg[pm.segOver - rAX]), ":");
    }
    else    *seg = '\0';

    if (pm.regi == 0)
    {
        p<<seg<<"["<<strHex((uint32_t)pm.off)<<"]";
    }

    else if (pm.regi == (INDEX_BX_SI - 1))
    {
        p<<"tmp";
    }

    else if (pm.regi < INDEX_BX_SI)
    {
        if(flg & B)
            p << szBreg[pm.regi - rAL];
        else
            p << szWreg[pm.regi - rAX];
    }

    else if (pm.off)
    {
        if (pm.off < 0)
        {
            p <<seg<<"["<<szIndex[pm.regi - INDEX_BX_SI]<<"-"<<strHex((uint32_t)(- pm.off))<<"]";
        }
        else
        {
            p <<seg<<"["<<szIndex[pm.regi - INDEX_BX_SI]<<"+"<<strHex((uint32_t)(pm.off))<<"]";
        }
    }
    else
        p <<seg<<"["<<szIndex[pm.regi - INDEX_BX_SI]<<"]";
}


/*****************************************************************************
 * strDst
 ****************************************************************************/
static ostringstream & strDst(ostringstream &os,uint32_t flg, LLOperand &pm)
{
    /* Immediates to memory require size descriptor */
    //os << setw(WID_PTR);
    if ((flg & I) && (pm.regi == 0 || pm.regi >= INDEX_BX_SI))
        os << szPtr[flg & B];
    formatRM(os, flg, pm);
    return os;
}


/****************************************************************************
 * strSrc                                                                   *
 ****************************************************************************/
ostringstream &LLInst::strSrc(ostringstream &os,bool skip_comma)
{

    if(false==skip_comma)
        os<<", ";
    if (testFlags(I))
        os<<strHex(src.op());
    else if (testFlags(IM_SRC))		/* level 2 */
        os<<"dx:ax";
    else
        formatRM(os, getFlag(), src);

    return os;
}



/****************************************************************************
 * strHex                                                                   *
 ****************************************************************************/
static char *strHex(uint32_t d)
{
    static char buf[10];

    d &= 0xFFFF;
    sprintf(buf, "0%lX%s", d, (d > 9)? "h": "");
    return (buf + (buf[1] <= '9'));
}

/****************************************************************************
 *          interactDis - interactive disassembler                          *
 ****************************************************************************/
void interactDis(Function * initProc, int initIC)
{
    printf("Sorry - interactive disasassembler option not available for Unix\n");
    return;
}

/* Handle the floating point opcodes (icode iESC) */
void LLInst::flops(std::ostringstream &out)
{
    char bf[30];
    uint8_t op = (uint8_t)src.op();

    /* Note that op is set to the escape number, e.g.
        esc 0x38 is FILD */

    if ((dst.regi == 0) || (dst.regi >= INDEX_BX_SI))
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
                        strcpy(bf, "uint16_t  ptr ");
                        break;
                }
        }

        formatRM(out, getFlag(), dst);
    }
    else
    {
        /* The mod/rm mod bits are set to 11 (i.e. register).
           Could be specials (0x0C-0x0F, etc), or the st(i) versions of
                   normal opcodes. Because the opcodes are slightly different for
                   this case (e.g. op=04 means FSUB if reg != 3, but FSUBR for
                   reg == 3), a separate table is used (szFlops2). */
        int destRegIdx=dst.regi - rAX;
        switch (op)
        {
            case 0x0C:
                out << szFlops0C[destRegIdx];
                break;
            case 0x0D:
                out << szFlops0D[destRegIdx];
                break;
            case 0x0E:
                out << szFlops0E[destRegIdx];
                break;
            case 0x0F:
                out << szFlops0F[destRegIdx];
                break;
            case 0x15:
                out << szFlops15[destRegIdx];
                break;
            case 0x1C:
                out << szFlops1C[destRegIdx];
                break;
            case 0x33:
                out << szFlops33[destRegIdx];
                break;
            case 0x3C:
                out << szFlops3C[destRegIdx];
                break;
            default:
                out << szFlops2[op];
                if ((op >= 0x20) && (op <= 0x27))
                {
                    /* This is the ST(i), ST form. */
                    out << "ST("<<destRegIdx - rAX<<"),ST";
                }
                else
                {
                    /* ST, ST(i) */
                    out << "ST,ST("<<destRegIdx;
                }

                break;
        }
    }
}


