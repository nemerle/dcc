/*
 * Input file	: ./tests/inputs/STRLEN.EXE
 * File type	: EXE
 */

#include "dcc.h"


void proc_1 (int arg0)
/* Takes 2 bytes of parameters.
 * High-level language prologue code.
 * C calling convention.
 */
{
int loc1; /* si */
    loc1 = 0;
    arg0 = (arg0 + 1);

    while ((*arg0 != 0)) {
        loc1 = (loc1 + 1);
        arg0 = (arg0 + 1);
    }	/* end of while */
}


void main ()
/* Takes no parameters.
 */
{
int loc1;
    loc1 = 404;
    proc_1 (loc1);
}

