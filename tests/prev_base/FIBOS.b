/*
 * Input file	: ./tests/inputs/FIBOS.EXE
 * File type	: EXE
 */

#include "dcc.h"


int proc_1 (int arg0)
/* Takes 2 bytes of parameters.
 * High-level language prologue code.
 * C calling convention.
 */
{
int loc1; /* si */
int loc2; /* ax */

    loc1 = arg0;

    if (loc1 > 2) {
        loc2 = (proc_1 ((loc1 - 1)) + proc_1 ((loc1 + 0xfffe)));
    }
    else {
        loc2 = 1;
    }
    return (loc2);
}


void main ()
/* Takes no parameters.
 * High-level language prologue code.
 */
{
int loc1;
int loc2;
int loc3;
int loc4;

    printf ("Input number of iterations: ");
    scanf ("%d", &loc1);
    loc3 = 1;

    while ((loc3 <= loc1)) {
        printf ("Input number: ");
        scanf ("%d", &loc2);
        printf ("fibonacci(%d) = %u\n", loc2, proc_1 (loc2));
        loc3 = (loc3 + 1);
    }	/* end of while */
    exit (0);
}

