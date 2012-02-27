#include "dcc.h"
#include "mov_idioms.h"
using namespace std;

/*****************************************************************************
 * idiom 14 - Long word assign
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
    /* Check for regL */
    m_regL = m_icodes[0]->ic.ll.dst.regi;
    if (not m_icodes[0]->isLlFlag(I) && ((m_regL == rAX) || (m_regL ==rBX)))
    {
        /* Check for XOR regH, regH */
        if (m_icodes[1]->ic.ll.match(iXOR) && not m_icodes[1]->isLlFlag(I))
        {
            m_regH = m_icodes[1]->ic.ll.dst.regi;
            if (m_regH == m_icodes[1]->ic.ll.src.regi)
            {
                if ((m_regL == rAX) && (m_regH == rDX))
                    return true;
                if ((m_regL == rBX) && (m_regH == rCX))
                    return true;
            }
        }
    }
    return false;
}
int Idiom14::action()
{
    int idx;
    COND_EXPR *lhs,*rhs;
    idx = m_func->localId.newLongReg (TYPE_LONG_SIGN, m_regH, m_regL, m_icodes[0]);
    lhs = COND_EXPR::idLongIdx (idx);
    m_icodes[0]->setRegDU( m_regH, eDEF);
    rhs = COND_EXPR::id (*m_icodes[0], SRC, m_func, m_icodes[0], *m_icodes[0], NONE);
    m_icodes[0]->setAsgn(lhs, rhs);
    m_icodes[1]->invalidate();
    return 2;
}


/*****************************************************************************
 * idiom 13 - Word assign
 *      MOV regL, mem
 *      MOV regH, 0
 *      Eg:     MOV al, [bp-2]
 *              MOV ah, 0
 *      => MOV ax, [bp-2]
 *      Found in Borland Turbo C, used for multiplication and division of
 *      byte operands (ie. they need to be extended to words).
 ****************************************************************************/
bool Idiom13::match(iICODE pIcode)
{
    if(distance(pIcode,m_end)<2)
        return false;
    m_icodes[0]=pIcode++;
    m_icodes[1]=pIcode++;
    m_loaded_reg = 0;
    byte regi;

    /* Check for regL */
    regi = m_icodes[0]->ic.ll.dst.regi;
    if (not m_icodes[0]->isLlFlag(I) && (regi >= rAL) && (regi <= rBH))
    {
        /* Check for MOV regH, 0 */
        if (m_icodes[1]->ic.ll.match(iMOV) && m_icodes[1]->isLlFlag(I) && (m_icodes[1]->ic.ll.src.op() == 0))
        {
            if (m_icodes[1]->ic.ll.dst.regi == (regi + 4)) //TODO: based on distance between AH-AL,BH-BL etc.
            {
                m_loaded_reg=(regi - rAL + rAX);
                return true;
            }
        }
    }
    return false;
}

int Idiom13::action()
{
    COND_EXPR *lhs,*rhs;
    lhs = COND_EXPR::idReg (m_loaded_reg, 0, &m_func->localId);
    m_icodes[0]->setRegDU( m_loaded_reg, eDEF);
    m_icodes[0]->du1.numRegsDef--;   	/* prev byte reg def */
    rhs = COND_EXPR::id (*m_icodes[0], SRC, m_func, m_icodes[0], *m_icodes[0], NONE);
    m_icodes[0]->setAsgn(lhs, rhs);
    m_icodes[1]->invalidate();
    return 2;
}

