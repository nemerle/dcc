#include "dcc.h"
#include "call_idioms.h"
using namespace std;
/*****************************************************************************
 * idiom3 - C calling convention.
 *          CALL(F)  proc_X
 *          ADD  SP, immed
 *      Eg: CALL proc_X
 *          ADD  SP, 6
 *          =>  pProc->cbParam = immed
 *		Special case: when the call is at the end of the procedure,
 *					  sometimes the stack gets restored by a MOV sp, bp.
 *					  Need to flag the procedure in these cases.
 *  Used by compilers to restore the stack when invoking a procedure using
 *  the C calling convention.
 ****************************************************************************/
bool Idiom3::match(iICODE picode)
{
    if(distance(picode,m_end)<2)
        return false;
    m_param_count=0;
    /* Match ADD  SP, immed */
    for(int i=0; i<2; ++i)
        m_icodes[i] = picode++;
    if ( (m_icodes[1]->ic.ll.flg & I) && m_icodes[1]->ic.ll.match(iADD,rSP))
    {
        m_param_count = m_icodes[1]->ic.ll.src.op();
        return true;
    }
    else if (m_icodes[1]->ic.ll.match(iMOV,rSP,rBP))
    {
        m_icodes[0]->ic.ll.flg |= REST_STK;
        return true;
    }
    return 0;
}
int Idiom3::action()
{
    if (m_icodes[0]->ic.ll.flg & I)
    {
        m_icodes[0]->ic.ll.src.proc.proc->cbParam = (int16_t)m_param_count;
        m_icodes[0]->ic.ll.src.proc.cb = m_param_count;
        m_icodes[0]->ic.ll.src.proc.proc->flg |= CALL_C;
    }
    else
    {
        printf("Indirect call at idiom3\n");
    }
    m_icodes[1]->invalidate();
    return 2;
}


/*****************************************************************************
 * idiom 17 - C calling convention.
 *          CALL(F)  xxxx
 *          POP reg
 *          [POP reg]           reg in {AX, BX, CX, DX}
 *      Eg: CALL proc_X
 *          POP cx
 *          POP cx      (4 bytes of arguments)
 *          => pProc->cbParam = # pops * 2
 *  Found in Turbo C when restoring the stack for a procedure that uses the
 *  C calling convention.  Used to restore the stack of 2 or 4 bytes args.
 ****************************************************************************/
bool Idiom17::match(iICODE picode)
{
    if(distance(picode,m_end)<2)
        return false;
    m_param_count=0; /* Count on # pops */
    m_icodes.clear();

    /* Match ADD  SP, immed */
    for(int i=0; i<2; ++i)
        m_icodes.push_back(picode++);
    uint8_t regi;

    /* Match POP reg */
    if (m_icodes[1]->ic.ll.match(iPOP))
    {
        int i=0;
        regi = m_icodes[1]->ic.ll.dst.regi;
        if ((regi >= rAX) && (regi <= rBX))
            i++;

        while (picode != m_end && picode->ic.ll.match(iPOP))
        {
            if (picode->ic.ll.dst.regi != regi)
                break;
            i++;
            m_icodes.push_back(picode++);
        }
        m_param_count = i*2;
    }
    return m_param_count!=0;
}
int Idiom17::action()
{
    if (m_icodes[0]->isLlFlag(I))
    {
        m_icodes[0]->ic.ll.src.proc.proc->cbParam = (int16_t)m_param_count;
        m_icodes[0]->ic.ll.src.proc.cb = m_param_count;
        m_icodes[0]->ic.ll.src.proc.proc->flg |= CALL_C;
        for(int idx=1; idx<m_icodes.size(); ++idx)
        {
            m_icodes[idx]->invalidate();
        }
    }
    // TODO : it's a calculated call
    else
    {
        printf("Indirect call at idiom17\n");
    }
    return m_icodes.size();
}
