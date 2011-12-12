/*
 * Input file	: ./tests/inputs/BENCHMUL.EXE
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
long loc3;
long loc4;
int loc5;

    printf ("enter number of iterations\n");
    scanf ("%ld", &loc0);
    printf ("executing %ld iterations\n", loc4);
    scanf ("%d", &loc1);
    scanf ("%d", &loc2);
    loc3 = 1;

    while ((loc3 <= loc4)) {
        loc5 = 1;

        while ((loc5 <= 40)) {
            loc1 = (((((((((((((((((((((((((loc1 * loc1) * loc1) * loc1) * loc1) * loc1) * loc1) * loc1) * loc1) * loc1) * loc1) * loc1) * loc1) * loc1) * loc1) * loc1) * loc1) * loc1) * loc1) * loc1) * loc1) * loc1) * loc1) * loc1) * loc1) * 3);
            loc5 = (loc5 + 1);
        }	/* end of while */
        loc3 = (loc3 + 1);
    }	/* end of while */
    printf ("a=%d\n", loc1);
}

