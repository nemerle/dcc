#pragma once
#include <string>
/* Machine registers */
enum eReg
{
    rUNDEF =     0,
    rAX =        1,  /* These are numbered relative to real 8086 */
    rCX =        2,
    rDX =        3,
    rBX =        4,
    rSP =        5,
    rBP =        6,
    rSI =        7,
    rDI =        8,

    rES =        9,
    rCS =       10,
    rSS =       11,
    rDS =       12,

    rAL =       13,
    rCL =       14,
    rDL =       15,
    rBL =       16,
    rAH =       17,
    rCH =       18,
    rDH =       19,
    rBH =       20,

    rTMP=       21, /* temp register for DIV/IDIV/MOD	*/
    /* Indexed modes go from INDEXBASE to INDEXBASE+7  */
    INDEX_BX_SI = 22, // "bx+si"
    INDEX_BX_DI, // "bx+di"
    INDEX_BP_SI, // "bp+si"
    INDEX_BP_DI, // "bp+di"
    INDEX_SI, // "si"
    INDEX_DI, // "di"
    INDEX_BP, // "bp"
    INDEX_BX, // "bx"
    LAST_REG
};

class Machine_X86
{
public:
    Machine_X86();
    static const std::string &regName(eReg r);
};
