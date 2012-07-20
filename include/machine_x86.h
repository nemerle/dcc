#pragma once
#include <stdint.h>
#include <string>
#include <sstream>
#include <bitset>

struct LivenessSet;
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
    rTMP2=       22, /* temp register for DIV/IDIV/MOD	*/
    /* Indexed modes go from INDEXBASE to INDEXBASE+7  */
    INDEX_BX_SI = 23, // "bx+si"
    INDEX_BX_DI, // "bx+di"
    INDEX_BP_SI, // "bp+si"
    INDEX_BP_DI, // "bp+di"
    INDEX_SI, // "si"
    INDEX_DI, // "di"
    INDEX_BP, // "bp"
    INDEX_BX, // "bx"
    LAST_REG
};
class SourceMachine
{
public:
    virtual bool physicalReg(eReg r)=0;

};
//class Machine_X86_Disassembler
//{
//    void formatRM(std::ostringstream &p, uint32_t flg, const LLOperand &pm);
//};
class Machine_X86 : public SourceMachine
{
public:
    Machine_X86();
    virtual ~Machine_X86() {}
    static const std::string &regName(eReg r);
    static const std::string &opcodeName(unsigned r);
    static const std::string &floatOpName(unsigned r);
    bool physicalReg(eReg r);
    /* Writes the registers that are set in the bitvector */
    //TODO: move this into Machine_X86 ?
    static void writeRegVector (std::ostream &ostr,const LivenessSet &regi);
    static eReg subRegH(eReg reg);
    static eReg subRegL(eReg reg);
    static bool isMemOff(eReg r);
    static bool isSubRegisterOf(eReg reg, eReg parent);
    static bool hasSubregisters(eReg reg);

    static bool isPartOfComposite(eReg reg);
    static eReg compositeParent(eReg reg);

};
