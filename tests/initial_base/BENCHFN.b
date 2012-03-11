/*
 * Input file	: test\benchfn.exe
 * File type	: EXE
 */

#include "dcc.h"


void proc_4 ()
/* Takes no parameters.
 * High-level language prologue code.
 */
{
}


void proc_3 ()
/* Takes no parameters.
 * High-level language prologue code.
 */
{
    proc_4 ();
    proc_4 ();
    proc_4 ();
    proc_4 ();
    proc_4 ();
    proc_4 ();
    proc_4 ();
    proc_4 ();
    proc_4 ();
    proc_4 ();
}


void proc_2 ()
/* Takes no parameters.
 * High-level language prologue code.
 */
{
    proc_3 ();
    proc_3 ();
    proc_3 ();
    proc_3 ();
    proc_3 ();
    proc_3 ();
    proc_3 ();
    proc_3 ();
    proc_3 ();
    proc_3 ();
}


void proc_1 ()
/* Takes no parameters.
 * High-level language prologue code.
 */
{
    proc_2 ();
    proc_2 ();
    proc_2 ();
    proc_2 ();
    proc_2 ();
    proc_2 ();
    proc_2 ();
    proc_2 ();
    proc_2 ();
}


void main ()
/* Takes no parameters.
 * High-level language prologue code.
 */
{
long loc1;
long loc2;

    printf ("enter number of iterations ");
    scanf ("%ld", &loc0);
    printf ("executing %ld iterations\n", loc2);
    loc1 = 1;
    while ((loc1 <= loc2)) {
        proc_1 ();
        loc1 = (loc1 + 1);
    }
    printf ("finished\n");
}

