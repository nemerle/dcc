/* 
 * File:    hlIcode.h
 * Purpose: module definitions for high-level icodes
 * Date:    September 1993
 */


/* High level icodes opcodes - def in file icode.h */
/*typedef enum {
    HLI_ASSIGN,
    INC,
    DEC,
    HLI_JCOND,

} hlIcode; */


typedef struct {
    hlIcode             opcode;     /* hlIcode opcode           */
    union {                         /* different operands       */
        struct {
            COND_EXPR   *lhs;
            COND_EXPR   *rhs;
        }               asgn;       /* for HLI_ASSIGN hlIcode       */
        COND_EXPR       *exp;       /* for HLI_JCOND, INC, DEC      */
    }                   oper;       /* operand                  */
    boolT               valid;      /* has a valid hlIcode      */
} HLICODE;


//typedef struct {
//    Int         numIcodes;      /* No. of hlIcode reocrds written   */
//    Int         numAlloc;       /* No. of hlIcode records allocated */
//    HLICODE     *hlIcode;       /* Array of high-level icodes       */
//} HLICODEREC;

