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

const std::string &Machine_X86::opcodeName(unsigned r)
{
    assert(r<(sizeof(szOps)/sizeof(std::string)));
    return szOps[r];
}

bool Machine_X86::physicalReg(eReg r)
{
    return (r>=rAX) && (r<rTMP);
}
