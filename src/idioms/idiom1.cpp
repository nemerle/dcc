#include "idiom1.h"
#include "dcc.h"

/*****************************************************************************
/* checkStkVars - Checks for PUSH SI
 *                          [PUSH DI]
 *                or         PUSH DI
 *                          [PUSH SI]
 * In which case, the stack variable flags are set
 ****************************************************************************/
int Idiom1::checkStkVars (iICODE pIcode)
{
    /* Look for PUSH SI */
    int si_matched=0;
    int di_matched=0;
    if(pIcode==m_end)
        return 0;
    if (pIcode->ic.ll.match(iPUSH,rSI))
    {
        si_matched = 1;
        ++pIcode;
        if ((pIcode != m_end) && pIcode->ic.ll.match(iPUSH,rDI)) // Look for PUSH DI
            di_matched = 1;
    }
    else if (pIcode->ic.ll.match(iPUSH,rDI))
    {
        di_matched = 1;
        ++pIcode;
        if ((pIcode != m_end) && pIcode->ic.ll.match(iPUSH,rSI)) // Look for PUSH SI
            si_matched = 1;
    }
    m_func->flg |= (si_matched ? SI_REGVAR : 0) | (di_matched ? DI_REGVAR : 0);
    return si_matched+di_matched;
}
/*****************************************************************************
 * idiom1 - HLL procedure prologue;  Returns number of instructions matched.
 *          PUSH BP     ==> ENTER immed, 0
 *          MOV  BP, SP     and sets PROC_HLL flag
 *          [SUB  SP, immed]
 *          [PUSH SI]
 *          [PUSH DI]
 *        - Second version: Push stack variables and then save BP
 *          PUSH BP
 *          PUSH SI
 *          [PUSH DI]
 *          MOV BP, SP
 *        - Third version: Stack variables
 *          [PUSH SI]
 *          [PUSH DI]
 ****************************************************************************/
bool Idiom1::match(iICODE picode)
{
    uint8_t type = 0;	/* type of variable: 1 = reg-var, 2 = local */
    uint8_t regi;		/* register of the MOV */
    if(m_func->flg & PROC_HLL)
        return false;
    if(picode==m_end)
        return false;
    int n;
    m_icodes.clear();
    m_min_off = 0;
    /* PUSH BP as first instruction of procedure */
    if ( !(picode->ic.ll.flg & I) && picode->ic.ll.src.regi == rBP)
    {
        m_icodes.push_back( picode++ ); // insert iPUSH
        if(picode==m_end)
            return false;
        /* MOV BP, SP as next instruction */
        if ( !picode->ic.ll.anyFlagSet(I | TARGET | CASE) && picode->ic.ll.match(iMOV ,rBP,rSP) )
        {
            m_icodes.push_back( picode++ ); // insert iMOV
            if(picode==m_end)
                return false;
            m_min_off = 2;

            /* Look for SUB SP, immed */
            if (
                picode->ic.ll.anyFlagSet(I | TARGET | CASE) && picode->ic.ll.match(iSUB,rSP)
                )
            {
                m_icodes.push_back( picode++ ); // insert iSUB
                int n = checkStkVars (picode); // find iPUSH si [iPUSH di]
                for(int i=0; i<n; ++i)
                    m_icodes.push_back(picode++); // insert
            }
        }

        /* PUSH SI
         * [PUSH DI]
         * MOV BP, SP */
        else
        {
            int n = checkStkVars (picode);
            if (n > 0)
            {
                for(int i=0; i<n; ++i)
                    m_icodes.push_back(picode++);
                if(picode == m_end)
                    return false;
                /* Look for MOV BP, SP */
                if ( picode != m_end &&
                    !picode->ic.ll.anyFlagSet(I | TARGET | CASE) &&
                     picode->ic.ll.match(iMOV,rBP,rSP))
                {
                    m_icodes.push_back(picode);
                    m_min_off = 2 + (n * 2);
                }
                else
                    return false;	// Cristina: check this please!
            }
            else
                return false;	// Cristina: check this please!
        }
    }
    else // push di [push si] / push si [push di]
    {
        n = checkStkVars (picode);
        for(int i=0; i<n; ++i)
            m_icodes.push_back(picode++);

    }
    return !m_icodes.empty();
}
int Idiom1::action()
{
    for(iICODE ic : m_icodes)
    {
        ic->invalidate();
    }
    m_func->flg |= PROC_HLL;
    if(0!=m_min_off)
    {
        m_func->args.m_minOff = m_min_off;
        m_func->flg |= PROC_IS_HLL;
    }
    return m_icodes.size();
}
