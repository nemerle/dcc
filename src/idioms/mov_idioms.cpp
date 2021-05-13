#include "mov_idioms.h"

#include "Procedure.h"
#include "msvc_fixes.h"

using namespace std;

/*****************************************************************************
 * idiom 14 - Long uint16_t assign
 *      MOV regL, mem/reg
 *      XOR regH, regH
 *      Eg:     MOV ax, di
 *              XOR dx, dx
 *      => MOV dx:ax, di
 *		Note: only the following combinations are allowed:
 *				dx:ax
 *				cx:bx
 *		this is to remove the possibility of making errors in situations
 *		like this:
 *			MOV dx, offH
 *			MOV ax, offL
 *			XOR	cx, cx
 *      Found in Borland Turbo C, used for division of unsigned integer
 *      operands.
 ****************************************************************************/

bool Idiom14::match(iICODE pIcode)
{
    if(distance(pIcode,m_end)<2)
        return false;
    m_icodes[0]=pIcode++;
    m_icodes[1]=pIcode++;
    LLInst * matched [] {m_icodes[0]->ll(),m_icodes[1]->ll()};
    /* Check for regL */
    m_regL = matched[0]->m_dst.regi;
    if (not matched[0]->testFlags(I) and ((m_regL == rAX) or (m_regL ==rBX)))
    {
        /* Check for XOR regH, regH */
        if (matched[1]->match(iXOR) and not matched[1]->testFlags(I))
        {
            m_regH = matched[1]->m_dst.regi;
            if (m_regH == matched[1]->src().getReg2())
            {
                if ((m_regL == rAX) and (m_regH == rDX))
                    return true;
                if ((m_regL == rBX) and (m_regH == rCX))
                    return true;
            }
        }
    }
    return false;
}
int Idiom14::action()
{

    int idx = m_func->localId.newLongReg (TYPE_LONG_SIGN, LONGID_TYPE(m_regH,m_regL), m_icodes[0]);
    AstIdent *lhs = AstIdent::LongIdx (idx);
    m_icodes[0]->setRegDU( m_regH, eDEF);
    Expr *rhs = AstIdent::id (*m_icodes[0]->ll(), SRC, m_func, m_icodes[0], *m_icodes[0], NONE);
    m_icodes[0]->setAsgn(lhs, rhs);
    m_icodes[1]->invalidate();
    return 2;
}


/*****************************************************************************
 * idiom 13 - uint16_t assign
 *      MOV regL, mem
 *      MOV regH, 0
 *      Eg:     MOV al, [bp-2]
 *              MOV ah, 0
 *      => MOV ax, [bp-2]
 *      Found in Borland Turbo C, used for multiplication and division of
 *      uint8_t operands (ie. they need to be extended to words).
 ****************************************************************************/
bool Idiom13::match(iICODE pIcode)
{
    if(distance(pIcode,m_end)<2)
        return false;
    m_icodes[0]=pIcode++;
    m_icodes[1]=pIcode++;
    m_loaded_reg = rUNDEF;
    eReg regi;

    /* Check for regL */
    regi = m_icodes[0]->ll()->m_dst.regi;
    if (not m_icodes[0]->ll()->testFlags(I) and (regi >= rAL) and (regi <= rBH))
    {
        /* Check for MOV regH, 0 */
        if (m_icodes[1]->ll()->match(iMOV,I) and (m_icodes[1]->ll()->src().getImm2() == 0))
        {
            if (m_icodes[1]->ll()->m_dst.regi == (regi + 4)) //WARNING: based on distance between AH-AL,BH-BL etc.
            {
                m_loaded_reg=(eReg)(regi - rAL + rAX);
                return true;
            }
        }
    }
    return false;
}

int Idiom13::action()
{
    AstIdent *lhs;
    Expr *rhs;
    eReg regi = m_icodes[0]->ll()->m_dst.regi;
    m_icodes[0]->du1.removeDef(regi);
    //m_icodes[0]->du1.numRegsDef--;   	/* prev uint8_t reg def */
    lhs = new RegisterNode(LLOperand(m_loaded_reg, 0), &m_func->localId);
    m_icodes[0]->setRegDU( m_loaded_reg, eDEF);
    rhs = AstIdent::id (*m_icodes[0]->ll(), SRC, m_func, m_icodes[0], *m_icodes[0], NONE);
    m_icodes[0]->setAsgn(lhs, rhs);
    m_icodes[1]->invalidate();
    return 2;
}

