
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
    return regNames[r];
}
