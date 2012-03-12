/*
 * Input file	: ./tests/inputs/TESTLONG.EXE
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
int loc3;
int loc4;
    scanf ("%d", &loc1);
    scanf ("%d", &loc2);
    scanf ("%d %d", &loc2, &loc1);
    printf ("%ld %ld", loc2, loc4, loc1, loc3);
}

