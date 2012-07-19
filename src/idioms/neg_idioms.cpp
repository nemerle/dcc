#include "dcc.h"
#include "neg_idioms.h"
using namespace std;


/*****************************************************************************
 * idiom11 - Negate long integer
 *      NEG regH
 *      NEG regL
 *      SBB regH, 0
 *      Eg:     NEG dx
 *              NEG ax
 *              SBB dx, 0
 *      => dx:ax = - dx:ax
 *      Found in Borland Turbo C.
 ****************************************************************************/
bool Idiom11::match (iICODE picode)
{
    //const char *matchstring="(oNEG rH) (oNEG rL) (SBB \rH i0)";
    condId type;          /* type of argument */
    if(distance(picode,m_end)<3)
        return false;
    for(int i=0; i<3; ++i)
        m_icodes[i]=picode++;
    type = m_icodes[0]->ll()->idType(DST);
    if(type==CONSTANT || type == OTHER)
        return false;
    /* Check NEG reg/mem
     *       SBB reg/mem, 0*/
    if (not m_icodes[1]->ll()->match(iNEG) or not m_icodes[2]->ll()->match(iSBB))
        return false;
    switch (type)
    {
        case GLOB_VAR:
            if ((m_icodes[2]->ll()->dst.segValue == m_icodes[0]->ll()->dst.segValue) &&
                    (m_icodes[2]->ll()->dst.off == m_icodes[0]->ll()->dst.off))
                return true;
            break;
        case REGISTER:
            if (m_icodes[2]->ll()->dst.regi == m_icodes[0]->ll()->dst.regi)
                return true;
            break;
        case PARAM:
        case LOCAL_VAR:
            if (m_icodes[2]->ll()->dst.off == m_icodes[0]->ll()->dst.off)
                return true;
            break;
        default:
            fprintf(stderr,"Idiom11::match unhandled type %d\n",type);
    }
    return false;
}
int Idiom11::action()
{
    AstIdent *lhs;
    Expr *rhs;
    lhs = AstIdent::Long (&m_func->localId, DST, m_icodes[0], HIGH_FIRST,m_icodes[0], USE_DEF, *m_icodes[1]->ll());
    rhs = UnaryOperator::Create(NEGATION, lhs);
    m_icodes[0]->setAsgn(lhs, rhs);
    m_icodes[1]->invalidate();
    m_icodes[2]->invalidate();
    return 3;
}

/*****************************************************************************
 * idiom 16: Bitwise negation
 *		NEG reg
 *		SBB reg, reg
 *		INC reg
 *		=> ASGN reg, !reg
 *		Eg:		NEG ax
 *				SBB ax, ax
 *				INC ax
 *			=> ax = !ax
 *		Found in Borland Turbo C when negating bitwise.
 ****************************************************************************/
bool Idiom16::match (iICODE picode)
{
    //const char *matchstring="(oNEG rR) (oSBB rR rR) (oINC rR)";
    if(distance(picode,m_end)<3)
        return false;
    for(int i=0; i<3; ++i)
        m_icodes[i]=picode++;

    uint8_t regi = m_icodes[0]->ll()->dst.regi;
    if ((regi >= rAX) && (regi < INDEX_BX_SI))
    {
        if (m_icodes[1]->ll()->match(iSBB) && m_icodes[2]->ll()->match(iINC))
            if ((m_icodes[1]->ll()->dst.regi == (m_icodes[1]->ll()->src().getReg2())) &&
                    m_icodes[1]->ll()->match((eReg)regi) &&
                    m_icodes[2]->ll()->match((eReg)regi))
                return true;
    }
    return false;
}
int Idiom16::action()
{
    AstIdent *lhs;
    Expr *rhs;
    lhs = new RegisterNode(m_icodes[0]->ll()->dst.regi, m_icodes[0]->ll()->getFlag(),&m_func->localId);
    rhs = UnaryOperator::Create(NEGATION, lhs->clone());
    m_icodes[0]->setAsgn(lhs, rhs);
    m_icodes[1]->invalidate();
    m_icodes[2]->invalidate();
    return 3;
}
