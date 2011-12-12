/*
 * Input file	: ./tests/inputs/BENCHLNG.EXE
 * File type	: EXE
 */

#include "dcc.h"


long LMOD@ (long arg0, int arg2, int arg3)
/* Takes 8 bytes of parameters.
 * Runtime support routine of the compiler.
 * Untranslatable routine.  Assembler provided.
 * Return value in registers dx:ax.
 * Pascal calling convention.
 */
{
        MOV            cx, 2
        PUSH           bp
        PUSH           si
        PUSH           di
        MOV            bp, sp
        MOV            di, cx
        MOV            ax, [bp+0Ah]
        MOV            dx, [bp+0Ch]
        MOV            bx, [bp+0Eh]
        MOV            cx, [bp+10h]
        CMP            cx, 0
        JNE            L1
        OR             dx, dx
        JE             L2
        OR             bx, bx
        JE             L2

L1:     TEST           di, 1
        JNE            L3
        OR             dx, dx
        JNS            L4
        NEG            dx
        NEG            ax
        SBB            dx, 0
        OR             di, 0Ch

L4:     OR             cx, cx
        JNS            L3
        NEG            cx
        NEG            bx
        SBB            cx, 0
        XOR            di, 4

L3:     MOV            bp, cx
        MOV            cx, 20h
        PUSH           di
        XOR            di, 0
        XOR            si, 0

L5:     SHL            ax, 1
        RCL            dx, 1
        RCL            si, 1
        RCL            di, 1
        CMP            di, bp
        JB             L6
        JA             L7
        CMP            si, bx
        JB             L6

L7:     SUB            si, bx
        SBB            di, bp
        INC            ax

L6:     LOOP           L5
        POP            bx
        TEST           bx, 2
        JE             L8
        MOV            ax, si
        MOV            dx, di
        SHR            bx, 1

L8:     TEST           bx, 4
        JE             L9
        NEG            dx
        NEG            ax
        SBB            dx, 0

L9:     POP            di
        POP            si
        POP            bp
        RETF           8

L2:     MOV            tmp, dx:ax                     ;Synthetic inst
        DIV            bx
        MOD            bx                             ;Synthetic inst
        TEST           di, 2
        JE             L10
        MOV            ax, dx

L10:    XOR            dx, dx
        JMP            L9
}


void main ()
/* Takes no parameters.
 * High-level language prologue code.
 */
{
long loc1;
long loc2;
long loc3;
long loc4;
long loc5;
int loc6; /* ax */

    scanf ("%ld", &loc0);
    printf ("executing %ld iterations\n", loc5);
    scanf ("%ld", &loc2);
    scanf ("%ld", &loc3);
    loc3 = 1;

    while ((loc3 <= loc5)) {
        loc2 = 1;

        while ((loc2 <= 40)) {
            loc4 = ((loc4 + loc1) + loc2);
            loc1 = (loc4 >> 1);
            loc4 = LMOD@ (loc1, 10);

            if (loc1 == loc2) {
                loc6 = 1;
            }
            else {
                loc6 = 0;
            }
            loc4 = loc6;
            loc1 = (loc4 | loc2);

            if ((loc3 | loc9) == 0) {
                loc6 = 1;
            }
            else {
                loc6 = 0;
            }
            loc4 = loc6;
            loc1 = (loc4 + loc2);

            if (loc1 > loc2) {
                loc6 = 1;
            }
            else {
                loc6 = 0;
            }
            loc4 = loc6;
            loc2 = (loc2 + 1);
        }	/* end of while */
        loc3 = (loc3 + 1);
    }	/* end of while */
    printf ("a=%d\n", loc4);
}

