/*
 * Input file	: ./tests/inputs/BENCHMUS.EXE
 * File type	: EXE
 */

#include "dcc.h"


void main ()
/* Takes no parameters.
 * High-level language prologue code.
 */
{
int loc1;
long loc2;
long loc3;
int loc4;
int loc5;
    printf ("enter number of iterations\n");
    scanf ("%ld", &loc0);
    printf ("executing %ld iterations\n", loc3);
    loc4 = 20;
    loc1 = loc4;
    loc2 = 1;

    while ((loc2 <= loc3)) {
        loc5 = 1;

        while ((loc5 <= 40)) {
            loc4 = (((((((((((((((((((((((((loc4 * loc4) * loc4) * loc4) * loc4) * loc4) * loc4) * loc4) * loc4) * loc4) * loc4) * loc4) * loc4) * loc4) * loc4) * loc4) * loc4) * loc4) * loc4) * loc4) * loc4) * loc4) * loc4) * loc4) * loc4) * 3);
            loc5 = (loc5 + 1);
        }	/* end of while */
        loc2 = (loc2 + 1);
    }	/* end of while */
    printf ("a=%d\n", loc4);
}

