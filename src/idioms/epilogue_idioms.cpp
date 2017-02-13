#include "epilogue_idioms.h"

#include "dcc.h"
#include "msvc_fixes.h"

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
    if (pIcode->ll()->match(iPOP))
    {
        if ((m_func->flg & DI_REGVAR) and pIcode->ll()->match(rDI))
            m_icodes.push_front(pIcode);
        else if ((m_func->flg & SI_REGVAR) and pIcode->ll()->match(rSI))
            m_icodes.push_front(pIcode);
    }
    ++pIcode;
    if(pIcode==m_end)
        return;
    /* Match [POP SI] */
    if (pIcode->ll()->match(iPOP))
    {
        if ((m_func->flg & SI_REGVAR) and pIcode->ll()->match(rSI))
            m_icodes.push_front(pIcode);
        else if ((m_func->flg & DI_REGVAR) and pIcode->ll()->match(rDI))
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
    if ( pIcode->ll()->testFlags(I) or (not pIcode->ll()->match(rSP,rBP)) )
        return false;
    if(distance(pIcode,m_end)<3)
        return false;
    /* Matched MOV SP, BP */
    m_icodes.clear();
    m_icodes.push_back(pIcode);
    /* Get next icode, skip over holes in the icode array */
    nicode = ++iICODE(pIcode);
    while (nicode->ll()->testFlags(NO_CODE) and (nicode != m_end))
    {
        nicode++;
    }
    if(nicode == m_end)
        return false;

    if (nicode->ll()->match(iPOP,rBP) and not (nicode->ll()->testFlags(I | TARGET | CASE)) )
    {
        m_icodes.push_back(nicode++); // Matched POP BP

        /* Match RET(F) */
        if (    nicode != m_end and
                not (nicode->ll()->testFlags(I | TARGET | CASE)) and
                (nicode->ll()->match(iRET) or nicode->ll()->match(iRETF))
                )
        {
            m_icodes.push_back(nicode); // Matched RET
            advance(pIcode,-2); // move back before our start
            popStkVars (pIcode); // and add optional pop di/si to m_icodes
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
    {
        iICODE search_at(pIcode);
        advance(search_at,-3);
        popStkVars(search_at);
    }
    if(pIcode != m_func->Icode.begin())
    {
        iICODE prev1 = --iICODE(pIcode);
        /* Check for POP BP */
        if (prev1->ll()->match(iPOP,rBP) and not prev1->ll()->testFlags(I) )
            m_icodes.push_back(prev1);
        else if(prev1!=m_func->Icode.begin())
        {
            iICODE search_at(pIcode);
            advance(search_at,-2);
            popStkVars (search_at);
        }
    }

    /* Check for RET(F) immed */
    if (pIcode->ll()->testFlags(I) )
    {
        m_param_count = (int16_t)pIcode->ll()->src().getImm2();
        return true;
    }
    return false;
}
int Idiom4::action()
{
    if( not m_icodes.empty()) // if not an empty RET[F] N
    {
    for(size_t idx=0; idx<m_icodes.size()-1; ++idx) // don't invalidate last entry
            m_icodes[idx]->invalidate();
    }
    if(m_param_count)
    {
        m_func->cbParam = (int16_t)m_param_count;
        m_func->callingConv(CConv::ePascal);
    }
    return 1;
}

