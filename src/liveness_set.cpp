#include "BasicBlock.h"
#include "machine_x86.h"

/* DU bit definitions for each reg value - including index registers */
LivenessSet duReg[] = { {},
                        //AH AL . . AX, BH
                        {rAH,rAL,rAX},{rCH,rCL,rCX},{rDH,rDL,rDX},{rBH,rBL,rBX},
                            /* uint16_t regs */
                        {rSP},{rBP},{rSI},{rDI},
                        /* seg regs     */
                        {rES},{rCS},{rSS},{rDS},
                        /* uint8_t regs    */
                        {rAL},{rCL},{rDL},{rBL},
                        {rAH},{rCH},{rDH},{rBH},
                        /* tmp reg      */
                        {rTMP},{rTMP2},
                        /* index regs   */
                        {rBX,rSI},{rBX,rDI},{rBP,rSI},{rBP,rDI},
                        {rSI},{rDI},{rBP},{rBX}
                      };

LivenessSet &LivenessSet::setReg(int r)
{
    this->reset();
    *this |= duReg[r];
    return *this;
}
LivenessSet &LivenessSet::clrReg(int r)
{
    return *this -= duReg[r];
}

LivenessSet &LivenessSet::addReg(int r)
{
    *this |= duReg[r];
//   postProcessCompositeRegs();
    return *this;
}

bool LivenessSet::testRegAndSubregs(int r) const
{
    return (*this & duReg[r]).any();
}
void LivenessSet::postProcessCompositeRegs()
{
    if(testReg(rAL) && testReg(rAH))
        registers.insert(rAX);
    if(testReg(rCL) && testReg(rCH))
        registers.insert(rCX);
    if(testReg(rDL) && testReg(rDH))
        registers.insert(rDX);
    if(testReg(rBL) && testReg(rBH))
        registers.insert(rBX);
}
