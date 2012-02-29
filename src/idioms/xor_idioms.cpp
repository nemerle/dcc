#include "dcc.h"
#include "xor_idioms.h"
using namespace std;

/*****************************************************************************
 * idiom21 - Assign long kte with high part zero
 *		XOR regH, regH
 *		MOV regL, kte
 *		=> regH:regL = kte
 *		Eg: XOR dx, dx
 *			MOV ax, 3
 *			=> dx:ax = 3
 *		Note: only the following valid combinations are available:
 *				dx:ax
 *				cx:bx
 *		Found in Borland Turbo C code.
 ****************************************************************************/
bool Idiom21::match (iICODE picode)
{
    LLOperand *dst, *src;
    if(distance(picode,m_end)<2)
        return false;
    m_icodes[0]=picode++;
    m_icodes[1]=picode++;

    if (not m_icodes[1]->ll()->testFlags(I))
        return false;

    dst = &m_icodes[0]->ll()->dst;
    src = &m_icodes[0]->ll()->src;
    if ((dst->regi == src->regi) && (dst->regi > 0) && (dst->regi < INDEXBASE))
    {
        if ((dst->regi == rDX) && m_icodes[1]->ll()->match(rAX))
            return true;
        if ((dst->regi == rCX) && m_icodes[1]->ll()->match(rBX))
            return true;
    }
    return false;
}
int Idiom21::action()
{
    COND_EXPR *lhs,*rhs;
    lhs = COND_EXPR::idLong (&m_func->localId, DST, m_icodes[0],HIGH_FIRST, m_icodes[0], eDEF, m_icodes[1]);
    rhs = COND_EXPR::idKte (m_icodes[1]->ll()->src.op() , 4);
    m_icodes[0]->setAsgn(lhs, rhs);
    m_icodes[0]->du.use = 0;		/* clear register used in iXOR */
    m_icodes[1]->invalidate();
    return 2;
}

/*****************************************************************************
 * idiom7 - Assign zero
 *      XOR reg/stackOff, reg/stackOff
 *      Eg:     XOR ax, ax
 *          =>  ax = 0
 *      Found in Borland Turbo C and Microsoft C code.
 ****************************************************************************/
bool Idiom7::match(iICODE picode)
{
    if(picode==m_end)
        return false;
    LLOperand *dst, *src;
    m_icode=picode;
    dst = &picode->ll()->dst;
    src = &picode->ll()->src;
    if (dst->regi == 0)                 /* global variable */
    {
        if ((dst->segValue == src->segValue) && (dst->off == src->off))
            return true;
    }
    else if (dst->regi < INDEXBASE)     /* register */
    {
        if (dst->regi == src->regi)
            return true;
    }
    else if ((dst->off) && (dst->seg == rSS) && (dst->regi == INDEXBASE + 6)) /* offset from BP */
    {
        if ((dst->off == src->off) && (dst->seg == src->seg) && (dst->regi == src->regi))
            return true;
    }
    return false;
}
int Idiom7::action()
{
    COND_EXPR *lhs,*rhs;
    lhs = COND_EXPR::id (*m_icode, DST, m_func, m_icode, *m_icode, NONE);
    rhs = COND_EXPR::idKte (0, 2);
    m_icode->setAsgn(lhs, rhs);
    m_icode->du.use = 0;    /* clear register used in iXOR */
    m_icode->ll()->setFlags(I);
    return 1;
}


/*****************************************************************************
 * idiom10 - Jump if not equal to 0
 *      OR  reg, reg
 *      JNE labX
 *      Eg:     OR  ax, ax
 *              JNE labX
 *      => CMP reg 0
 *		   JNE labX
 *		This instruction is NOT converted into the equivalent high-level
 *		instruction "HLI_JCOND (reg != 0) labX" because we do not know yet if
 *		it forms part of a long register conditional test.  It is therefore
 *		modified to simplify the analysis.
 *      Found in Borland Turbo C.
 ****************************************************************************/
bool Idiom10::match(iICODE pIcode)
{
    if(distance(pIcode,m_end)<2)
        return false;
    m_icodes[0]=pIcode++;
    m_icodes[1]=pIcode++;
    /* Check OR reg, reg */
    if (not m_icodes[0]->ll()->testFlags(I)  &&
            (m_icodes[0]->ll()->src.regi > 0) &&
            (m_icodes[0]->ll()->src.regi < INDEXBASE) &&
            (m_icodes[0]->ll()->src.regi == m_icodes[0]->ll()->dst.regi))
        if (m_icodes[1]->ll()->match(iJNE)) //.conditionalJump()
        {
            return true;
        }
    return false;
}

int Idiom10::action()
{
    m_icodes[0]->ll()->opcode = iCMP;
    m_icodes[0]->ll()->setFlags(I);
      m_icodes[0]->ll()->src.SetImmediateOp(0); // todo check if proc should be zeroed too
    m_icodes[0]->du.def = 0;
    m_icodes[0]->du1.numRegsDef = 0;
    return 2;

}
