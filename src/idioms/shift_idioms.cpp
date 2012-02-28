#include "dcc.h"
#include "shift_idioms.h"
using namespace std;


/*****************************************************************************
 * idiom8 - Shift right by 1 (signed long ops)
 *      SAR reg, 1
 *      RCR reg, 1
 *      Eg:     SAR dx, 1
 *              RCR ax, 1
 *          =>  dx:ax = dx:ax >> 1      (dx:ax are signed long)
 *      Found in Microsoft C code for long signed variable shift right.
 ****************************************************************************/
bool Idiom8::match(iICODE pIcode)
{
    if(distance(pIcode,m_end)<2)
        return false;
    m_icodes[0]=pIcode++;
    m_icodes[1]=pIcode++;
    if (m_icodes[0]->isLlFlag(I) && (m_icodes[0]->ic.ll.src.op() == 1))
        if (m_icodes[1]->ic.ll.match(iRCR) &&
            m_icodes[1]->isLlFlag(I) &&
            (m_icodes[1]->ic.ll.src.op() == 1))
            return true;
    return false;
}

int Idiom8::action()
{
    int idx;
    COND_EXPR *rhs,*lhs,*expr;
    uint8_t regH,regL;
    regH=m_icodes[0]->ic.ll.dst.regi;
    regL=m_icodes[1]->ic.ll.dst.regi;
    idx = m_func->localId.newLongReg (TYPE_LONG_SIGN, regH, regL, m_icodes[0]);
    lhs = COND_EXPR::idLongIdx (idx);
    m_icodes[0]->setRegDU( regL, USE_DEF);

    rhs = COND_EXPR::idKte(1,2);
    expr = COND_EXPR::boolOp(lhs, rhs, SHR);
    m_icodes[0]->setAsgn(lhs, expr);
    m_icodes[1]->invalidate();
    return 2;
}


/*****************************************************************************
 * idiom 15 - Shift left by n
 *      SHL reg, 1
 *      SHL reg, 1
 *      [...]
 *      [SHL    reg, 1]
 *      Eg:     SHL ax, 1
 *              SHL ax, 1
 *          =>  ax = ax << 2
 *      Found in Borland Turbo C code to index an array (array multiplication)
 ****************************************************************************/
bool Idiom15::match(iICODE pIcode)
{
    int n = 1;
    uint8_t regi;

    if(distance(pIcode,m_end)<2)
        return false;
    /* Match SHL reg, 1 */
    if (not pIcode->isLlFlag(I) or (pIcode->ic.ll.src.op() != 1))
        return false;
    m_icodes.clear();
    regi = pIcode->ic.ll.dst.regi;
    m_icodes.push_back(pIcode++);
    while(  (pIcode!=m_end) and
            pIcode->ic.ll.match(iSHL,(eReg)regi) and
            pIcode->isLlFlag(I) and
            (pIcode->ic.ll.src.op() == 1) )
    {
        n++;
        m_icodes.push_back(pIcode++);
    }
    return m_icodes.size()>1;
}

int Idiom15::action()
{
    COND_EXPR *lhs,*rhs,*exp;
    lhs = COND_EXPR::idReg (m_icodes[0]->ic.ll.dst.regi,
                            m_icodes[0]->ic.ll.flg & NO_SRC_B,
                            &m_func->localId);
    rhs = COND_EXPR::idKte (m_icodes.size(), 2);
    exp = COND_EXPR::boolOp (lhs, rhs, SHL);
    m_icodes[0]->setAsgn(lhs, exp);
    for (size_t i=1; i<m_icodes.size()-1; ++i)
    {
        m_icodes[i]->invalidate();
    }
    return m_icodes.size();
}

/*****************************************************************************
 * idiom12 - Shift left long by 1
 *      SHL reg, 1
 *      RCL reg, 1
 *      Eg:     SHL ax, 1
 *              RCL dx, 1
 *          =>  dx:ax = dx:ax << 1
 *      Found in Borland Turbo C code for long variable shift left.
 ****************************************************************************/
bool Idiom12::match(iICODE pIcode)
{
    if(distance(pIcode,m_end)<2)
        return false;
    m_icodes[0]=pIcode++;
    m_icodes[1]=pIcode++;
    if (m_icodes[0]->isLlFlag(I) && (m_icodes[0]->ic.ll.src.op() == 1))
        if (m_icodes[1]->ic.ll.match(iRCL) &&
                m_icodes[1]->isLlFlag(I) && (m_icodes[1]->ic.ll.src.op() == 1))
            return true;
    return false;
}

int Idiom12::action()
{
    int idx;
    COND_EXPR *rhs,*lhs,*expr;
    uint8_t regH,regL;
    regL=m_icodes[0]->ic.ll.dst.regi;
    regH=m_icodes[1]->ic.ll.dst.regi;

    idx = m_func->localId.newLongReg (TYPE_LONG_UNSIGN, regH, regL,m_icodes[0]);
    lhs = COND_EXPR::idLongIdx (idx);
    m_icodes[0]->setRegDU( regH, USE_DEF);
    rhs = COND_EXPR::idKte (1, 2);
    expr = COND_EXPR::boolOp (lhs, rhs, SHL);
    m_icodes[0]->setAsgn(lhs, expr);
    m_icodes[1]->invalidate();
    return 2;
}

/*****************************************************************************
 * idiom9 - Shift right by 1 (unsigned long ops)
 *      SHR reg, 1
 *      RCR reg, 1
 *      Eg:     SHR dx, 1
 *              RCR ax, 1
 *          =>  dx:ax = dx:ax >> 1   (dx:ax are unsigned long)
 *      Found in Microsoft C code for long unsigned variable shift right.
 ****************************************************************************/
bool Idiom9::match(iICODE pIcode)
{
    if(distance(pIcode,m_end)<2)
        return false;
    m_icodes[0]=pIcode++;
    m_icodes[1]=pIcode++;
    if (m_icodes[0]->isLlFlag(I) && (m_icodes[0]->ic.ll.src.op() == 1))
        if (m_icodes[1]->ic.ll.match(iRCR) &&
                m_icodes[1]->isLlFlag(I) && (m_icodes[1]->ic.ll.src.op() == 1))
            return true;
    return false;
}

int Idiom9::action()
{
    int idx;
    COND_EXPR *rhs,*lhs,*expr;
    uint8_t regH,regL;
    regL=m_icodes[1]->ic.ll.dst.regi;
    regH=m_icodes[0]->ic.ll.dst.regi;
    idx = m_func->localId.newLongReg (TYPE_LONG_UNSIGN,regH,regL,m_icodes[0]);
    lhs = COND_EXPR::idLongIdx (idx);
    m_icodes[0]->setRegDU(regL, USE_DEF);
    rhs = COND_EXPR::idKte (1, 2);
    expr = COND_EXPR::boolOp (lhs, rhs, SHR);
    m_icodes[0]->setAsgn(lhs, expr);
    m_icodes[1]->invalidate();
    return 2;
}
