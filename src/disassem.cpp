/****************************************************************************
 *          dcc project disassembler
 * (C) Cristina Cifuentes, Mike van Emmerik, Jeff Ledermann
 ****************************************************************************/
#include "disassem.h"

#include "dcc.h"
#include "msvc_fixes.h"
#include "symtab.h"
#include "project.h"

#include <QtCore/QFile>
#include <QtCore/QDebug>
#include <stdint.h>
#include <vector>
#include <map>
#include <sstream>
#include <iomanip>
#include <stdio.h>
#include <string.h>
#include "src/ui/StructuredTextTarget.h"

// Note: for the time being, there is no interactive disassembler
// for unix

using namespace std;


#define POS_LAB     15              /* Position of label */
#define POS_OPC     20              /* Position of opcode */
#define POS_OPR     25              /* Position of operand */
#define WID_PTR     10              /* Width of the "xword ptr" lingo */
#define POS_OPR2    POS_OPR+WID_PTR /* Position of operand after "xword ptr" */
#define POS_CMT     54              /* Position of comment */

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


static const char *szPtr[2]   = { "word ptr ", "byte ptr " };

static void  formatRM(QTextStream & p, const LLOperand &pm);
static QTextStream & strDst(QTextStream & os, uint32_t flg, const LLOperand &pm);

static char *strHex(uint32_t d);
//static int   checkScanned(uint32_t pcCur);
//static void  setProc(Function * proc);
//static void  dispData(uint16_t dataSeg);
bool callArg(uint16_t off, char *temp);  /* Check for procedure name */

//static  FILE   *dis_g_fp;
static  CIcodeRec pc;
static  int     cb, numIcode, allocIcode;
static  map<int,int> pl;
static  uint32_t   nextInst;
//static  bool    fImpure;
//static  int     g_lab;
static  PtrFunction pProc;          /* Points to current proc struct */

struct POSSTACK_ENTRY
{
    int     ic;                 /* An icode offset */
    Function *   pProc;              /* A pointer to a PROCEDURE structure */
} ;
static vector<POSSTACK_ENTRY> posStack; /* position stack */
//static uint8_t              iPS;          /* Index into the stack */


// These are "curses equivalent" functions. (Used to use curses for all this,
// but it was too much of a distribution hassle

#define printfd(x) printf(x)
#define dis_newline() printf("\n")
#define dis_show()              // Nothing to do unless using Curses


void LLInst::findJumpTargets(CIcodeRec &_pc)
{
    if (srcIsImmed() and not testFlags(JMP_ICODE) and isJmpInst())
    {
        /* Replace the immediate operand with an icode index */
        iICODE labTgt=_pc.labelSrch(src().getImm2());
        if (labTgt!=_pc.end())
        {
            m_src.SetImmediateOp(labTgt->loc_ip);
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
 *          pass == 1 generates output on file .a1
 *          pass == 2 generates output on file .a2
 *          pass == 3 generates output on file .b
 ****************************************************************************/

void Disassembler::disassem(PtrFunction ppProc)
{


    pProc = ppProc;             /* Save the passes pProc */
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
        m_disassembly_target = new QFile(p);
        if(!m_disassembly_target->open(QFile::WriteOnly|QFile::Text|QFile::Append)) {
            fatalError(CANNOT_OPEN, p.toStdString().c_str());
        }
        m_fp.setDevice(m_disassembly_target);
    }
    /* Create temporary code array */
    // Mike: needs objectising!
    pc=pProc->Icode;

    if (pass == 1)
    {
        /* Bind jump offsets to labels */
        //for (i = 0; i < numIcode; i++)
        for( ICODE &icode : pc)
        {
            LLInst *ll=icode.ll();
            ll->findJumpTargets(pc);
        }
    }

    /* Create label array to keep track of location => label name */
    pl.clear();

    /* Write procedure header */
    if (pass != 3)
    {
        const char * near_far=(pProc->flg & PROC_FAR)? "FAR": "NEAR";
        m_fp << "\t\t"<<pProc->name<<"  PROC  "<< near_far<<"\n";
    }

    /* Loop over array printing each record */
    nextInst = 0;
    for( ICODE &icode : pc)
    {
        this->dis1Line(*icode.ll(),icode.loc_ip,pass);
    }

    /* Write procedure epilogue */
    if (pass != 3)
    {
        m_fp << "\n\t\t"<<pProc->name<<"  ENDP\n\n";
        m_fp.setDevice(nullptr);
        m_disassembly_target->close();
        delete m_disassembly_target;

    }

    pc.clear();
    destroySymTables();
}
/****************************************************************************
 * dis1Line() - disassemble one line to stream fp                           *
 * i is index into Icode for this proc                                      *
 * It is assumed that icode i is already scanned                            *
 ****************************************************************************/
void Disassembler::dis1Line(LLInst &inst,int loc_ip, int pass)
{
    assert(false);
}



/****************************************************************************
 * formatRM
 ***************************************************************************/
static void formatRM(QTextStream &p, const LLOperand &pm)
{
    //char    seg[4];

    if (pm.segOver)
    {
        p <<Machine_X86::regName(pm.segOver)<<':';
    }

    if (pm.regi == rUNDEF)
    {
        p<<"["<<strHex((uint32_t)pm.off)<<"]";
    }
    else if (pm.isReg())
    {
        p<<Machine_X86::regName(pm.regi);
    }

    else if (pm.off)
    {
        if (pm.off < 0)
        {
            p <<"["<<Machine_X86::regName(pm.regi)<<"-"<<strHex((uint32_t)(- pm.off))<<"]";
        }
        else
        {
            p <<"["<<Machine_X86::regName(pm.regi)<<"+"<<strHex((uint32_t)(pm.off))<<"]";
        }
    }
    else
        p <<"["<<Machine_X86::regName(pm.regi)<<"]";
}


/*****************************************************************************
 * strDst
 ****************************************************************************/
static QTextStream & strDst(QTextStream &os,uint32_t flg, const LLOperand &pm)
{
    /* Immediates to memory require size descriptor */
    //os << setw(WID_PTR);
    if ((flg & I) and not pm.isReg())
        os << szPtr[flg & B];
    formatRM(os, pm);
    return os;
}


/****************************************************************************
 * strSrc                                                                   *
 ****************************************************************************/
//QTextStream &LLInst::strSrc(QTextStream &os,bool skip_comma)
//{
//    if(false==skip_comma)
//        os<<", ";
//    if (srcIsImmed())
//        os<<strHex(src().getImm2());
//    else if (testFlags(IM_SRC)) /* level 2 */
//        os<<"dx:ax";
//    else
//        formatRM(os, src());

//    return os;
//}



/****************************************************************************
 * strHex                                                                   *
 ****************************************************************************/
static char *strHex(uint32_t d)
{
    static char buf[10];

    d &= 0xFFFF;
    sprintf(buf, "0%X%s", d, (d > 9)? "h": "");
    return (buf + (buf[1] <= '9'));
}

/****************************************************************************
 *          interactDis - interactive disassembler                          *
 ****************************************************************************/
void interactDis(const PtrFunction & initProc, int initIC)
{
    QString procname = "UNKNOWN";
    if(initProc)
        procname = initProc->name;

    qDebug() << "Wanted to start interactive disasassembler for "<<procname<<":"<<initIC;
    return;
}

/* Handle the floating point opcodes (icode iESC) */
void LLInst::flops(QTextStream &out)
{
    //char bf[30];
    uint8_t op = (uint8_t)src().getImm2();

    /* Note that op is set to the escape number, e.g.
        esc 0x38 is FILD */

    if ( not m_dst.isReg() )
    {
        /* The mod/rm mod bits are not set to 11 (i.e. register). This is the normal floating point opcode */
        out<<Machine_X86::floatOpName(op)<<' ';
        out.setFieldWidth(10);
        if ((op == 0x29) or (op == 0x1F))
        {
            out <<  "tbyte ptr ";
        }
        else switch (op & 0x30)
        {
        case 0x00:
        case 0x10:
            out << "dword ptr ";
            break;
        case 0x20:
            out << "qword ptr ";
            break;
        case 0x30:
            switch (op)
            {
            case 0x3C:       /* FBLD */
            case 0x3E:       /* FBSTP */
                out << "tbyte ptr ";
                break;
            case 0x3D:       /* FILD 64 bit */
            case 0x3F:       /* FISTP 64 bit */
                out << "qword ptr ";
                break;

            default:
                out << "uint16_t  ptr ";
                break;
            }
        }
        out.setFieldWidth(0);
        formatRM(out, m_dst);
    }
    else
    {
        /* The mod/rm mod bits are set to 11 (i.e. register).
           Could be specials (0x0C-0x0F, etc), or the st(i) versions of
            normal opcodes. Because the opcodes are slightly different for
            this case (e.g. op=04 means FSUB if reg != 3, but FSUBR for
            reg == 3), a separate table is used (szFlops2). */
        int destRegIdx=m_dst.regi - rAX;
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
            out << Machine_X86::floatOpName(0x40+op);
            if ((op >= 0x20) and (op <= 0x27))
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
struct AsmFormatter {
    IStructuredTextTarget * target;
    int operand_count;
    void visitOperand(const LLOperand &pm) {
        if(not pm.isSet())
            return;
        if(operand_count>0) {
            target->prtt(", ");
        }
        operand_count++;
        if (pm.immed and not pm.isReg()) {
            //target->addTaggedString(XT_Keyword,szPtr[flg&B]);
            target->addTaggedString(XT_Number,strHex(pm.getImm2()));
            return;
        }

        if (pm.segOver)
        {
            target->prtt(Machine_X86::regName(pm.segOver)+':');
        }

        if (pm.regi == rUNDEF)
        {
            target->prtt(QString("[")+strHex((uint32_t)pm.off)+"]");
        }
        else if (pm.isReg())
        {
            target->prtt(Machine_X86::regName(pm.regi));
        }

        else if (pm.off)
        {
            if (pm.off < 0)
            {
                target->prtt("["+Machine_X86::regName(pm.regi)+"-"+strHex((uint32_t)(- pm.off))+"]");
            }
            else
            {
                target->prtt("["+Machine_X86::regName(pm.regi)+"+"+strHex((uint32_t)(pm.off))+"]");
            }
        }
        else
            target->prtt("["+Machine_X86::regName(pm.regi)+"]");

    }
};

void toStructuredText(LLInst *insn,IStructuredTextTarget *out, int level) {
    AsmFormatter formatter {out};
    const LLInst &inst(*insn);
    QString opcode = Machine_X86::opcodeName(insn->getOpcode());
    out->addSpace(4);
    out->addTaggedString(XT_Number,QString("%1").arg(insn->label,8,16,QChar('0').toUpper()));
    out->addSpace(4);

    out->addTaggedString(XT_Keyword,Machine_X86::opcodeName(insn->getOpcode()),insn);
    out->addSpace(2);

    switch(insn->getOpcode()) {
    case iADD:  case iADC:  case iSUB:  case iSBB:  case iAND:  case iOR:
    case iXOR:  case iTEST: case iCMP:  case iMOV:  case iLEA:  case iXCHG:

    case iSAR:  case iSHL:  case iSHR:  case iRCL:  case iRCR:  case iROL:
    case iROR:
        formatter.visitOperand(insn->dst());
        formatter.visitOperand(insn->src());
        break;

    case iINC:  case iDEC:  case iNEG:  case iNOT:  case iPOP:
        formatter.visitOperand(insn->dst());
        break;
    case iPUSH:
        formatter.visitOperand(insn->dst());
        break;

    case iDIV:  case iIDIV:  case iMUL: case iIMUL: case iMOD:
        if (inst.srcIsImmed())
        {
            formatter.visitOperand(insn->dst());
            formatter.visitOperand(insn->src());
        }
        else
            formatter.visitOperand(insn->dst());
        break;
    case iLDS:  case iLES:  case iBOUND:
        formatter.visitOperand(insn->dst());
        formatter.visitOperand(insn->src());
        break;
    case iJB:  case iJBE:  case iJAE:  case iJA:
    case iJL:  case iJLE:  case iJGE:  case iJG:
    case iJE:  case iJNE:  case iJS:   case iJNS:
    case iJO:  case iJNO:  case iJP:   case iJNP:
    case iJCXZ:case iLOOP: case iLOOPE:case iLOOPNE:
    case iJMP: case iJMPF:

        /* Check if there is a symbol here */
    {
//        ICODE *lab=pc.GetIcode(inst.src().getImm2());
//        selectTable(Label);
//        if ((inst.src().getImm2() < (uint32_t)numIcode) and  /* Ensure in range */
//                readVal(operands_s, lab->ll()->label, nullptr))
//        {
//            break;                          /* Symbolic label. Done */
//        }
    }
        if (inst.testFlags(NO_LABEL))
        {
            //strcpy(p + WID_PTR, strHex(pIcode->ll()->immed.op));
            out->addTaggedString(XT_AsmLabel,strHex(inst.src().getImm2()));
        }
        else if (inst.srcIsImmed() )
        {
            int64_t tgt_addr = inst.src().getImm2();
            if (inst.getOpcode() == iJMPF)
            {
                out->addTaggedString(XT_Keyword," far ptr ");
            }
            out->addTaggedString(XT_AsmLabel,QString("L_%1").arg(strHex(tgt_addr)));
        }
        else if (inst.getOpcode() == iJMPF)
        {
            out->addTaggedString(XT_Keyword,"dword ptr");
            formatter.visitOperand(inst.src());
        }
        else
        {
            formatter.visitOperand(inst.src());
        }

        break;
    case iCALL: case iCALLF:
        if (inst.srcIsImmed())
        {
            out->addTaggedString(XT_Keyword,QString("%1 ptr ").arg((inst.getOpcode() == iCALL) ? "near" : "far"));
            out->addTaggedString(XT_AsmLabel,(inst.src().proc.proc)->name);
        }
        else if (inst.getOpcode() == iCALLF)
        {
            out->addTaggedString(XT_Keyword,"dword ptr ");
            formatter.visitOperand(inst.src());
        }
        else
            formatter.visitOperand(inst.src());
        break;

    case iENTER:
        formatter.visitOperand(inst.dst());
        formatter.visitOperand(inst.src());
        break;

    case iRET:
    case iRETF:
    case iINT:
        formatter.visitOperand(inst.src());
        break;

    case iCMPS:  case iREPNE_CMPS:  case iREPE_CMPS:
    case iSCAS:  case iREPNE_SCAS:  case iREPE_SCAS:
    case iSTOS:  case iREP_STOS:
    case iLODS:  case iREP_LODS:
    case iMOVS:  case iREP_MOVS:
    case iINS:   case iREP_INS:
    case iOUTS:  case iREP_OUTS:
        if (inst.src().segOver)
        {
            bool is_dx_src=(inst.getOpcode() == iOUTS or inst.getOpcode() == iREP_OUTS);
            if(is_dx_src)
            {
                out->addTaggedString(XT_Symbol,"dx");
                out->prtt(", ");
                out->addTaggedString(XT_Keyword,szPtr[inst.getFlag() & B]);
                out->addSpace(2);
            }
            else
                out->addTaggedString(XT_Keyword,szPtr[inst.getFlag() & B]);
            if (inst.getOpcode() == iLODS or
                    inst.getOpcode() == iREP_LODS or
                    inst.getOpcode() == iOUTS or
                    inst.getOpcode() == iREP_OUTS)
            {
                out->addTaggedString(XT_Symbol,Machine_X86::regName(inst.src().segOver)); // szWreg[src.segOver-rAX]
            }
            else
            {
                out->addTaggedString(XT_Symbol,"es:[di]");
                out->prtt(", ");
                out->addTaggedString(XT_Symbol,Machine_X86::regName(inst.src().segOver));
            }
            out->addTaggedString(XT_Symbol,":[si]");
        }
        else
        {
            out->delChars(2); // TODO: this is wonky way of adding instruction suffix
            if(inst.getFlag() & B)
                out->addTaggedString(XT_Keyword,"B");
            else
                out->addTaggedString(XT_Keyword,"W");
            out->addSpace(2);
        }
        break;
    case iXLAT:
        if (inst.src().segOver)
        {
            out->addTaggedString(XT_Keyword,QString(" ") + szPtr[1]);
            out->addTaggedString(XT_Symbol,Machine_X86::regName(inst.src().segOver)+":[bx]");
        }
        break;

    case iIN:
        out->addTaggedString(XT_Symbol, (inst.getFlag() & B)? "al" : "ax");
        out->prtt(", ");
        formatter.visitOperand(inst.src());
        break;

    case iOUT:
    {
        formatter.visitOperand(inst.src());
        if(inst.srcIsImmed())
            out->addTaggedString(XT_Number, strHex(inst.src().getImm2()));
        else
            out->addTaggedString(XT_Symbol, "dx");
        out->prtt(", ");
        out->addTaggedString(XT_Symbol, (inst.getFlag() & B)? "al" : "ax");
    }
    }
    out->addEOL();
}
