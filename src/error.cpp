/****************************************************************************
 *          dcc project error messages
 * (C) Cristina Cifuentes
 ***************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <map>
#include <string>
#include <stdarg.h>

#include "dcc.h"

    static std::map<eErrorId,std::string> errorMessage =
    {
      {INVALID_ARG      ,"Invalid option -%c\n"},
      {INVALID_OPCODE   ,"Invalid instruction %02X at location %06lX\n"},
      {INVALID_386OP    ,"Don't understand 80386 instruction %02X at location %06lX\n"},
      {FUNNY_SEGOVR     ,"Segment override with no memory operand at location %06lX\n"},
      {FUNNY_REP        ,"REP prefix without a string instruction at location %06lX\n"},
      {CANNOT_OPEN      ,"Cannot open %s\n"},
      {CANNOT_READ      ,"Error while reading %s\n"},
      {MALLOC_FAILED    ,"malloc of %ld bytes failed\n"},
      {NEWEXE_FORMAT    ,"Don't understand new EXE format\n"},
      {NO_BB            ,"Failed to find a BB for jump to %ld in proc %s\n"},
      {INVALID_SYNTHETIC_BB,"Basic Block is a synthetic jump\n"},
      {INVALID_INT_BB   ,"Failed to find a BB for interval\n"},
      {IP_OUT_OF_RANGE  ,"Instruction at location %06lX goes beyond loaded image\n"},
      {DEF_NOT_FOUND    ,"Definition not found for condition code usage at opcode %d\n"},
      {JX_NOT_DEF       ,"JX use, definition not supported at opcode #%d\n"},
      {NOT_DEF_USE      ,"%x: Def - use not supported.  Def op = %d, use op = %d.\n"},
      {REPEAT_FAIL      ,"Failed to construct repeat..until() condition.\n"},
      {WHILE_FAIL       ,"Failed to construct while() condition.\n"},
    };

/****************************************************************************
 fatalError: displays error message and exits the program.
 ****************************************************************************/
void fatalError(eErrorId errId, ...)
{  va_list args;
//#ifdef __UNIX__   /* ultrix */
#if 0
   int errId;

    va_start(args);
    errId = va_arg(args, int);
#else
    va_start(args, errId);
#endif

    if (errId == USAGE)
       fprintf(stderr,"Usage: dcc [-a1a2cmpsvVi][-o asmfile] DOS_executable\n");
    else {
        fprintf(stderr, "dcc: ");
        vfprintf(stderr, errorMessage[errId].c_str(), args);
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
   int errId;

    va_start(args);
    errId = va_arg(args, int);
#else           /* msdos or windows*/
    va_start(args, errId);
#endif
    fprintf(stderr, "dcc: ");
    vfprintf(stderr, errorMessage[errId].c_str(), args);
    va_end(args);
}
