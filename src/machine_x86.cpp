#include <cassert>
#include "machine_x86.h"
// Index registers **** temp solution
static const std::string regNames[] = {
    "undef",
    "ax", "cx", "dx", "bx",
    "sp", "bp", "si", "di",
    "es", "cs", "ss", "ds",
    "al", "cl", "dl", "bl",
    "ah", "ch", "dh", "bh",
    "tmp",
    "bx+si", "bx+di", "bp+si", "bp+di",
    "si", "di", "bp", "bx"
};

/* uint8_t and uint16_t registers */
Machine_X86::Machine_X86()
{
    static_assert((sizeof(regNames)/sizeof(std::string))==LAST_REG,
                  "Reg count not equal number of strings");
}

const std::string &Machine_X86::regName(eReg r)
{
    assert(r<(sizeof(regNames)/sizeof(std::string)));
    return regNames[r];
}

static const std::string szOps[] =
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
    "XLAT", "XOR",      "INTO",     "NOP",      "REPNE",    "REPE",	"MOD"
};
/* The following opcodes are for mod != 3 */
static std::string szFlops1[] =
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
static std::string szFlops2[] =
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

const std::string &Machine_X86::opcodeName(unsigned r)
{
    assert(r<(sizeof(szOps)/sizeof(std::string)));
    return szOps[r];
}
const std::string &Machine_X86::floatOpName(unsigned r)
{
    if(r>=(sizeof(szFlops1)/sizeof(std::string)))
    {
        r-= (sizeof(szFlops1)/sizeof(std::string));
        assert(r<(sizeof(szFlops2)/sizeof(std::string)));
        return szFlops2[r];
    }
    return szFlops1[r];
}

bool Machine_X86::physicalReg(eReg r)
{
    return (r>=rAX) && (r<rTMP);
}
bool Machine_X86::isMemOff(eReg r)
{
    return r == 0 || r >= INDEX_BX_SI;
}
//TODO: Move these to Machine_X86
eReg Machine_X86::subRegH(eReg reg)
{
    return eReg((int)reg + (int)rAH-(int)rAX);
}
eReg Machine_X86::subRegL(eReg reg)
{
    return eReg((int)reg + (int)rAL-(int)rAX);
}
bool Machine_X86::isSubRegisterOf(eReg reg,eReg parent)
{
    if ((parent < rAX) || (parent > rBX))
        return false; // only AX -> BX are coverede by subregisters
    return ((reg==subRegH(parent)) || (reg == subRegL(parent)));
}
bool Machine_X86::hasSubregisters(eReg reg)
{
    return ((reg >= rAX) && (reg <= rBX));
}

bool Machine_X86::isPartOfComposite(eReg reg)
{
    return ((reg >= rAL) && (reg <= rBH));
}

eReg Machine_X86::compositeParent(eReg reg)
{
    switch(reg)
    {
        case rAL: case rAH: return rAX;
        case rCL: case rCH: return rCX;
        case rDL: case rDH: return rDX;
        case rBL: case rBH: return rBX;
        default:
            return rUNDEF;
    }
    return rUNDEF;
}
