/*
 * Input file	: ./tests/inputs/MAX.EXE
 * File type	: EXE
 */

#include "dcc.h"


int proc_1 (int arg0, int arg1)
/* Takes 4 bytes of parameters.
 * High-level language prologue code.
 * C calling convention.
 */
{
int loc1; /* ax */

    if (arg0 > arg1) {
        loc1 = arg0;
    }
    else {
        loc1 = arg1;
    }
    return (loc1);
}


void main ()
/* Takes no parameters.
 * High-level language prologue code.
 */
{
int loc1;
int loc2;
    printf ("Enter 2 numbers: ");
    scanf ("%d %d", &loc2, &loc1);

    if (loc2 != loc1) {
        printf ("Maximum: %d\n", proc_1 (loc2, loc1));
    }
}

