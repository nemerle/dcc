#include "arith_idioms.h"

#include "dcc.h"
#include "msvc_fixes.h"

using namespace std;

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
bool Idiom5::match(iICODE pIcode)
{
    if(distance(pIcode,m_end)<2)
        return false;
    m_icodes[0]=pIcode++;
    m_icodes[1]=pIcode++;
    if (m_icodes[1]->ll()->match(iADC))
        return true;
    return false;
}

int Idiom5::action()
{
    AstIdent *rhs,*lhs;
    Expr *expr;
    lhs = AstIdent::Long (&m_func->localId, DST, m_icodes[0], LOW_FIRST, m_icodes[0], USE_DEF, *m_icodes[1]->ll());
    rhs = AstIdent::Long (&m_func->localId, SRC, m_icodes[0], LOW_FIRST, m_icodes[0], eUSE, *m_icodes[1]->ll());
    expr = new BinaryOperator(ADD,lhs, rhs);
    m_icodes[0]->setAsgn(lhs, expr);
    m_icodes[1]->invalidate();
    return 2;

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
bool Idiom6::match(iICODE pIcode)
{
    if(distance(pIcode,m_end)<2)
        return false;
    m_icodes[0]=pIcode++;
    m_icodes[1]=pIcode++;
    if (m_icodes[1]->ll()->match(iSBB))
        return true;
    return false;
}

int Idiom6::action()
{

    AstIdent *rhs,*lhs;
    Expr *expr;
    lhs = AstIdent::Long (&m_func->localId, DST, m_icodes[0], LOW_FIRST, m_icodes[0], USE_DEF, *m_icodes[1]->ll());
    rhs = AstIdent::Long (&m_func->localId, SRC, m_icodes[0], LOW_FIRST, m_icodes[0], eUSE, *m_icodes[1]->ll());
    expr = new BinaryOperator(SUB,lhs, rhs);
    m_icodes[0]->setAsgn(lhs, expr);
    m_icodes[1]->invalidate();
    return 2;

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
bool Idiom18::match(iICODE picode)
{
    if(picode==m_func->Icode.begin())
        return false;
    if(std::distance(picode,m_end)<3)
        return false;
    --picode; //

    for(int i=0; i<4; ++i)
        m_icodes[i] =picode++;

    m_idiom_type=-1;
    m_is_dec = m_icodes[1]->ll()->match(iDEC);

    uint8_t regi;		/* register of the MOV */
    if(not m_icodes[0]->ll()->matchWithRegDst(iMOV) )
        return false;
    regi = m_icodes[0]->ll()->m_dst.regi;
    if( not ( m_icodes[2]->ll()->match(iCMP,regi)  and
              m_icodes[3]->ll()->conditionalJump() ) )
        return false;
    // Simple matching finished, select apropriate matcher based on dst type
    /* Get variable */
    if (m_icodes[1]->ll()->m_dst.regi == 0)	/* global variable */
    {
        /* not supported yet */
        m_idiom_type = 0;
    }
    else if ( m_icodes[1]->ll()->m_dst.isReg() )	/* register */
    {
        m_idiom_type = 1;
//        if ((m_icodes[1]->ll()->dst.regi == rSI) and (m_func->flg & SI_REGVAR))
//            m_idiom_type = 1;
//        else if ((m_icodes[1]->ll()->dst.regi == rDI) and (m_func->flg & DI_REGVAR))
//            m_idiom_type = 1;
    }
    else if (m_icodes[1]->ll()->m_dst.off)		/* local variable */
        m_idiom_type = 2;
    else		/* indexed */
    {
        m_idiom_type=3;
        /* not supported yet */
        ICODE &ic(*picode);
        const Function *my_proc(ic.getParent()->getParent());
        printf("Unsupported idiom18 type at %x in %s:%x : indexed\n",ic.loc_ip,my_proc->name.c_str(),my_proc->procEntry);
    }

    switch(m_idiom_type)
    {
        case 0: // global
            printf("Unsupported idiom18 type at %x : global variable\n",picode->loc_ip);
            break;
        case 1:  /* register variable */
            /* Check previous instruction for a MOV */
            if ( (m_icodes[0]->ll()->src().regi == m_icodes[1]->ll()->m_dst.regi))
            {
                return true;
            }
            break;
        case 2: /* local */
            if ((m_icodes[0]->ll()->src().off == m_icodes[1]->ll()->m_dst.off))
            {
                return true;
            }
            break;
        case 3: // indexed
            printf("Untested idiom18 type: indexed\n");
            if ((m_icodes[0]->ll()->src() == m_icodes[1]->ll()->m_dst))
            {
                return true;
            }
            break;
    }
    return false;
}

int Idiom18::action() // action length
{
    Expr *rhs,*lhs;/* Pointers to left and right hand side exps */
    Expr *expr;
    lhs     = AstIdent::id (*m_icodes[0]->ll(), SRC, m_func, m_icodes[1], *m_icodes[1], eUSE);
    lhs     = UnaryOperator::Create(m_is_dec ? POST_DEC : POST_INC, lhs);
    rhs     = AstIdent::id (*m_icodes[2]->ll(), SRC, m_func, m_icodes[1], *m_icodes[3], eUSE);
    expr    = new BinaryOperator(condOpJCond[m_icodes[3]->ll()->getOpcode() - iJB],lhs, rhs);
    m_icodes[3]->setJCond(expr);

    m_icodes[0]->invalidate();
    m_icodes[1]->invalidate();
    m_icodes[2]->invalidate();
    return 3;
}

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
bool Idiom19::match(iICODE picode)
{
    if(std::distance(picode,m_end)<2)
        return false;
    ICODE &ic(*picode);
    int type;
    for(int i=0; i<2; ++i)
        m_icodes[i] =picode++;
    m_is_dec = m_icodes[0]->ll()->match(iDEC);
    if ( not m_icodes[1]->ll()->conditionalJump() )
        return false;
    if (m_icodes[0]->ll()->m_dst.regi == 0)	/* global variable */
        /* not supported yet */ ;
    else if ( m_icodes[0]->ll()->m_dst.isReg() ) /* register */
    {
        //        if (((picode->ll()->dst.regi == rSI) and (pproc->flg & SI_REGVAR)) or
        //            ((picode->ll()->dst.regi == rDI) and (pproc->flg & DI_REGVAR)))
        return true;
    }
    else if (m_icodes[0]->ll()->m_dst.off)		/* stack variable */
    {
        return true;
    }
    else	/* indexed */
    {
        fprintf(stderr,"idiom19 : Untested type [indexed]\n");
        return true;

        /* not supported yet */
    }
    return false;
}
int Idiom19::action()
{
    Expr *lhs,*expr;

    lhs = AstIdent::id (*m_icodes[0]->ll(), DST, m_func, m_icodes[0], *m_icodes[1], eUSE);
    lhs = UnaryOperator::Create(m_is_dec ? PRE_DEC : PRE_INC, lhs);
    expr = new BinaryOperator(condOpJCond[m_icodes[1]->ll()->getOpcode() - iJB],lhs, new Constant(0, 2));
    m_icodes[1]->setJCond(expr);
    m_icodes[0]->invalidate();
    return 2;
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
bool Idiom20::match(iICODE picode)
{
    uint8_t type = 0;	/* type of variable: 1 = reg-var, 2 = local */
    uint8_t regi;		/* register of the MOV */
    if(std::distance(picode,m_end)<4)
        return false;
    for(int i=0; i<4; ++i)
        m_icodes[i] =picode++;
    /* Check second instruction for a MOV */
    if( not m_icodes[1]->ll()->matchWithRegDst(iMOV) )
        return false;

    m_is_dec = m_icodes[0]->ll()->match(iDEC) ? PRE_DEC : PRE_INC;

    const LLOperand &ll_dest(m_icodes[0]->ll()->m_dst);
    /* Get variable */
    if (ll_dest.regi == 0)	/* global variable */
    {
        /* not supported yet */ ;
    }
    else if ( ll_dest.isReg() )	/* register */
    {
        type = 1;
//        if ((ll_dest.regi == rSI) and (m_func->flg & SI_REGVAR))
//            type = 1;
//        else if ((ll_dest.regi == rDI) and (m_func->flg & DI_REGVAR))
//            type = 1;
    }
    else if (ll_dest.off)		/* local variable */
        type = 2;
    else		/* indexed */
    {
        printf("idiom20 : Untested type [indexed]\n");
        type = 3;
        /* not supported yet */ ;
    }
    regi = m_icodes[1]->ll()->m_dst.regi;
    const LLOperand &mov_src(m_icodes[1]->ll()->src());
    if (m_icodes[2]->ll()->match(iCMP,(eReg)regi) and m_icodes[3]->ll()->conditionalJump())
    {
        switch(type)
        {
            case 1: /* register variable */
                if ((mov_src.regi == ll_dest.regi))
                {
                    return true;
                }
                break;
            case 2: // local
                if ((mov_src.off == ll_dest.off))
                {
                    return true;
                }
                break;
            case 3:
                fprintf(stderr,"Test 3 ");
                if ((mov_src == ll_dest))
                {
                    return true;
                }
                break;
        }
    }
    return false;
}
int Idiom20::action()
{
    Expr *lhs,*rhs,*expr;
    lhs  = AstIdent::id (*m_icodes[1]->ll(), SRC, m_func, m_icodes[0], *m_icodes[0], eUSE);
    lhs  = UnaryOperator::Create(m_is_dec, lhs);
    rhs  = AstIdent::id (*m_icodes[2]->ll(), SRC, m_func, m_icodes[0], *m_icodes[3], eUSE);
    expr = new BinaryOperator(condOpJCond[m_icodes[3]->ll()->getOpcode() - iJB],lhs, rhs);
    m_icodes[3]->setJCond(expr);
    for(int i=0; i<3; ++i)
        m_icodes[i]->invalidate();
    return 4;
}
