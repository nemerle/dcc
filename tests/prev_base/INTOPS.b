/*
 * Input file	: ./tests/inputs/INTOPS.EXE
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
    loc1 = 255;
    loc2 = (loc1 + 143);
    loc1 = (loc1 - loc2);
    loc1 = (loc1 * loc2);
    loc1 = (loc1 << 5);
    printf ("a = %d, b = %d\n", loc1, (((loc2 / loc1) % loc1) >> loc1));
}

