/****************************************************************************
 *          dcc project error messages
 * (C) Cristina Cifuentes
 ***************************************************************************/

#include "dcc.h"

#include <stdio.h>
#include <stdlib.h>
//#ifndef __UNIX__
#if 1
#include <stdarg.h>
#else
#include <varargs.h>
#endif

static const char *errorMessage[] = {
  "Invalid option -%c\n",                                   /* INVALID_ARG    */
  "Invalid instruction %02X at location %06lX\n",           /* INVALID_OPCODE */
  "Don't understand 80386 instruction %02X at location %06lX\n", /* INVALID_386OP  */
  "Segment override with no memory operand at location %06lX\n", /* FUNNY_SEGOVR   */
  "REP prefix without a string instruction at location %06lX\n",/* FUNNY_REP */
  "Cannot open %s\n",                                       /* CANNOT_OPEN    */
  "Error while reading %s\n",                               /* CANNOT_READ    */
  "malloc of %ld bytes failed\n",                           /* MALLOC_FAILED  */
  "Don't understand new EXE format\n",                      /* NEWEXE_FORMAT  */
  "Failed to find a BB for jump to %ld in proc %s\n",    	/* NO_BB		  */
  "Basic Block is a synthetic jump\n",                /* INVALID_SYNTHETIC_BB */
  "Failed to find a BB for interval\n",                     /* INVALID_INT_BB */
  "Instruction at location %06lX goes beyond loaded image\n", /* IP_OUT_OF_RANGE*/
  "Definition not found for condition code usage at opcode %d\n",
                                                            /* DEF_NOT_FOUND */
  "JX use, definition not supported at opcode #%d\n",		/* JX_NOT_DEF */
  "Def - use not supported.  Def op = %d, use op = %d.\n",  /* NOT_DEF_USE */
  "Failed to construct repeat..until() condition.\n",		/* REPEAT_FAIL */
  "Failed to construct while() condition.\n",				/* WHILE_FAIL */
};


/****************************************************************************
 fatalError: displays error message and exits the program.
 ****************************************************************************/
void fatalError(eErrorId errId, ...)
{  va_list args;
//#ifdef __UNIX__   /* ultrix */
#if 0
   Int errId;

    va_start(args);
    errId = va_arg(args, Int);
#else
    va_start(args, errId);
#endif

    if (errId == USAGE)
       fprintf(stderr,"Usage: dcc [-a1a2cmpsvVi][-o asmfile] DOS_executable\n");
    else {
        fprintf(stderr, "dcc: ");
        vfprintf(stderr, errorMessage[errId - 1], args);
    }
    va_end(args);
    exit((int)errId);
}


/****************************************************************************
 reportError: reports the warning/error and continues with the program.
 ****************************************************************************/
void reportError(eErrorId errId, ...)
{  va_list args;
//#ifdef __UNIX__   /* ultrix */
#if 0
   Int errId;

    va_start(args);
    errId = va_arg(args, Int);
#else           /* msdos or windows*/
    va_start(args, errId);
#endif
    fprintf(stderr, "dcc: ");
    vfprintf(stderr, errorMessage[errId - 1], args);
    va_end(args);
}
