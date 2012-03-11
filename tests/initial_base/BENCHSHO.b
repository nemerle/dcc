/*
 * Input file	: test\benchsho.exe
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
long loc4;
long loc5;
int loc6; /* ax */

    scanf ("%ld", &loc0);
    printf ("executing %ld iterations\n", loc5);
    scanf ("%ld", &loc1);
    scanf ("%ld", &loc2);
    loc4 = 1;
    while ((loc4 <= loc5)) {
        loc3 = 1;
        while ((loc3 <= 40)) {
            loc1 = ((loc1 + loc2) + loc3);
            loc2 = (loc1 >> 1);
            loc1 = (loc2 % 10);
            if (loc2 == loc3) {
                loc6 = 1;
            }
            else {
                loc6 = 0;
            }
            loc1 = loc6;
            loc2 = (loc1 | loc3);
            loc1 = !loc2;
            loc2 = (loc1 + loc3);
            if (loc2 > loc3) {
                loc6 = 1;
            }
            else {
                loc6 = 0;
            }
            loc1 = loc6;
            loc3 = (loc3 + 1);
        }
        loc4 = (loc4 + 1);
    }
    printf ("a=%d\n", loc1);
}

