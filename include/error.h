/*****************************************************************************
 * Error codes
 * (C) Cristina Cifuentes
 ****************************************************************************/


/* These definitions refer to errorMessage in error.c */

#define USAGE                   0
#define INVALID_ARG             1
#define INVALID_OPCODE          2
#define INVALID_386OP           3
#define FUNNY_SEGOVR            4
#define FUNNY_REP               5
#define CANNOT_OPEN             6
#define CANNOT_READ             7
#define MALLOC_FAILED           8
#define NEWEXE_FORMAT           9

#define NO_BB              		10
#define INVALID_SYNTHETIC_BB    11
#define INVALID_INT_BB          12
#define IP_OUT_OF_RANGE         13
#define DEF_NOT_FOUND           14
#define JX_NOT_DEF				15
#define NOT_DEF_USE				16
#define REPEAT_FAIL				17
#define WHILE_FAIL				18


void fatalError(Int errId, ...);
void reportError(Int errId, ...);

