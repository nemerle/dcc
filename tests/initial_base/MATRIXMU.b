/*
 * Input file	: test\matrixmu.exe
 * File type	: EXE
 */

#include "dcc.h"


void proc_1 (int arg0, int arg1, int arg2)
/* Takes 6 bytes of parameters.
 * High-level language prologue code.
 * C calling convention.
 */
{
int loc1;
int loc2;
int loc3;

    loc2 = 0;
    while ((loc2 < 5)) {
        loc3 = 0;
        while ((loc3 < 4)) {
            loc1 = 0;
            while ((loc1 < 4)) {
                *((((loc2 * 10) + arg2) + (loc3 << 1))) = ((*((((loc2 << 3) + arg0) + (loc1 << 1))) * *((((loc1 * 10) + arg1) + (loc3 << 1)))) + *((((loc2 * 10) + arg2) + (loc3 << 1))));
                loc1 = (loc1 + 1);
            }
            loc3 = (loc3 + 1);
        }
        loc2 = (loc2 + 1);
    }
}


void main ()
/* Takes no parameters.
 * High-level language prologue code.
 */
{
int loc1;
int loc2;
int loc3;

    proc_1 (&loc3, &loc2, &loc1);
}

