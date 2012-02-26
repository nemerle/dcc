#include "dcc.h"
#include "epilogue_idioms.h"

/*****************************************************************************
 * popStkVars - checks for
 *          [POP DI]
 *          [POP SI]
 *      or  [POP SI]
 *          [POP DI]
 ****************************************************************************/
void EpilogIdiom::popStkVars(iICODE pIcode)
{
    // TODO : only process SI-DI DI-SI pairings, no SI-SI, DI-DI like it's now
    /* Match [POP DI] */
    if (pIcode->ic.ll.match(iPOP))
    {
        if ((m_func->flg & DI_REGVAR) && pIcode->ic.ll.match(rDI))
            m_icodes.push_front(pIcode);
        else if ((m_func->flg & SI_REGVAR) && pIcode->ic.ll.match(rSI))
            m_icodes.push_front(pIcode);
    }
    ++pIcode;
    if(pIcode==m_end)
        return;
    /* Match [POP SI] */
    if (pIcode->ic.ll.match(iPOP))
    {
        if ((m_func->flg & SI_REGVAR) && pIcode->ic.ll.match(rSI))
            m_icodes.push_front(pIcode);
        else if ((m_func->flg & DI_REGVAR) && pIcode->ic.ll.match(rDI))
            m_icodes.push_front(pIcode);
    }
}


/*****************************************************************************
 * idiom2 - HLL procedure epilogue;  Returns number of instructions matched.
 *          [POP DI]
 *          [POP SI]
 *          MOV  SP, BP
 *          POP  BP
 *          RET(F)
 *****************************************************************************/
bool Idiom2::match(iICODE pIcode)
{
    iICODE nicode;
    if(pIcode==m_func->Icode.begin()) // pIcode->loc_ip == 0
        return false;
    if ( ((pIcode->ic.ll.flg & I) == I) || not pIcode->ic.ll.match(rSP,rBP))
        return false;
    if(distance(pIcode,m_end)<3)
        return false;
    /* Matched MOV SP, BP */
    m_icodes.clear();
    m_icodes.push_back(pIcode);
    /* Get next icode, skip over holes in the icode array */
    nicode = pIcode + 1;
    while (nicode->ic.ll.flg & NO_CODE && (nicode != m_end))
    {
        nicode++;
    }
    if(nicode == m_end)
        return false;

    if (nicode->ic.ll.match(iPOP,rBP) && ! (nicode->ic.ll.flg & (I | TARGET | CASE)) )
    {
        m_icodes.push_back(nicode++); // Matched POP BP

        /* Match RET(F) */
        if (    nicode != m_end &&
                !(nicode->ic.ll.flg & (I | TARGET | CASE)) &&
                (nicode->ic.ll.match(iRET) || nicode->ic.ll.match(iRETF))
                )
        {
            m_icodes.push_back(nicode); // Matched RET
            popStkVars (pIcode-2); // will add optional pop di/si to m_icodes
            return true;
        }
    }
    return false;
}
int Idiom2::action()
{
    for(size_t idx=0; idx<m_icodes.size()-1; ++idx) // don't invalidate last entry
        m_icodes[idx]->invalidate();
    return 3;

}

/*****************************************************************************
 * idiom4 - Pascal calling convention.
 *          RET(F) immed
 *          ==> pProc->cbParam = immed
 *              sets CALL_PASCAL flag
 *        - Second version: check for optional pop of stack vars
 *          [POP DI]
 *          [POP SI]
 *          POP BP
 *          RET(F) [immed]
 *		  - Third version: pop stack vars
 *			[POP DI]
 *			[POP SI]
 *			RET(F) [immed]
 ****************************************************************************/
bool Idiom4::match(iICODE pIcode)
{
    m_param_count = 0;
    /* Check for [POP DI]
     *           [POP SI] */
    if(distance(m_func->Icode.begin(),pIcode)>=3)
        popStkVars (pIcode-3);
    if(pIcode != m_func->Icode.begin())
    {
        iICODE prev1=pIcode-1;
        /* Check for POP BP */
        if (prev1->ic.ll.match(iPOP,rBP) && not prev1->ic.ll.anyFlagSet(I) )
            m_icodes.push_back(prev1);
        else if(prev1!=m_func->Icode.begin())
            popStkVars (pIcode-2);
    }

    /* Check for RET(F) immed */
    if (pIcode->ic.ll.flg & I)
    {
        m_param_count = (int16)pIcode->ic.ll.src.op();
    }
}
int Idiom4::action()
{
    for(size_t idx=0; idx<m_icodes.size()-1; ++idx) // don't invalidate last entry
        m_icodes[idx]->invalidate();
    if(m_param_count)
    {
        m_func->cbParam = (int16)m_param_count;
        m_func->flg |= CALL_PASCAL;
    }
    return 0;
}

