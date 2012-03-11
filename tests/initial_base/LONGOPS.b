/*
 * Input file	: test\longops.exe
 * File type	: EXE
 */

#include "dcc.h"


long LXMUL@ (long arg0, long arg1)
/* Uses register arguments:
 *     arg0 = dx:ax.
 *     arg1 = cx:bx.
 * Runtime support routine of the compiler.
 */
{
int loc1;
int loc2; /* tmp */

    loc2 = LO(arg0);
    LO(arg0) = loc1;
    loc1 = loc2;
    loc2 = LO(arg0);
    LO(arg0) = HI(arg0);
    if ((LO(arg0) & LO(arg0)) != 0) {
        LO(arg0) = (LO(arg0) * LO(arg1));
    }
    loc2 = LO(arg0);
    LO(arg0) = HI(arg1);
    HI(arg1) = loc2;
    if ((LO(arg0) & LO(arg0)) != 0) {
        LO(arg0) = (LO(arg0) * loc1);
        HI(arg1) = (HI(arg1) + LO(arg0));
    }
    loc2 = LO(arg0);
    LO(arg0) = loc1;
    loc1 = loc2;
    arg0 = (LO(arg0) * LO(arg1));
    HI(arg0) = (HI(arg0) + HI(arg1));
    return (arg0);
}


long LDIV@ (long arg0, long arg2)
/* Takes 8 bytes of parameters.
 * Runtime support routine of the compiler.
 * Untranslatable routine.  Assembler provided.
 * Return value in registers dx:ax.
 * Pascal calling convention.
 */
{
        XOR            cx, 0
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


long LMOD@ (long arg0, long arg2)
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
        JNE            L11
        OR             dx, dx
        JE             L12
        OR             bx, bx
        JE             L12

L11:    TEST           di, 1
        JNE            L13
        OR             dx, dx
        JNS            L14
        NEG            dx
        NEG            ax
        SBB            dx, 0
        OR             di, 0Ch

L14:    OR             cx, cx
        JNS            L13
        NEG            cx
        NEG            bx
        SBB            cx, 0
        XOR            di, 4

L13:    MOV            bp, cx
        MOV            cx, 20h
        PUSH           di
        XOR            di, 0
        XOR            si, 0

L15:    SHL            ax, 1
        RCL            dx, 1
        RCL            si, 1
        RCL            di, 1
        CMP            di, bp
        JB             L16
        JA             L17
        CMP            si, bx
        JB             L16

L17:    SUB            si, bx
        SBB            di, bp
        INC            ax

L16:    LOOP           L15
        POP            bx
        TEST           bx, 2
        JE             L18
        MOV            ax, si
        MOV            dx, di
        SHR            bx, 1

L18:    TEST           bx, 4
        JE             L19
        NEG            dx
        NEG            ax
        SBB            dx, 0

L19:    POP            di
        POP            si
        POP            bp
        RETF           8

L12:    MOV            tmp, dx:ax                     ;Synthetic inst
        DIV            bx
        MOD            bx                             ;Synthetic inst
        TEST           di, 2
        JE             L20
        MOV            ax, dx

L20:    XOR            dx, dx
        JMP            L19
}


long LXLSH@ (long arg0, char arg1)
/* Uses register arguments:
 *     arg0 = dx:ax.
 *     arg1 = cl.
 * Runtime support routine of the compiler.
 */
{
int loc1; /* bx */

    if (arg1 < 16) {
        loc1 = LO(arg0);
        LO(arg0) = (LO(arg0) << arg1);
        HI(arg0) = (HI(arg0) << arg1);
        HI(arg0) = (HI(arg0) | (loc1 >> (!arg1 + 16)));
        return (arg0);
    }
    else {
        HI(arg0) = LO(arg0);
        LO(arg0) = 0;
        HI(arg0) = (HI(arg0) << (arg1 - 16));
        return (arg0);
    }
}


long LXRSH@ (long arg0, char arg1)
/* Uses register arguments:
 *     arg0 = dx:ax.
 *     arg1 = cl.
 * Runtime support routine of the compiler.
 */
{
int loc1; /* bx */

    if (arg1 < 16) {
        loc1 = HI(arg0);
        LO(arg0) = (LO(arg0) >> arg1);
        HI(arg0) = (HI(arg0) >> arg1);
        LO(arg0) = (LO(arg0) | (loc1 << (!arg1 + 16)));
        return (arg0);
    }
    else {
        arg0 = HI(arg0);
        LO(arg0) = (LO(arg0) >> (arg1 - 16));
        return (arg0);
    }
}


void main ()
/* Takes no parameters.
 * High-level language prologue code.
 */
{
long loc1;
long loc2;

    loc2 = 255;
    loc1 = 143;
    loc1 = (loc2 + loc1);
    loc2 = (loc2 - loc1);
    loc2 = LXMUL@ (loc2, loc1);
    loc1 = LDIV@ (loc1, loc2);
    loc1 = LMOD@ (loc1, loc2);
    loc2 = LXLSH@ (loc2, 5);
    loc1 = LXRSH@ (loc1, loc1);
    printf ("a = %ld, b = %ld\n", loc2, loc1);
}

