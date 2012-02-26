/*****************************************************************************
 *          dcc project machine idiom recognition
 * (C) Cristina Cifuentes
 ****************************************************************************/

#include <cstring>
#include <deque>
#include "idiom.h"
#include "idiom1.h"
#include "epilogue_idioms.h"
#include "dcc.h"

/*****************************************************************************
 * JmpInst - Returns TRUE if opcode is a conditional or unconditional jump
 ****************************************************************************/
bool JmpInst(llIcode opcode)
{
    switch (opcode) {
    case iJMP:  case iJMPF: case iJCXZ:
    case iLOOP: case iLOOPE:case iLOOPNE:
    case iJB:   case iJBE:  case iJAE:  case iJA:
    case iJL:   case iJLE:  case iJGE:  case iJG:
    case iJE:   case iJNE:  case iJS:   case iJNS:
    case iJO:   case iJNO:  case iJP:   case iJNP:
        return true;
    }
    return false;
}

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
struct Idiom3 : public Idiom
{
protected:
    iICODE m_icodes[2];
    int m_param_count;
public:
    Idiom3(Function *f) : Idiom(f)
    {
    }
    uint8_t minimum_match_length() {return 2;}
    bool match(iICODE picode)
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
    int action()
    {
        if (m_icodes[0]->ic.ll.flg & I)
        {
            m_icodes[0]->ic.ll.src.proc.proc->cbParam = (int16)m_param_count;
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
};


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
struct Idiom17 : public Idiom
{
protected:
    std::vector<iICODE> m_icodes;
    int m_param_count;
public:
    Idiom17(Function *f) : Idiom(f)
    {
    }
    uint8_t minimum_match_length() {return 2;}
    bool match(iICODE picode)
    {
        if(distance(picode,m_end)<2)
            return false;
        m_param_count=0; /* Count on # pops */
        m_icodes.clear();

        /* Match ADD  SP, immed */
        for(int i=0; i<2; ++i)
            m_icodes.push_back(picode++);
        byte regi;

        /* Match POP reg */
        if (m_icodes[1]->ic.ll.match(iPOP))
        {
            int i=0;
            regi = m_icodes[1]->ic.ll.dst.regi;
            if ((regi >= rAX) && (regi <= rBX))
                i++;

            m_icodes.push_back(++picode);
            while (m_icodes.back() != m_end && m_icodes.back()->ic.ll.match(iPOP))
            {
                if (m_icodes.back()->ic.ll.dst.regi != regi)
                    break;
                i++;
                m_icodes.push_back(++picode);
            }
            m_param_count = i*2;
        }
        return m_param_count!=0;
    }
    int action()
    {
        if (m_icodes[0]->isLlFlag(I))
        {
            m_icodes[0]->ic.ll.src.proc.proc->cbParam = (int16)m_param_count;
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
};

/*****************************************************************************
 * idiom5 - Long addition.
 *      ADD reg/stackOff, reg/stackOff
 *      ADC reg/stackOff, reg/stackOff
 *      Eg:     ADD ax, [bp-4]
 *              ADC dx, [bp-2]
 *          =>  dx:ax = dx:ax + [bp-2]:[bp-4]
 *      Found in Borland Turbo C code.
 *      Commonly used idiom for long addition.
 ****************************************************************************/
static boolT idiom5 (iICODE pIcode, iICODE pEnd)
{
    iICODE next1=pIcode+1;
    if (pIcode >= pEnd)
        return false;
    if (next1->ic.ll.match(iADC))
        return true;
    return false;
}


/*****************************************************************************
 * idiom6 - Long substraction.
 *      SUB reg/stackOff, reg/stackOff
 *      SBB reg/stackOff, reg/stackOff
 *      Eg:     SUB ax, [bp-4]
 *              SBB dx, [bp-2]
 *          =>  dx:ax = dx:ax - [bp-2]:[bp-4]
 *      Found in Borland Turbo C code.
 *      Commonly used idiom for long substraction.
 ****************************************************************************/
static boolT idiom6 (iICODE pIcode, iICODE pEnd)
{
    if (pIcode < pEnd)
        if ((pIcode+1)->ic.ll.match(iSBB))
            return true;
    return false;
}


/*****************************************************************************
 * idiom7 - Assign zero
 *      XOR reg/stackOff, reg/stackOff
 *      Eg:     XOR ax, ax
 *          =>  ax = 0
 *      Found in Borland Turbo C and Microsoft C code.
 ****************************************************************************/
static boolT idiom7 (iICODE pIcode)
{
    LLOperand *dst, *src;

    dst = &pIcode->ic.ll.dst;
    src = &pIcode->ic.ll.src;
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
    else if ((dst->off) && (dst->seg == rSS) && (dst->regi == INDEXBASE + 6))
        /* offset from BP */
    {
        if ((dst->off == src->off) && (dst->seg == src->seg) &&
                (dst->regi == src->regi))
            return true;
    }
    return false;
}


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
static boolT idiom21 (iICODE picode, iICODE pend)
{
    LLOperand *dst, *src;
    iICODE next1=picode+1;
    if(next1>=pend)
        return false;
    dst = &picode->ic.ll.dst;
    src = &picode->ic.ll.src;
    if (next1->ic.ll.flg & I)
    {
        if ((dst->regi == src->regi) && (dst->regi > 0) && (dst->regi < INDEXBASE))
        {
            if ((dst->regi == rDX) && (next1->ic.ll.dst.regi == rAX))
                return true;
            if ((dst->regi == rCX) && (next1->ic.ll.dst.regi == rBX))
                return true;
        }
    }
    return false;
}


/*****************************************************************************
 * idiom8 - Shift right by 1 (signed long ops)
 *      SAR reg, 1
 *      RCR reg, 1
 *      Eg:     SAR dx, 1
 *              RCR ax, 1
 *          =>  dx:ax = dx:ax >> 1      (dx:ax are signed long)
 *      Found in Microsoft C code for long signed variable shift right.
 ****************************************************************************/
static boolT idiom8 (iICODE pIcode, iICODE pEnd)
{
    iICODE next=pIcode+1;
    if((pIcode>=pEnd) and (next>=pEnd))
        return false;
    if (pIcode->ic.ll.anyFlagSet(I) && (pIcode->ic.ll.src.op() == 1))
        if ((next->ic.ll.opcode == iRCR) &&
                next->ic.ll.anyFlagSet(I) &&
                (next->ic.ll.src.op() == 1))
            return true;
    return false;
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
static Int idiom15 (iICODE picode, iICODE pend)
{
    Int n = 1;
    byte regi;

    if (picode < pend)
    {
        /* Match SHL reg, 1 */
        if ((picode->ic.ll.flg & I) && (picode->ic.ll.src.op() == 1))
        {
            regi = picode->ic.ll.dst.regi;
            for(auto iter=picode+1;iter < pend;++iter)
            {
                if ((iter->ic.ll.opcode == iSHL) &&
                        (iter->ic.ll.flg & I) &&
                        (iter->ic.ll.src.op() == 1) &&
                        (iter->ic.ll.dst.regi == regi))
                    n++;
                else
                    break;
            }
        }
    }
    if (n > 1)
        return (n);
    else
        return (0);
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
static boolT idiom12 (iICODE pIcode, iICODE pEnd)
{
    iICODE next=pIcode+1;
    if (pIcode < pEnd)
    {
        if (pIcode->ic.ll.anyFlagSet(I) && (pIcode->ic.ll.src.op() == 1))
            if ((next->ic.ll.opcode == iRCL) &&
                    (next->ic.ll.anyFlagSet(I)) &&
                    (next->ic.ll.src.op() == 1))
                return true;
    }
    return false;
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
static boolT idiom9 (iICODE pIcode, iICODE pEnd)
{
    iICODE next=pIcode+1;
    if (pIcode < pEnd)
    {
        if (((pIcode->ic.ll.flg & I) == I) && (pIcode->ic.ll.src.op() == 1))
            if ((next->ic.ll.opcode == iRCR) &&
                    ((next->ic.ll.flg & I) == I) &&
                    (next->ic.ll.src.op() == 1))
                return true;
    }
    return false;
}


/*****************************************************************************
 * idiom10 - Jump if not equal to 0
 *      OR  reg, reg
 *      JNE labX
 *      Eg:     OR  ax, ax
 *              JNE labX
 *      => HLI_JCOND (ax != 0) labX
 *		Note: we also check that these instructions are not followed by
 *			  CMP reg, kte
 *			  JE  lab
 *			  because this is most likely a long conditional equality test.
 *      Found in Borland Turbo C.
 ****************************************************************************/
static boolT idiom10old (iICODE pIcode, iICODE pEnd)
{
    if (pIcode < pEnd)
    {
        /* Check OR reg, reg */
        if (((pIcode->ic.ll.flg & I) != I) &&
                (pIcode->ic.ll.src. regi > 0) &&
                (pIcode->ic.ll.src.regi < INDEXBASE) &&
                (pIcode->ic.ll.src.regi == pIcode->ic.ll.dst.regi))
            if ((pIcode+3) < pEnd)
            {
                if (((pIcode+1)->ic.ll.opcode == iJNE) &&
                        ((pIcode+2)->ic.ll.opcode != iCMP) &&
                        ((pIcode+3)->ic.ll.opcode != iJE))
                    return true;
            }
            else	/* at the end of the procedure */
                if (((pIcode+1) < pEnd) && ((pIcode+1)->ic.ll.opcode == iJNE))
                    return true;
    }
    return false;
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
static void idiom10 (iICODE pIcode, iICODE pEnd)
{
    iICODE next=pIcode+1;
    if((pIcode>=pEnd) or (next>=pEnd))
        return;
    /* Check OR reg, reg */
    if (((pIcode->ic.ll.flg & I) != I) &&
            (pIcode->ic.ll.src.regi > 0) &&
            (pIcode->ic.ll.src.regi < INDEXBASE) &&
            (pIcode->ic.ll.src.regi == pIcode->ic.ll.dst.regi))
        if (next->ic.ll.opcode == iJNE)
        {
            pIcode->ic.ll.opcode = iCMP;
            pIcode->ic.ll.flg |= I;
            pIcode->ic.ll.src.SetImmediateOp(0); // todo check if proc should be zeroed too
            pIcode->du.def = 0;
            pIcode->du1.numRegsDef = 0;
        }
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
static byte idiom13 (iICODE pIcode, iICODE pEnd)
{
    byte regi;
    iICODE next=pIcode+1;
    if((pIcode>=pEnd) or (next>=pEnd))
        return 0;

    /* Check for regL */
    regi = pIcode->ic.ll.dst.regi;
    if (((pIcode->ic.ll.flg & I) != I) && (regi >= rAL) && (regi <= rBH))
    {
        /* Check for MOV regH, 0 */
        if ((next->ic.ll.opcode == iMOV) &&
                (next->ic.ll.flg & I) &&
                (next->ic.ll.src.op() == 0))
        {
            if (next->ic.ll.dst.regi == (regi + 4))
                return (regi - rAL + rAX);
        }
    }
    return (0);
}


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
static boolT idiom14 (iICODE picode, iICODE pend, byte *regL, byte *regH)
{
    if (picode >= pend)
        return false;
    iICODE next1=picode+1;
    /* Check for regL */
    *regL = picode->ic.ll.dst.regi;
    if (((picode->ic.ll.flg & I) != I) && ((*regL == rAX) || (*regL ==rBX)))
    {
        /* Check for XOR regH, regH */
        if ((next1->ic.ll.opcode == iXOR) && ((next1->ic.ll.flg & I) != I))
        {
            *regH = next1->ic.ll.dst.regi;
            if (*regH == next1->ic.ll.src.regi)
            {
                if ((*regL == rAX) && (*regH == rDX))
                    return true;
                if ((*regL == rBX) && (*regH == rCX))
                    return true;
            }
        }
    }
    return false;
}

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
static boolT idiom11 (iICODE pIcode, iICODE pEnd)
{
    condId type;          /* type of argument */
    const char *matchstring="(oNEG rH) (oNEG rL) (SBB \rH i0)";
    iICODE next2 = pIcode+2;
    if (next2 < pEnd)
    {
        type = pIcode->idType(DST);
        if ((type != CONSTANT) && (type != OTHER))
        {
            /* Check NEG reg/mem
             *       SBB reg/mem, 0*/
            if (((pIcode+1)->ic.ll.match(iNEG)) && next2->ic.ll.match(iSBB))
                switch (type)
                {
                case GLOB_VAR:
                    if ((next2->ic.ll.dst.segValue == pIcode->ic.ll.dst.segValue) &&
                            (next2->ic.ll.dst.off == pIcode->ic.ll.dst.off))
                        return true;
                    break;
                case REGISTER:
                    if (next2->ic.ll.dst.regi == pIcode->ic.ll.dst.regi)
                        return true;
                    break;
                case PARAM:
                case LOCAL_VAR:
                    if (next2->ic.ll.dst.off == pIcode->ic.ll.dst.off)
                        return true;
                    break;
                }
        }
    }
    return false;
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
static boolT idiom16 (iICODE picode, iICODE pend)
{
    const char *matchstring="(oNEG rR) (oSBB rR rR) (oINC rR)";
    byte regi;
    iICODE next1(picode+1),next2(picode+2);
    if (next2 >= pend)
        return false;
    regi = picode->ic.ll.dst.regi;
    if ((regi >= rAX) && (regi < INDEXBASE))
    {
        if (next1->ic.ll.match(iSBB) && (next2->ic.ll.opcode == iINC))
            if ((next1->ic.ll.dst.regi == (next1->ic.ll.src.regi)) &&
                    (next1->ic.ll.dst.regi == regi) &&
                    (next2->ic.ll.dst.regi == regi))
                return true;
    }
    return false;
}


/*****************************************************************************
 * idiom 18: Post-increment or post-decrement in a conditional jump
 * Used
 *	0	MOV  reg, var (including register variables)
 *	1	INC var	or DEC var <------------------------- input point
 *	2	CMP  var, Y
 *	3	JX   label
 *		=> HLI_JCOND (var++ X Y)
 *		Eg:		MOV  ax, si
 *				INC  si
 *				CMP  ax, 8
 *				JL   labX
 *			=>  HLI_JCOND (si++ < 8)
 * 		Found in Borland Turbo C.  Intrinsic to C languages.
 ****************************************************************************/
struct Idiom18 : public Idiom
{
protected:
    iICODE m_icodes[4];
    bool m_is_dec;
public:
    Idiom18(Function *f) : Idiom(f)
    {
    }
    uint8_t minimum_match_length() {return 4;}
    bool match(iICODE picode)
    {
        if(std::distance(picode,m_end)<2)
            return false;
        --picode; //

        for(int i=0; i<4; ++i)
            m_icodes[i] =picode++;

        m_is_dec = m_icodes[1]->ic.ll.match(iDEC);
        int type = -1;	/* type of variable: 1 = reg-var, 2 = local */
        byte regi;		/* register of the MOV */

        /* Get variable */
        if (m_icodes[1]->ic.ll.dst.regi == 0)	/* global variable */
        {
            /* not supported yet */
            type = 0;
        }
        else if (m_icodes[1]->ic.ll.dst.regi < INDEXBASE)	/* register */
        {
            if ((m_icodes[1]->ic.ll.dst.regi == rSI) && (m_func->flg & SI_REGVAR))
                type = 1;
            else if ((m_icodes[1]->ic.ll.dst.regi == rDI) && (m_func->flg & DI_REGVAR))
                type = 1;
        }
        else if (m_icodes[1]->ic.ll.dst.off)		/* local variable */
            type = 2;
        else		/* indexed */
        {
            type=3;
            /* not supported yet */
            printf("Unsupported idiom18 type: indexed");
        }

        switch(type)
        {
        case 0: // global
            printf("Unsupported idiom18 type: global variable");
            break;
        case 1:  /* register variable */
            /* Check previous instruction for a MOV */
            if (m_icodes[0]->ic.ll.match(iMOV) && (m_icodes[0]->ic.ll.src.regi == m_icodes[1]->ic.ll.dst.regi))
            {
                regi = m_icodes[0]->ic.ll.dst.regi;
                if ((regi > 0) && (regi < INDEXBASE))
                {
                    if ( m_icodes[2]->ic.ll.match(iCMP) && (m_icodes[2]->ic.ll.dst.regi == regi) &&
                         m_icodes[3]->ic.ll.conditionalJump() )
                        return true;
                }
            }
            break;
        case 2: /* local */
            if (m_icodes[0]->ic.ll.match(iMOV) && (m_icodes[0]->ic.ll.src.off == m_icodes[1]->ic.ll.dst.off))
            {
                regi = m_icodes[0]->ic.ll.dst.regi;
                if ((regi > 0) && (regi < INDEXBASE))
                {
                    if ( m_icodes[2]->ic.ll.match(iCMP) && (m_icodes[2]->ic.ll.dst.regi == regi) &&
                         m_icodes[3]->ic.ll.conditionalJump() )
                        return true;
                }
            }
            break;
        case 3: // indexed
            printf("Unsupported idiom18 type: indexed");
            break;
        }
        return false;
    }

    int action() // action length
    {
        COND_EXPR *rhs, *lhs;   /* Pointers to left and right hand side exps */
        COND_EXPR *expr;
        lhs     = COND_EXPR::id (*m_icodes[0], SRC, m_func, m_icodes[1], *m_icodes[1], eUSE);
        lhs     = COND_EXPR::unary ( m_is_dec ? POST_DEC : POST_INC, lhs);
        rhs     = COND_EXPR::id (*m_icodes[2], SRC, m_func, m_icodes[1], *m_icodes[3], eUSE);
        expr    = COND_EXPR::boolOp (lhs, rhs, condOpJCond[m_icodes[3]->ic.ll.opcode - iJB]);
        m_icodes[3]->setJCond(expr);

        m_icodes[0]->invalidate();
        m_icodes[1]->invalidate();
        m_icodes[2]->invalidate();
        return 3;
    }
};
/*****************************************************************************
 * idiom 19: pre-increment or pre-decrement in conditional jump, comparing against 0.
 *		[INC | DEC] var (including register vars)
 *		JX	lab	JX  lab
 *		=>  HLI_JCOND (++var X 0) or HLI_JCOND (--var X 0)
 *		Eg: INC [bp+4]
 *                  JG  lab2
 *			=> HLI_JCOND (++[bp+4] > 0)
 *		Found in Borland Turbo C.  Intrinsic to C language.
 ****************************************************************************/
struct Idiom19 : public Idiom
{
protected:
    iICODE m_icodes[2];
    bool m_is_dec;
public:
    Idiom19(Function *f) : Idiom(f)
    {
    }
    uint8_t minimum_match_length() {return 2;}
    bool match(iICODE picode)
    {
        if(std::distance(picode,m_end)<1)
            return false;

        for(int i=0; i<2; ++i)
            m_icodes[i] =picode++;
        m_is_dec = m_icodes[0]->ic.ll.match(iDEC);
        if (m_icodes[0]->ic.ll.dst.regi == 0)	/* global variable */
            /* not supported yet */ ;
        else if (m_icodes[0]->ic.ll.dst.regi < INDEXBASE) /* register */
        {
            //        if (((picode->ic.ll.dst.regi == rSI) && (pproc->flg & SI_REGVAR)) ||
            //            ((picode->ic.ll.dst.regi == rDI) && (pproc->flg & DI_REGVAR)))
            if (m_icodes[1]->ic.ll.conditionalJump())
                return true;
        }
        else if (m_icodes[0]->ic.ll.dst.off)		/* stack variable */
        {
            if ( m_icodes[1]->ic.ll.conditionalJump() )
                return true;
        }
        else	/* indexed */
            /* not supported yet */ ;
        return false;
    }
    int action()
    {
        COND_EXPR *lhs,*rhs,*expr;
        lhs = COND_EXPR::id (*m_icodes[1], DST, m_func, m_icodes[0], *m_icodes[1], eUSE);
        lhs = COND_EXPR::unary (m_is_dec ? PRE_DEC : PRE_INC, lhs);
        rhs = COND_EXPR::idKte (0, 2);
        expr = COND_EXPR::boolOp (lhs, rhs, condOpJCond[m_icodes[1]->ic.ll.opcode - iJB]);
        m_icodes[1]->setJCond(expr);
        m_icodes[0]->invalidate();
        return 2;
    }
};


/*****************************************************************************
 * idiom20: Pre increment/decrement in conditional expression (compares
 *			against a register, variable or constant different than 0).
 *		INC var			or DEC var (including register vars)
 *		MOV reg, var	   MOV reg, var
 *		CMP reg, Y		   CMP reg, Y
 *		JX  lab			   JX  lab
 *		=> HLI_JCOND (++var X Y) or HLI_JCOND (--var X Y)
 *		Eg: INC si	(si is a register variable)
 *			MOV ax, si
 *			CMP ax, 2
 *			JL	lab4
 *			=> HLI_JCOND (++si < 2)
 *		Found in Turbo C.  Intrinsic to C language.
 ****************************************************************************/
struct Idiom20 : public Idiom
{
protected:
    iICODE m_icodes[4];
    bool m_is_dec;
public:
    Idiom20(Function *f) : Idiom(f)
    {
    }
    uint8_t minimum_match_length() {return 4;}
    bool match(iICODE picode)
    {
        uint8_t type = 0;	/* type of variable: 1 = reg-var, 2 = local */
        byte regi;		/* register of the MOV */
        if(std::distance(picode,m_end)<3)
            return false;
        for(int i=0; i<4; ++i)
            m_icodes[i] =picode++;

        m_is_dec = m_icodes[0]->ic.ll.match(iDEC);

        /* Get variable */
        if (m_icodes[0]->ic.ll.dst.regi == 0)	/* global variable */
        {
            /* not supported yet */ ;
        }
        else if (m_icodes[0]->ic.ll.dst.regi < INDEXBASE)	/* register */
        {
            if ((m_icodes[0]->ic.ll.dst.regi == rSI) && (m_func->flg & SI_REGVAR))
                type = 1;
            else if ((m_icodes[0]->ic.ll.dst.regi == rDI) && (m_func->flg & DI_REGVAR))
                type = 1;
        }
        else if (m_icodes[0]->ic.ll.dst.off)		/* local variable */
            type = 2;
        else		/* indexed */
        {
            printf("idiom20 : Unsupported type\n");
            /* not supported yet */ ;
        }

        /* Check previous instruction for a MOV */
        if (type == 1)			/* register variable */
        {
            if (m_icodes[1]->ic.ll.match(iMOV) &&
                    (m_icodes[1]->ic.ll.src.regi == m_icodes[0]->ic.ll.dst.regi))
            {
                regi = m_icodes[1]->ic.ll.dst.regi;
                if ((regi > 0) && (regi < INDEXBASE))
                {
                    if (m_icodes[2]->ic.ll.match(iCMP,(eReg)regi) &&
                            m_icodes[3]->ic.ll.conditionalJump())
                        return true;
                }
            }
        }
        else if (type == 2)		/* local */
        {
            if ( m_icodes[0]->ic.ll.match(iMOV) &&
                 (m_icodes[1]->ic.ll.src.off == m_icodes[0]->ic.ll.dst.off))
            {
                regi = m_icodes[1]->ic.ll.dst.regi;
                if ((regi > 0) && (regi < INDEXBASE))
                {
                    if (m_icodes[2]->ic.ll.match(iCMP,(eReg)regi) &&
                            m_icodes[3]->ic.ll.conditionalJump())
                        return true;
                }
            }
        }
        return false;
    }
    int action()
    {
        COND_EXPR *lhs,*rhs,*expr;
        lhs  = COND_EXPR::id (*m_icodes[1], SRC, m_func, m_icodes[0], *m_icodes[0], eUSE);
        lhs  = COND_EXPR::unary (m_is_dec ? PRE_DEC : PRE_INC, lhs);
        rhs  = COND_EXPR::id (*m_icodes[2], SRC, m_func, m_icodes[0], *m_icodes[3], eUSE);
        expr = COND_EXPR::boolOp (lhs, rhs, condOpJCond[m_icodes[3]->ic.ll.opcode - iJB]);
        m_icodes[3]->setJCond(expr);
        m_icodes[0]->invalidate();
        m_icodes[1]->invalidate();
        m_icodes[2]->invalidate();
        return 3;
    }
};

/*****************************************************************************
 * findIdioms  - translates LOW_LEVEL icode idioms into HIGH_LEVEL icodes.
 ****************************************************************************/
void Function::findIdioms()
{
    //    Int     ip;             /* Index to current icode                   */
    iICODE  pEnd, pIcode;   /* Pointers to end of BB and current icodes */
    int16   delta;
    COND_EXPR *rhs, *lhs;   /* Pointers to left and right hand side exps */
    COND_EXPR *exp;         /* Pointer to temporal expression           */
    Int     idx;            /* Index into local identifier table        */
    byte    regH, regL;     /* High and low registers for long word reg */

    pIcode = Icode.begin();
    pEnd = Icode.end();
    Idiom1  i01(this);
    Idiom2  i02(this);
    Idiom3  i03(this);
    Idiom4  i04(this);
    Idiom17 i17(this);
    Idiom18 i18(this);
    Idiom19 i19(this);
    Idiom20 i20(this);

    while (pIcode < pEnd)
    {
        switch (pIcode->ic.ll.opcode)
        {
        case iDEC: case iINC:
            if (i18.match(pIcode))
                pIcode += i18.action();
            else if (i20.match(pIcode))
                pIcode += i20.action();
            else if (i19.match(pIcode))
                pIcode += i19.action();
            else
                pIcode++;
            break;

        case iPUSH:             /* Idiom 1 */
            if(flg & PROC_HLL) // todo add other push idioms.
            {
                ++pIcode;
                break;
            }
            if (i01.match(pIcode))
                pIcode += i01.action();
            else
                pIcode++;
            break;

        case iMOV:              /* Idiom 2 */
            if (i02.match(pIcode))
            {
                pIcode += i02.action();
            }
            else if (idiom14 (pIcode, pEnd, &regL, &regH))  /* Idiom 14 */
            {
                idx = localId.newLongReg (TYPE_LONG_SIGN, regH, regL, pIcode/*ip*/);
                lhs = COND_EXPR::idLongIdx (idx);
                pIcode->setRegDU( regH, eDEF);
                rhs = COND_EXPR::id (*pIcode, SRC, this, pIcode, *pIcode, NONE);
                pIcode->setAsgn(lhs, rhs);
                (pIcode+1)->invalidate();
                pIcode += 2;
            }
            else if (idx = idiom13 (pIcode, pEnd))      /* Idiom 13 */
            {
                lhs = COND_EXPR::idReg (idx, 0, &localId);
                pIcode->setRegDU( idx, eDEF);
                pIcode->du1.numRegsDef--;   	/* prev byte reg def */
                rhs = COND_EXPR::id (*pIcode, SRC, this, pIcode, *pIcode, NONE);
                pIcode->setAsgn(lhs, rhs);
                (pIcode+1)->invalidate();
                pIcode += 2;
            }
            else
                pIcode++;
            break;

        case iCALL:  case iCALLF:
            /* Check for library functions that return a long register.
                         * Propagate this result */
            if (pIcode->ic.ll.src.proc.proc != 0)
                if ((pIcode->ic.ll.src.proc.proc->flg & PROC_ISLIB) &&
                        (pIcode->ic.ll.src.proc.proc->flg & PROC_IS_FUNC))
                {
                    if ((pIcode->ic.ll.src.proc.proc->retVal.type==TYPE_LONG_SIGN)
                            || (pIcode->ic.ll.src.proc.proc->retVal.type == TYPE_LONG_UNSIGN))
                        localId.newLongReg(TYPE_LONG_SIGN, rDX, rAX, pIcode/*ip*/);
                }

            /* Check for idioms */
            if (i03.match(pIcode))         /* idiom 3 */
            {
                pIcode+=i03.action();
            }
            else if (i17.match(pIcode))  /* idiom 17 */
            {
                pIcode+=i17.action();
            }
            else
                pIcode++;
            break;

        case iRET:          /* Idiom 4 */
        case iRETF:
            if(i04.match(pIcode))
                i04.action();
            pIcode++;
            break;

        case iADD:          /* Idiom 5 */
            if (idiom5 (pIcode, pEnd))
            {

                lhs = COND_EXPR::idLong (&localId, DST, pIcode, LOW_FIRST,
                                         pIcode/*ip*/, USE_DEF, 1);
                rhs = COND_EXPR::idLong (&localId, SRC, pIcode, LOW_FIRST,
                                         pIcode/*ip*/, eUSE, 1);
                exp = COND_EXPR::boolOp (lhs, rhs, ADD);
                pIcode->setAsgn(lhs, exp);
                (pIcode+1)->invalidate();
                pIcode++;
            }
            pIcode++;
            break;

        case iSAR:          /* Idiom 8 */
            if (idiom8 (pIcode, pEnd))
            {
                idx = localId.newLongReg(TYPE_LONG_SIGN,
                                         pIcode->ic.ll.dst.regi, (pIcode+1)->ic.ll.dst.regi,pIcode/*ip*/);
                lhs = COND_EXPR::idLongIdx (idx);
                pIcode->setRegDU( (pIcode+1)->ic.ll.dst.regi, USE_DEF);
                rhs = COND_EXPR::idKte (1, 2);
                exp = COND_EXPR::boolOp (lhs, rhs, SHR);
                pIcode->setAsgn(lhs, exp);
                (pIcode+1)->invalidate();
                pIcode++;
            }
            pIcode++;
            break;

        case iSHL:
            if (idx = idiom15 (pIcode, pEnd))       /* idiom 15 */
            {
                lhs = COND_EXPR::idReg (pIcode->ic.ll.dst.regi,
                                        pIcode->ic.ll.flg & NO_SRC_B,
                                        &localId);
                rhs = COND_EXPR::idKte (idx, 2);
                exp = COND_EXPR::boolOp (lhs, rhs, SHL);
                pIcode->setAsgn(lhs, exp);
                pIcode++;
                for (idx-- ; idx > 0; idx--)
                {
                    (pIcode++)->invalidate();
                }
            }
            else if (idiom12 (pIcode, pEnd))        /* idiom 12 */
            {
                idx = localId.newLongReg (TYPE_LONG_UNSIGN,
                                          (pIcode+1)->ic.ll.dst.regi, pIcode->ic.ll.dst.regi,pIcode/*ip*/);
                lhs = COND_EXPR::idLongIdx (idx);
                pIcode->setRegDU( (pIcode+1)->ic.ll.dst.regi, USE_DEF);
                rhs = COND_EXPR::idKte (1, 2);
                exp = COND_EXPR::boolOp (lhs, rhs, SHL);
                pIcode->setAsgn(lhs, exp);
                (pIcode+1)->invalidate();
                pIcode += 2;
            }
            else
                pIcode++;
            break;

        case iSHR:          /* Idiom 9 */
            if (idiom9 (pIcode, pEnd))
            {
                idx = localId.newLongReg (TYPE_LONG_UNSIGN,
                                          pIcode->ic.ll.dst.regi, (pIcode+1)->ic.ll.dst.regi,pIcode/*ip*/);
                lhs = COND_EXPR::idLongIdx (idx);
                pIcode->setRegDU( (pIcode+1)->ic.ll.dst.regi, USE_DEF);
                rhs = COND_EXPR::idKte (1, 2);
                exp = COND_EXPR::boolOp (lhs, rhs, SHR);
                pIcode->setAsgn(lhs, exp);
                (pIcode+1)->invalidate();
                pIcode++;
            }
            pIcode++;
            break;

        case iSUB:          /* Idiom 6 */
            if (idiom6 (pIcode, pEnd))
            {
                lhs = COND_EXPR::idLong (&localId, DST, pIcode, LOW_FIRST, pIcode, USE_DEF, 1);
                rhs = COND_EXPR::idLong (&localId, SRC, pIcode, LOW_FIRST, pIcode, eUSE, 1);
                exp = COND_EXPR::boolOp (lhs, rhs, SUB);
                pIcode->setAsgn(lhs, exp);
                (pIcode+1)->invalidate();
                pIcode++;
            }
            pIcode++;
            break;

        case iOR:           /* Idiom 10 */
            idiom10 (pIcode, pEnd);
            pIcode++;
            break;

        case iNEG:          /* Idiom 11 */
            if (idiom11 (pIcode, pEnd))
            {
                lhs = COND_EXPR::idLong (&localId, DST, pIcode, HIGH_FIRST,
                                         pIcode/*ip*/, USE_DEF, 1);
                rhs = COND_EXPR::unary (NEGATION, lhs);
                pIcode->setAsgn(lhs, rhs);
                (pIcode+1)->invalidate();
                (pIcode+2)->invalidate();
                pIcode += 3;
            }
            else if (idiom16 (pIcode, pEnd))
            {
                lhs = COND_EXPR::idReg (pIcode->ic.ll.dst.regi, pIcode->ic.ll.flg,
                                        &localId);
                rhs = lhs->clone();
                rhs = COND_EXPR::unary (NEGATION, lhs);
                pIcode->setAsgn(lhs, rhs);
                (pIcode+1)->invalidate();
                (pIcode+2)->invalidate();
                pIcode += 3;
            }
            else
                pIcode++;
            break;

        case iNOP:
            (pIcode++)->invalidate();
            break;

        case iENTER:		/* ENTER is equivalent to init PUSH bp */
            if (pIcode == Icode.begin()) //ip == 0
            {
                flg |= (PROC_HLL | PROC_IS_HLL);
            }
            pIcode++;
            break;

        case iXOR:          /* Idiom 7 */
            if (idiom21 (pIcode, pEnd))
            {
                lhs = COND_EXPR::idLong (&localId, DST, pIcode,HIGH_FIRST, pIcode/*ip*/, eDEF, 1);
                rhs = COND_EXPR::idKte ((pIcode+1)->ic.ll.src.op() , 4);
                pIcode->setAsgn(lhs, rhs);
                pIcode->du.use = 0;		/* clear register used in iXOR */
                (pIcode+1)->invalidate();
                pIcode++;
            }
            else if (idiom7 (pIcode))
            {
                lhs = COND_EXPR::id (*pIcode, DST, this, pIcode, *pIcode, NONE);
                rhs = COND_EXPR::idKte (0, 2);
                pIcode->setAsgn(lhs, rhs);
                pIcode->du.use = 0;    /* clear register used in iXOR */
                pIcode->ic.ll.flg |= I;
            }
            pIcode++;
            break;

        default:
            pIcode++;
        }
    }

    /* Check if number of parameter bytes match their calling convention */
    if ((flg & PROC_HLL) && (!args.sym.empty()))
    {
        args.m_minOff += (flg & PROC_FAR ? 4 : 2);
        delta = args.maxOff - args.m_minOff;
        if (cbParam != delta)
        {
            cbParam = delta;
            flg |= (CALL_MASK & CALL_UNKNOWN);
        }
    }
}


/* Sets up the TARGET flag for jump target addresses, and
 * binds jump target addresses to icode offsets.    */
void Function::bindIcodeOff()
{
    Int i;                 /* idx into icode array */
    iICODE pIcode;            /* ptr icode array      */
    dword *p;                 /* for case table       */

    if (Icode.empty())        /* No Icode */
        return;
    pIcode = Icode.begin();

    /* Flag all jump targets for BB construction and disassembly stage 2 */
    for (i = 0; i < Icode.size(); i++)
        if ((pIcode[i].ic.ll.flg & I) && JmpInst(pIcode[i].ic.ll.opcode))
        {
            iICODE loc=Icode.labelSrch(pIcode[i].ic.ll.src.op());
            if (loc!=Icode.end())
                loc->ic.ll.flg |= TARGET;
        }

    /* Finally bind jump targets to Icode offsets.  Jumps for which no label
     * is found (no code at dest. of jump) are simply left unlinked and
     * flagged as going nowhere.  */
    //for (pIcode = Icode.begin(); pIcode!= Icode.end(); pIcode++)
    for(ICODE &icode : Icode)
    {
        if (not JmpInst(icode.ic.ll.opcode))
            continue;
        if (icode.ic.ll.flg & I)
        {
            dword found;
            if (! Icode.labelSrch(icode.ic.ll.src.op(), found))
            {
                icode.ic.ll.flg |= NO_LABEL;
            }
            else
                icode.ic.ll.src.SetImmediateOp(found);

        }
        else if (icode.ic.ll.flg & SWITCH)
        {
            p = icode.ic.ll.caseTbl.entries;
            for (int j = 0; j < icode.ic.ll.caseTbl.numEntries; j++, p++)
                Icode.labelSrch(*p, *p);
        }
    }
}

/* Performs idioms analysis, and propagates long operands, if any */
void Function::lowLevelAnalysis ()
{
    /* Idiom analysis - sets up some flags and creates some HIGH_LEVEL icodes */
    findIdioms();
    /* Propagate HIGH_LEVEL idiom information for long operands */
    propLong();
}
