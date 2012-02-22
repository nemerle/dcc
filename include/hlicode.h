/*
 * File:    hlIcode.h
 * Purpose: module definitions for high-level icodes
 * Date:    September 1993
 */


/* High level icodes opcodes - def in file icode.h */
struct HLICODE
{
    hlIcode             opcode;     /* hlIcode opcode           */
    union {                         /* different operands       */
        struct {
            COND_EXPR   *lhs;
            COND_EXPR   *rhs;
        }               asgn;       /* for HLI_ASSIGN hlIcode       */
        COND_EXPR       *exp;       /* for HLI_JCOND, INC, DEC      */
    }                   oper;       /* operand                  */
    boolT               valid;      /* has a valid hlIcode      */
};
