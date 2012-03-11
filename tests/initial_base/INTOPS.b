/*
 * Input file	: test\intops.exe
 * File type	: EXE
 */

#include "dcc.h"


void main ()
/* Takes no parameters.
 * High-level language prologue code.
 */
{
int loc1;
int loc2;
long loc3; /* dx:ax */
int loc4; /* tmp */

    loc1 = 255;
    loc2 = 143;
    loc2 = (loc1 + loc2);
    loc1 = (loc1 - loc2);
    loc1 = (loc1 * loc2);
    loc3 = loc2;
    loc2 = (loc4 / loc1);
    loc3 = loc2;
    LO(loc3) = (loc4 / loc1);
    loc2 = (loc4 % loc1);
    loc1 = (loc1 << 5);
    loc2 = (loc2 >> loc1);
    printf ("a = %d, b = %d\n", loc1, loc2);
}

