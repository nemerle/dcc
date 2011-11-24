/*****************************************************************************
 *          dcc project machine idiom recognition
 * (C) Cristina Cifuentes
 ****************************************************************************/

#include "dcc.h"
#include <string.h>
#ifdef __DOSWIN__
#include <stdio.h>
#endif


/*****************************************************************************
 * JmpInst - Returns TRUE if opcode is a conditional or unconditional jump
 ****************************************************************************/
boolT JmpInst(llIcode opcode)
{
    switch (opcode) {
        case iJMP:  case iJMPF: case iJCXZ:
        case iLOOP: case iLOOPE:case iLOOPNE:
        case iJB:   case iJBE:  case iJAE:  case iJA:
        case iJL:   case iJLE:  case iJGE:  case iJG:
        case iJE:   case iJNE:  case iJS:   case iJNS:
        case iJO:   case iJNO:  case iJP:   case iJNP:
            return TRUE;
    }
    return FALSE;
}


/*****************************************************************************
/* checkStkVars - Checks for PUSH SI
 *                          [PUSH DI]
 *                or         PUSH DI
 *                          [PUSH SI]
 * In which case, the stack variable flags are set
 ****************************************************************************/
static Int checkStkVars (ICODE * pIcode, ICODE * pEnd, Function * pProc)
{
    /* Look for PUSH SI */
    if ((pIcode < pEnd) && (pIcode->ic.ll.opcode == iPUSH) &&
        (pIcode->ic.ll.dst.regi == rSI))
    {
        pProc->flg |= SI_REGVAR;

        /* Look for PUSH DI */
        if (++pIcode < pEnd && (pIcode->ic.ll.opcode == iPUSH) &&
            (pIcode->ic.ll.dst.regi == rDI))
        {
            pProc->flg |= DI_REGVAR;
            return 2;
        }
        else
            return 1;
    }
    else if ((pIcode < pEnd) && (pIcode->ic.ll.opcode == iPUSH) &&
             (pIcode->ic.ll.dst.regi == rDI))
    {
        pProc->flg |= DI_REGVAR;

        /* Look for PUSH SI */
        if ((++pIcode < pEnd) && (pIcode->ic.ll.opcode == iPUSH) &&
            (pIcode->ic.ll.dst.regi == rSI))
        {
            pProc->flg |= SI_REGVAR;
            return 2;
        }
        else
            return 1;
    }
    return 0;
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
static Int idiom1(ICODE * pIcode, ICODE * pEnd, Function * pProc)
{ Int n;

    /* PUSH BP as first instruction of procedure */
    if ( !(pIcode->ic.ll.flg & I) && pIcode->ic.ll.src.regi == rBP)
    {
        /* MOV BP, SP as next instruction */
        if (++pIcode < pEnd && ! (pIcode->ic.ll.flg & (I | TARGET | CASE))
            && pIcode->ic.ll.opcode == iMOV && pIcode->ic.ll.dst.regi == rBP
            && pIcode->ic.ll.src.regi == rSP)
        {
            pProc->args.minOff = 2;
            pProc->flg |= PROC_IS_HLL;

            /* Look for SUB SP, immed */
            if ((++pIcode < pEnd) &&
                (pIcode->ic.ll.flg & (I | TARGET | CASE)) == I &&
                pIcode->ic.ll.opcode == iSUB && pIcode->ic.ll.dst.regi == rSP)
            {
                return (3 + checkStkVars (++pIcode, pEnd, pProc));
            }
            else
                return (2 + checkStkVars (pIcode, pEnd, pProc));
        }

        /* PUSH SI
         * [PUSH DI]
         * MOV BP, SP */
        else
        {
            n = checkStkVars (pIcode, pEnd, pProc);
            if (n > 0)
            {
                /* Look for MOV BP, SP */
                pIcode += n;
                if (pIcode < pEnd &&
                    ! (pIcode->ic.ll.flg & (I | TARGET | CASE)) &&
                    pIcode->ic.ll.opcode == iMOV &&
                    pIcode->ic.ll.dst.regi == rBP &&
                    pIcode->ic.ll.src.regi == rSP)
                {
                    pProc->args.minOff = 2 + (n * 2);
                    return (2 + n);
                }
                else return 0;		// Cristina: check this please!
            }
            else return 0;		// Cristina: check this please!
        }
    }
    else
        return (checkStkVars (pIcode, pEnd, pProc));
}


/*****************************************************************************
 * popStkVars - checks for
 *          [POP DI]
 *          [POP SI]
 *      or  [POP SI]
 *          [POP DI]
 ****************************************************************************/
static void popStkVars (ICODE * pIcode, ICODE * pEnd, Function * pProc)
{
    /* Match [POP DI] */
    if (pIcode->ic.ll.opcode == iPOP)
        if ((pProc->flg & DI_REGVAR) && (pIcode->ic.ll.dst.regi == rDI))
            pIcode->invalidate();
        else if ((pProc->flg & SI_REGVAR) && (pIcode->ic.ll.dst.regi == rSI))
            pIcode->invalidate();

    /* Match [POP SI] */
    if ((pIcode+1)->ic.ll.opcode == iPOP)
        if ((pProc->flg & SI_REGVAR) && ((pIcode+1)->ic.ll.dst.regi == rSI))
            (pIcode+1)->invalidate();
        else if ((pProc->flg & DI_REGVAR) && ((pIcode+1)->ic.ll.dst.regi == rDI))
            (pIcode+1)->invalidate();
}


/*****************************************************************************
 * idiom2 - HLL procedure epilogue;  Returns number of instructions matched.
 *          [POP DI]
 *          [POP SI]
 *          MOV  SP, BP
 *          POP  BP
 *          RET(F)
 *****************************************************************************/
static Int idiom2(ICODE * pIcode, ICODE * pEnd, Int ip, Function * pProc)
{ ICODE * nicode;

    /* Match MOV SP, BP */
    if (ip != 0 && ((pIcode->ic.ll.flg & I) != I) &&
        pIcode->ic.ll.dst.regi == rSP && pIcode->ic.ll.src.regi == rBP)
    {
        /* Get next icode, skip over holes in the icode array */
        nicode = pIcode + 1;
        while (nicode->ic.ll.flg & NO_CODE)
            nicode++;

        /* Match POP BP */
        if (nicode < pEnd &&
            ! (nicode->ic.ll.flg & (I | TARGET | CASE)) &&
            nicode->ic.ll.opcode == iPOP &&
            nicode->ic.ll.dst.regi == rBP)
        {
            nicode++;

            /* Match RET(F) */
            if (nicode < pEnd &&
                ! (nicode->ic.ll.flg & (I | TARGET | CASE)) &&
                (nicode->ic.ll.opcode == iRET ||
                 nicode->ic.ll.opcode == iRETF))
            {
                popStkVars (pIcode-2, pEnd, pProc);
                return 2;
            }
        }
    }
    return 0;
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
static Int idiom3(ICODE * pIcode, ICODE * pEnd)
{
    /* Match ADD  SP, immed */
    if ((++pIcode < pEnd) && (pIcode->ic.ll.flg & I) &&
        (pIcode->ic.ll.opcode == iADD) && (pIcode->ic.ll.dst.regi == rSP))
        return (pIcode->ic.ll.immed.op);
    else if ((pIcode->ic.ll.opcode == iMOV) && (pIcode->ic.ll.dst.regi == rSP)
             && (pIcode->ic.ll.src.regi == rBP))
        (pIcode-1)->ic.ll.flg |= REST_STK;
    return 0;
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
static Int idiom17 (ICODE * pIcode, ICODE * pEnd)
{ Int i = 0;            /* Count on # pops */
    byte regi;

    /* Match POP reg */
    if ((++pIcode < pEnd) && (pIcode->ic.ll.opcode == iPOP))
    {
        regi = pIcode->ic.ll.dst.regi;
        if ((regi >= rAX) && (regi <= rBX))
            i++;
        while ((++pIcode)->ic.ll.opcode == iPOP)
        {
            if (pIcode->ic.ll.dst.regi == regi)
                i++;
            else
                break;
        }
        return (i * 2);
    }
    return (0);
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
static void idiom4 (ICODE * pIcode, ICODE * pEnd, Function * pProc)
{
    /* Check for [POP DI]
     *           [POP SI] */
    popStkVars (pIcode-3, pEnd, pProc);

    /* Check for POP BP */
    if (((pIcode-1)->ic.ll.opcode == iPOP) &&
        (((pIcode-1)->ic.ll.flg & I) != I) &&
        ((pIcode-1)->ic.ll.dst.regi == rBP))
        (pIcode-1)->invalidate();
    else
        popStkVars (pIcode-2, pEnd, pProc);

    /* Check for RET(F) immed */
    if (pIcode->ic.ll.flg & I)
    {
        pProc->cbParam = (int16)pIcode->ic.ll.immed.op;
        pProc->flg |= CALL_PASCAL;
    }
}


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
static boolT idiom5 (ICODE * pIcode, ICODE * pEnd)
{
    if (pIcode < pEnd)
        if ((pIcode+1)->ic.ll.opcode == iADC)
            return (TRUE);
    return (FALSE);
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
static boolT idiom6 (ICODE * pIcode, ICODE * pEnd)
{
    if (pIcode < pEnd)
        if ((pIcode+1)->ic.ll.opcode == iSBB)
            return (TRUE);
    return (FALSE);
}


/*****************************************************************************
 * idiom7 - Assign zero
 *      XOR reg/stackOff, reg/stackOff
 *      Eg:     XOR ax, ax
 *          =>  ax = 0
 *      Found in Borland Turbo C and Microsoft C code.
 ****************************************************************************/
static boolT idiom7 (ICODE * pIcode)
{
    ICODEMEM *dst, *src;

    dst = &pIcode->ic.ll.dst;
    src = &pIcode->ic.ll.src;
    if (dst->regi == 0)                 /* global variable */
    {
        if ((dst->segValue == src->segValue) && (dst->off == src->off))
            return (TRUE);
    }
    else if (dst->regi < INDEXBASE)     /* register */
    {
        if (dst->regi == src->regi)
            return (TRUE);
    }
    else if ((dst->off) && (dst->seg == rSS) && (dst->regi == INDEXBASE + 6))
        /* offset from BP */
    {
        if ((dst->off == src->off) && (dst->seg == src->seg) &&
            (dst->regi == src->regi))
            return (TRUE);
    }
    return (FALSE);
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
static boolT idiom21 (ICODE * picode, ICODE * pend)
{ ICODEMEM *dst, *src;

    dst = &picode->ic.ll.dst;
    src = &picode->ic.ll.src;
    if (((picode+1) < pend) && ((picode+1)->ic.ll.flg & I))
    {
        if ((dst->regi == src->regi) && (dst->regi > 0) &&
            (dst->regi < INDEXBASE))
        {
            if ((dst->regi == rDX) && ((picode+1)->ic.ll.dst.regi == rAX))
                return (TRUE);
            if ((dst->regi == rCX) && ((picode+1)->ic.ll.dst.regi == rBX))
                return (TRUE);
        }
    }
    return (FALSE);
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
static boolT idiom8 (ICODE * pIcode, ICODE * pEnd)
{
    if (pIcode < pEnd)
    {
        if (((pIcode->ic.ll.flg & I) == I) && (pIcode->ic.ll.immed.op == 1))
            if (((pIcode+1)->ic.ll.opcode == iRCR) &&
                (((pIcode+1)->ic.ll.flg & I) == I) &&
                ((pIcode+1)->ic.ll.immed.op == 1))
                return (TRUE);
    }
    return (FALSE);
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
static Int idiom15 (ICODE * picode, ICODE * pend)
{ Int n = 1;
    byte regi;

    if (picode < pend)
    {
        /* Match SHL reg, 1 */
        if ((picode->ic.ll.flg & I) && (picode->ic.ll.immed.op == 1))
        {
            regi = picode->ic.ll.dst.regi;
            while (1)
            {
                if (((picode+n) < pend) &&
                    ((picode+n)->ic.ll.opcode == iSHL) &&
                    ((picode+n)->ic.ll.flg & I) &&
                    ((picode+n)->ic.ll.immed.op == 1) &&
                    ((picode+n)->ic.ll.dst.regi == regi))
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
static boolT idiom12 (ICODE * pIcode, ICODE * pEnd)
{
    if (pIcode < pEnd)
    {
        if (((pIcode->ic.ll.flg & I) == I) && (pIcode->ic.ll.immed.op == 1))
            if (((pIcode+1)->ic.ll.opcode == iRCL) &&
                (((pIcode+1)->ic.ll.flg & I) == I) &&
                ((pIcode+1)->ic.ll.immed.op == 1))
                return (TRUE);
    }
    return (FALSE);
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
static boolT idiom9 (ICODE * pIcode, ICODE * pEnd)
{
    if (pIcode < pEnd)
    {
        if (((pIcode->ic.ll.flg & I) == I) && (pIcode->ic.ll.immed.op == 1))
            if (((pIcode+1)->ic.ll.opcode == iRCR) &&
                (((pIcode+1)->ic.ll.flg & I) == I) &&
                ((pIcode+1)->ic.ll.immed.op == 1))
                return (TRUE);
    }
    return (FALSE);
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
static boolT idiom10old (ICODE * pIcode, ICODE * pEnd)
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
                    return (TRUE);
            }
            else	/* at the end of the procedure */
                if (((pIcode+1) < pEnd) && ((pIcode+1)->ic.ll.opcode == iJNE))
                    return (TRUE);
    }
    return (FALSE);
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
static void idiom10 (ICODE * pIcode, ICODE * pEnd)
{
    if (pIcode < pEnd)
    {
        /* Check OR reg, reg */
        if (((pIcode->ic.ll.flg & I) != I) &&
            (pIcode->ic.ll.src. regi > 0) &&
            (pIcode->ic.ll.src.regi < INDEXBASE) &&
            (pIcode->ic.ll.src.regi == pIcode->ic.ll.dst.regi))
            if (((pIcode+1) < pEnd) && ((pIcode+1)->ic.ll.opcode == iJNE))
            {
                pIcode->ic.ll.opcode = iCMP;
                pIcode->ic.ll.flg |= I;
                pIcode->ic.ll.immed.op = 0;
                pIcode->du.def = 0;
                pIcode->du1.numRegsDef = 0;
            }
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
static byte idiom13 (ICODE * picode, ICODE * pend)
{ byte regi;

    if (picode < pend)
    {
        /* Check for regL */
        regi = picode->ic.ll.dst.regi;
        if (((picode->ic.ll.flg & I) != I) && (regi >= rAL) && (regi <= rBH))
        {
            /* Check for MOV regH, 0 */
            if (((picode+1)->ic.ll.opcode == iMOV) &&
                ((picode+1)->ic.ll.flg & I) &&
                ((picode+1)->ic.ll.immed.op == 0))
            {
                if ((picode+1)->ic.ll.dst.regi == (regi + 4))
                    return (regi - rAL + rAX);
            }
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
static boolT idiom14 (ICODE * picode, ICODE * pend, byte *regL, byte *regH)
{
    if (picode < pend)
    {
        /* Check for regL */
        *regL = picode->ic.ll.dst.regi;
        if (((picode->ic.ll.flg & I) != I) && ((*regL == rAX) || (*regL ==rBX)))
        {
            /* Check for XOR regH, regH */
            if (((picode+1)->ic.ll.opcode == iXOR) &&
                (((picode+1)->ic.ll.flg & I) != I))
            {
                *regH = (picode+1)->ic.ll.dst.regi;
                if (*regH == (picode+1)->ic.ll.src.regi)
                {
                    if ((*regL == rAX) && (*regH == rDX))
                        return (TRUE);
                    if ((*regL == rBX) && (*regH == rCX))
                        return (TRUE);
                }
            }
        }
    }
    return (FALSE);
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
static boolT idiom11 (ICODE * pIcode, ICODE * pEnd)
{	condId type;          /* type of argument */

        if ((pIcode + 2) < pEnd)
        {
            type = pIcode->idType(DST);
            if ((type != CONSTANT) && (type != OTHER))
            {
            /* Check NEG reg/mem
             *       SBB reg/mem, 0*/
                if (((pIcode+1)->ic.ll.opcode == iNEG) &&
                    ((pIcode+2)->ic.ll.opcode == iSBB))
                    switch (type) {
                        case GLOB_VAR:    if (((pIcode+2)->ic.ll.dst.segValue ==
                                               pIcode->ic.ll.dst.segValue) &&
                                              ((pIcode+2)->ic.ll.dst.off ==
                                               pIcode->ic.ll.dst.off))
                                return (TRUE);
                            break;
                        case REGISTER:    if ((pIcode+2)->ic.ll.dst.regi ==
                                              pIcode->ic.ll.dst.regi)
                                return (TRUE);
                            break;
                        case PARAM:
                        case LOCAL_VAR:   if ((pIcode+2)->ic.ll.dst.off ==
                                              pIcode->ic.ll.dst.off)
                                return (TRUE);
                            break;
                    }
            }
        }
        return (FALSE);
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
static boolT idiom16 (ICODE * picode, ICODE * pend)
{ byte regi;

    if ((picode+2) < pend)
    {
        regi = picode->ic.ll.dst.regi;
        if ((regi >= rAX) && (regi < INDEXBASE))
        {
            if (((picode+1)->ic.ll.opcode == iSBB) &&
                ((picode+2)->ic.ll.opcode == iINC))
                if (((picode+1)->ic.ll.dst.regi ==
                     ((picode+1)->ic.ll.src.regi)) &&
                    ((picode+1)->ic.ll.dst.regi == regi) &&
                    ((picode+2)->ic.ll.dst.regi == regi))
                    return (TRUE);
        }
    }
    return (FALSE);
}


/*****************************************************************************
 * idiom 18: Post-increment or post-decrement in a conditional jump
 *		MOV  reg, var (including register variables)
 *		INC	 var	or DEC var
 *		CMP  var, Y
 *		JX   label
 *		=> HLI_JCOND (var++ X Y)
 *		Eg:		MOV  ax, si
 *				INC  si
 *				CMP  ax, 8
 *				JL   labX
 *			=>  HLI_JCOND (si++ < 8)
 * 		Found in Borland Turbo C.  Intrinsic to C languages.
 ****************************************************************************/
static boolT idiom18 (ICODE * picode, ICODE * pend, Function * pproc)
{ boolT type = 0;	/* type of variable: 1 = reg-var, 2 = local */
    byte regi;		/* register of the MOV */

    /* Get variable */
    if (picode->ic.ll.dst.regi == 0)	/* global variable */
        /* not supported yet */ ;
    else if (picode->ic.ll.dst.regi < INDEXBASE)	/* register */
    {
        if ((picode->ic.ll.dst.regi == rSI) && (pproc->flg & SI_REGVAR))
            type = 1;
        else if ((picode->ic.ll.dst.regi == rDI) && (pproc->flg & DI_REGVAR))
            type = 1;
    }
    else if (picode->ic.ll.dst.off)		/* local variable */
        type = 2;
    else		/* indexed */
        /* not supported yet */ ;

    /* Check previous instruction for a MOV */
    if (type == 1)			/* register variable */
    {
        if (((picode-1)->ic.ll.opcode == iMOV) &&
            ((picode-1)->ic.ll.src.regi == picode->ic.ll.dst.regi))
        {
            regi = (picode-1)->ic.ll.dst.regi;
            if ((regi > 0) && (regi < INDEXBASE))
            {
                if ((picode < pend) && ((picode+1) < pend) &&
                    ((picode+1)->ic.ll.opcode == iCMP) &&
                    ((picode+1)->ic.ll.dst.regi == regi) &&
                    (((picode+2)->ic.ll.opcode >= iJB) &&
                     ((picode+2)->ic.ll.opcode < iJCXZ)))
                    return (TRUE);
            }
        }
    }
    else if (type == 2)		/* local */
    {
        if (((picode-1)->ic.ll.opcode == iMOV) &&
            ((picode-1)->ic.ll.src.off == picode->ic.ll.dst.off))
        {
            regi = (picode-1)->ic.ll.dst.regi;
            if ((regi > 0) && (regi < INDEXBASE))
            {
                if ((picode < pend) && ((picode+1) < pend) &&
                    ((picode+1)->ic.ll.opcode == iCMP) &&
                    ((picode+1)->ic.ll.dst.regi == regi) &&
                    (((picode+2)->ic.ll.opcode >= iJB) &&
                     ((picode+2)->ic.ll.opcode < iJCXZ)))
                    return (TRUE);
            }
        }
    }
    return (FALSE);
}


/*****************************************************************************
 * idiom 19: pre-increment or pre-decrement in conditional jump, comparing
 *			 against 0.
 *		INC var			or DEC var		(including register vars)
 *		JX	lab			   JX  lab
 *		=>  HLI_JCOND (++var X 0) or HLI_JCOND (--var X 0)
 *		Eg: INC [bp+4]
 *			JG  lab2
 *			=> HLI_JCOND (++[bp+4] > 0)
 *		Found in Borland Turbo C.  Intrinsic to C language.
 ****************************************************************************/
static boolT idiom19 (ICODE * picode, ICODE * pend, Function * pproc)
{
    if (picode->ic.ll.dst.regi == 0)	/* global variable */
        /* not supported yet */ ;
    else if (picode->ic.ll.dst.regi < INDEXBASE) /* register */
    {
        if (((picode->ic.ll.dst.regi == rSI) && (pproc->flg & SI_REGVAR)) ||
            ((picode->ic.ll.dst.regi == rDI) && (pproc->flg & DI_REGVAR)))
            if ((picode < pend) && ((picode+1)->ic.ll.opcode >= iJB) &&
                ((picode+1)->ic.ll.opcode < iJCXZ))
                return (TRUE);
    }
    else if (picode->ic.ll.dst.off)		/* stack variable */
    {
        if ((picode < pend) && ((picode+1)->ic.ll.opcode >= iJB) &&
            ((picode+1)->ic.ll.opcode < iJCXZ))
            return (TRUE);
    }
    else	/* indexed */
        /* not supported yet */ ;
    return (FALSE);
}


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
static boolT idiom20 (ICODE * picode, ICODE * pend, Function * pproc)
{ boolT type = 0;	/* type of variable: 1 = reg-var, 2 = local */
    byte regi;		/* register of the MOV */

    /* Get variable */
    if (picode->ic.ll.dst.regi == 0)	/* global variable */
        /* not supported yet */ ;
    else if (picode->ic.ll.dst.regi < INDEXBASE)	/* register */
    {
        if ((picode->ic.ll.dst.regi == rSI) && (pproc->flg & SI_REGVAR))
            type = 1;
        else if ((picode->ic.ll.dst.regi == rDI) && (pproc->flg & DI_REGVAR))
            type = 1;
    }
    else if (picode->ic.ll.dst.off)		/* local variable */
        type = 2;
    else		/* indexed */
        /* not supported yet */ ;

    /* Check previous instruction for a MOV */
    if (type == 1)			/* register variable */
    {
        if ((picode < pend) && ((picode+1)->ic.ll.opcode == iMOV) &&
            ((picode+1)->ic.ll.src.regi == picode->ic.ll.dst.regi))
        {
            regi = (picode+1)->ic.ll.dst.regi;
            if ((regi > 0) && (regi < INDEXBASE))
            {
                if (((picode+1) < pend) && ((picode+2) < pend) &&
                    ((picode+2)->ic.ll.opcode == iCMP) &&
                    ((picode+2)->ic.ll.dst.regi == regi) &&
                    (((picode+3)->ic.ll.opcode >= iJB) &&
                     ((picode+3)->ic.ll.opcode < iJCXZ)))
                    return (TRUE);
            }
        }
    }
    else if (type == 2)		/* local */
    {
        if ((picode < pend) && ((picode+1)->ic.ll.opcode == iMOV) &&
            ((picode+1)->ic.ll.src.off == picode->ic.ll.dst.off))
        {
            regi = (picode+1)->ic.ll.dst.regi;
            if ((regi > 0) && (regi < INDEXBASE))
            {
                if (((picode+1) < pend) && ((picode+2) < pend) &&
                    ((picode+2)->ic.ll.opcode == iCMP) &&
                    ((picode+2)->ic.ll.dst.regi == regi) &&
                    (((picode+3)->ic.ll.opcode >= iJB) &&
                     ((picode+3)->ic.ll.opcode < iJCXZ)))
                    return (TRUE);
            }
        }
    }
    return (FALSE);
}


/*****************************************************************************
 * findIdioms  - translates LOW_LEVEL icode idioms into HIGH_LEVEL icodes.
 ****************************************************************************/
void Function::findIdioms()
{
    Int     ip;             /* Index to current icode                   */
    ICODE *  pEnd, * pIcode;   /* Pointers to end of BB and current icodes */
    int16   delta;
    COND_EXPR *rhs, *lhs;   /* Pointers to left and right hand side exps */
    COND_EXPR *exp;         /* Pointer to temporal expression           */
    Int     idx;            /* Index into local identifier table        */
    byte    regH, regL;     /* High and low registers for long word reg */

    pIcode = Icode.GetFirstIcode();
    pEnd = pIcode + Icode.GetNumIcodes();
    ip = 0;

    while (pIcode < pEnd)
    {
        switch (pIcode->ic.ll.opcode) {
            case iDEC: case iINC:
                if (idiom18 (pIcode, pEnd, this))
                {
                    lhs = COND_EXPR::id (pIcode-1, SRC, this, ip, pIcode, eUSE);
                    if (pIcode->ic.ll.opcode == iDEC)
                        lhs = COND_EXPR::unary (POST_DEC, lhs);
                    else
                        lhs = COND_EXPR::unary (POST_INC, lhs);
                    rhs = COND_EXPR::id (pIcode+1, SRC, this, ip, pIcode+2, eUSE);
                    exp = COND_EXPR::boolOp (lhs, rhs,
                                       condOpJCond[(pIcode+2)->ic.ll.opcode - iJB]);
                    (pIcode+2)->setJCond(exp);

                    (pIcode-1)->invalidate();
                    pIcode->invalidate();
                    (pIcode+1)->invalidate();
                    pIcode += 3;
                    ip += 2;
                }
                else if (idiom19 (pIcode, pEnd, this))
                {
                    lhs = COND_EXPR::id (pIcode, DST, this, ip, pIcode+1, eUSE);
                    if (pIcode->ic.ll.opcode == iDEC)
                        lhs = COND_EXPR::unary (PRE_DEC, lhs);
                    else
                        lhs = COND_EXPR::unary (PRE_INC, lhs);
                    rhs = COND_EXPR::idKte (0, 2);
                    exp = COND_EXPR::boolOp (lhs, rhs,
                                       condOpJCond[(pIcode+1)->ic.ll.opcode - iJB]);
                    (pIcode+1)->setJCond(exp);
                    pIcode->invalidate();
                    pIcode += 2;
                    ip++;
                }
                else if (idiom20 (pIcode, pEnd, this))
                {
                    lhs = COND_EXPR::id (pIcode+1, SRC, this, ip, pIcode, eUSE);
                    if (pIcode->ic.ll.opcode == iDEC)
                        lhs = COND_EXPR::unary (PRE_DEC, lhs);
                    else
                        lhs = COND_EXPR::unary (PRE_INC, lhs);
                    rhs = COND_EXPR::id (pIcode+2, SRC, this, ip, pIcode+3, eUSE);
                    exp = COND_EXPR::boolOp (lhs, rhs,
                                       condOpJCond[(pIcode+3)->ic.ll.opcode - iJB]);
                    (pIcode+3)->setJCond(exp);
                    pIcode->invalidate();
                    (pIcode+1)->invalidate();
                    (pIcode+2)->invalidate();
                    pIcode += 3;
                    ip += 2;
                }
                else
                    pIcode++;
                break;

            case iPUSH:             /* Idiom 1 */
                if ((! (flg & PROC_HLL)) && (idx = idiom1 (pIcode, pEnd, this)))
                {
                    flg |= PROC_HLL;
                    for ( ; idx > 0; idx--)
                    {
                        (pIcode++)->invalidate();
                        ip++;
                    }
                    ip--;
                }
                else
                    pIcode++;
                break;

            case iMOV:              /* Idiom 2 */
                if (idx = idiom2(pIcode, pEnd, ip, this))
                {
                    pIcode->invalidate();
                    (pIcode+1)->invalidate();
                    pIcode += 3;
                    ip += 2;
                }
                else if (idiom14 (pIcode, pEnd, &regL, &regH))  /* Idiom 14 */
                {
                    idx = localId.newLongReg (TYPE_LONG_SIGN, regH, regL, ip);
                    lhs = COND_EXPR::idLongIdx (idx);
                    pIcode->setRegDU( regH, eDEF);
                    rhs = COND_EXPR::id (pIcode, SRC, this, ip, pIcode, NONE);
                    pIcode->setAsgn(lhs, rhs);
                    (pIcode+1)->invalidate();
                    pIcode += 2;
                    ip++;
                }
                else if (idx = idiom13 (pIcode, pEnd))      /* Idiom 13 */
                {
                    lhs = COND_EXPR::idReg (idx, 0, &localId);
                    pIcode->setRegDU( idx, eDEF);
                    pIcode->du1.numRegsDef--;   	/* prev byte reg def */
                    rhs = COND_EXPR::id (pIcode, SRC, this, ip, pIcode, NONE);
                    pIcode->setAsgn(lhs, rhs);
                    (pIcode+1)->invalidate();
                    pIcode += 2;
                    ip++;
                }
                else
                    pIcode++;
                break;

            case iCALL:  case iCALLF:
                /* Check for library functions that return a long register.
                         * Propagate this result */
                if (pIcode->ic.ll.immed.proc.proc != 0)
                    if ((pIcode->ic.ll.immed.proc.proc->flg & PROC_ISLIB) &&
                        (pIcode->ic.ll.immed.proc.proc->flg & PROC_IS_FUNC))
                    {
                        if ((pIcode->ic.ll.immed.proc.proc->retVal.type==TYPE_LONG_SIGN)
                            || (pIcode->ic.ll.immed.proc.proc->retVal.type ==
                                TYPE_LONG_UNSIGN))
                            localId.newLongReg(TYPE_LONG_SIGN, rDX, rAX, ip);
                    }

                /* Check for idioms */
                if (idx = idiom3(pIcode, pEnd))         /* idiom 3 */
                {
                    if (pIcode->ic.ll.flg & I)
                    {
                        (pIcode->ic.ll.immed.proc.proc)->cbParam = (int16)idx;
                        pIcode->ic.ll.immed.proc.cb = idx;
                        (pIcode->ic.ll.immed.proc.proc)->flg |= CALL_C;
                        pIcode++;
                        (pIcode++)->invalidate();
                        ip++;
                    }
                }
                else if (idx = idiom17 (pIcode, pEnd))  /* idiom 17 */
                {
                    if (pIcode->ic.ll.flg & I)
                    {
                        (pIcode->ic.ll.immed.proc.proc)->cbParam = (int16)idx;
                        pIcode->ic.ll.immed.proc.cb = idx;
                        (pIcode->ic.ll.immed.proc.proc)->flg |= CALL_C;
                        ip += idx/2 - 1;
                        pIcode++;
                        for (idx /= 2; idx > 0; idx--)
                            (pIcode++)->invalidate();
                    }
                }
                else
                    pIcode++;
                break;

            case iRET:          /* Idiom 4 */
            case iRETF:
                idiom4 (pIcode, pEnd, this);
                pIcode++;
                break;

            case iADD:          /* Idiom 5 */
                if (idiom5 (pIcode, pEnd))
                {
                    lhs = COND_EXPR::idLong (&localId, DST, pIcode, LOW_FIRST,
                                         ip, USE_DEF, 1);
                    rhs = COND_EXPR::idLong (&localId, SRC, pIcode, LOW_FIRST,
                                         ip, eUSE, 1);
                    exp = COND_EXPR::boolOp (lhs, rhs, ADD);
                    pIcode->setAsgn(lhs, exp);
                    (pIcode+1)->invalidate();
                    pIcode++;
                    ip++;
                }
                pIcode++;
                break;

            case iSAR:          /* Idiom 8 */
                if (idiom8 (pIcode, pEnd))
                {
                    idx = localId.newLongReg(TYPE_LONG_SIGN,
                                        pIcode->ic.ll.dst.regi, (pIcode+1)->ic.ll.dst.regi,ip);
                    lhs = COND_EXPR::idLongIdx (idx);
                    pIcode->setRegDU( (pIcode+1)->ic.ll.dst.regi, USE_DEF);
                    rhs = COND_EXPR::idKte (1, 2);
                    exp = COND_EXPR::boolOp (lhs, rhs, SHR);
                    pIcode->setAsgn(lhs, exp);
                    (pIcode+1)->invalidate();
                    pIcode++;
                    ip++;
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
                        ip++;
                    }
                }
                else if (idiom12 (pIcode, pEnd))        /* idiom 12 */
                {
                    idx = localId.newLongReg (TYPE_LONG_UNSIGN,
                                        (pIcode+1)->ic.ll.dst.regi, pIcode->ic.ll.dst.regi,ip);
                    lhs = COND_EXPR::idLongIdx (idx);
                    pIcode->setRegDU( (pIcode+1)->ic.ll.dst.regi, USE_DEF);
                    rhs = COND_EXPR::idKte (1, 2);
                    exp = COND_EXPR::boolOp (lhs, rhs, SHL);
                    pIcode->setAsgn(lhs, exp);
                    (pIcode+1)->invalidate();
                    pIcode += 2;
                    ip++;
                }
                else
                    pIcode++;
                break;

            case iSHR:          /* Idiom 9 */
                if (idiom9 (pIcode, pEnd))
                {
                    idx = localId.newLongReg (TYPE_LONG_UNSIGN,
                                        pIcode->ic.ll.dst.regi, (pIcode+1)->ic.ll.dst.regi,ip);
                    lhs = COND_EXPR::idLongIdx (idx);
                    pIcode->setRegDU( (pIcode+1)->ic.ll.dst.regi, USE_DEF);
                    rhs = COND_EXPR::idKte (1, 2);
                    exp = COND_EXPR::boolOp (lhs, rhs, SHR);
                    pIcode->setAsgn(lhs, exp);
                    (pIcode+1)->invalidate();
                    pIcode++;
                    ip++;
                }
                pIcode++;
                break;

            case iSUB:          /* Idiom 6 */
                if (idiom6 (pIcode, pEnd))
                {
                    lhs = COND_EXPR::idLong (&localId, DST, pIcode, LOW_FIRST,
                                         ip, USE_DEF, 1);
                    rhs = COND_EXPR::idLong (&localId, SRC, pIcode, LOW_FIRST,
                                         ip, eUSE, 1);
                    exp = COND_EXPR::boolOp (lhs, rhs, SUB);
                    pIcode->setAsgn(lhs, exp);
                    (pIcode+1)->invalidate();
                    pIcode++;
                    ip++;
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
                                         ip, USE_DEF, 1);
                    rhs = COND_EXPR::unary (NEGATION, lhs);
                    pIcode->setAsgn(lhs, rhs);
                    (pIcode+1)->invalidate();
                    (pIcode+2)->invalidate();
                    pIcode += 3;
                    ip += 2;
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
                    ip += 2;
                }
                else
                    pIcode++;
                break;

            case iNOP:
                (pIcode++)->invalidate();
                break;

            case iENTER:		/* ENTER is equivalent to init PUSH bp */
                if (ip == 0)
                    flg |= (PROC_HLL | PROC_IS_HLL);
                pIcode++;
                break;

            case iXOR:          /* Idiom 7 */
                if (idiom21 (pIcode, pEnd))
                {
                    lhs = COND_EXPR::idLong (&localId, DST, pIcode,HIGH_FIRST, ip, eDEF, 1);
                    rhs = COND_EXPR::idKte ((pIcode+1)->ic.ll.immed.op , 4);
                    pIcode->setAsgn(lhs, rhs);
                    pIcode->du.use = 0;		/* clear register used in iXOR */
                    (pIcode+1)->invalidate();
                    pIcode++;
                    ip++;
                }
                else if (idiom7 (pIcode))
                {
                    lhs = COND_EXPR::id (pIcode, DST, this, ip, pIcode, NONE);
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
        ip++;
    }

    /* Check if number of parameter bytes match their calling convention */
    if ((flg & PROC_HLL) && (!args.sym.empty()))
    {
        args.minOff += (flg & PROC_FAR ? 4 : 2);
        delta = args.maxOff - args.minOff;
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
    Int i, j;                 /* idx into icode array */
    ICODE * pIcode;            /* ptr icode array      */
    dword *p;                 /* for case table       */

    if (! Icode.GetNumIcodes())        /* No Icode */
        return;
    pIcode = Icode.GetFirstIcode();

    /* Flag all jump targets for BB construction and disassembly stage 2 */
    for (i = 0; i < Icode.GetNumIcodes(); i++)
        if ((pIcode[i].ic.ll.flg & I) && JmpInst(pIcode[i].ic.ll.opcode))
        {
            if (Icode.labelSrch(pIcode[i].ic.ll.immed.op, &j))
            {
                pIcode[j].ic.ll.flg |= TARGET;
            }
        }

    /* Finally bind jump targets to Icode offsets.  Jumps for which no label
     * is found (no code at dest. of jump) are simply left unlinked and
     * flagged as going nowhere.  */
    pIcode = Icode.GetFirstIcode();
    for (i = 0; i < Icode.GetNumIcodes(); i++)
        if (JmpInst(pIcode[i].ic.ll.opcode))
        {
            if (pIcode[i].ic.ll.flg & I)
            {
                if (! Icode.labelSrch(pIcode[i].ic.ll.immed.op,
                                      (Int *)&pIcode[i].ic.ll.immed.op))
                    pIcode[i].ic.ll.flg |= NO_LABEL;
            }
            else if (pIcode[i].ic.ll.flg & SWITCH)
            {
                p = pIcode[i].ic.ll.caseTbl.entries;
                for (j = 0; j < pIcode[i].ic.ll.caseTbl.numEntries; j++, p++)
                    Icode.labelSrch(*p, (Int *)p);
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
