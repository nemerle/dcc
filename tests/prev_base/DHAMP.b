/*
 * Input file	: ./tests/inputs/DHAMP.EXE
 * File type	: EXE
 */

#include "dcc.h"


int proc_2 (long arg0, long arg1)
/* Takes 8 bytes of parameters.
 * High-level language prologue code.
 * C calling convention.
 */
{
char loc1; /* al */
int loc2; /* bx */

    do {
        arg0 = (arg0 + 1);
        loc1 = es[bx];
        arg1 = (arg1 + 1);
        es[bx] = loc1;
    } while ((loc1 != 0));
    return (loc2);
}


int proc_3 (long arg0, long arg1)
/* Takes 8 bytes of parameters.
 * High-level language prologue code.
 * C calling convention.
 */
{
int loc1; /* ax */

    while ((es[bx] == es[bx])) {
        if (es[bx] == 0) {
            loc1 = 0;
            return (loc1);
        }
        else {
            arg0 = (arg0 + 1);
            arg1 = (arg1 + 1);
        }
    }	/* end of while */
    loc1 = (es[bx] - es[bx]);
}


int proc_1 (int arg0, int arg1, int arg2, int arg3)
/* Takes 8 bytes of parameters.
 * High-level language prologue code.
 * C calling convention.
 */
{
int loc1; /* si */
int loc2; /* di */

    loc1 = 0;
    loc2 = 0;
    while ((loc1 < 0x2328)) {
        proc_2 (arg1, arg0, 311);
        proc_2 (arg3, arg2, 328);
        loc2 = (loc2 + proc_3 (arg1, arg0, arg3, arg2));
        loc1 = (loc1 + 1);
    }	/* end of while */
    return (loc2);
}


int proc_4 ()
/* Takes no parameters.
 * High-level language prologue code.
 */
{
int loc1;
int loc2;
int loc3;
int loc4;

    loc3 = 0;
    while ((loc3 < 0x3e8)) {
        loc1 = 0;
        loc4 = 0;
        loc2 = 1;
        while ((loc4 < 179)) {
            loc1 = (loc1 + loc2);
            loc2 = (loc2 + 2);
            loc4 = (loc4 + 1);
        }	/* end of while */
        loc3 = (loc3 + 1);
    }	/* end of while */
    return (loc1);
}


int proc_5 (int arg0)
/* Takes 2 bytes of parameters.
 * High-level language prologue code.
 * C calling convention.
 */
{
int loc1; /* si */
int loc2; /* ax */

    loc1 = arg0;
    if (loc1 > 2) {
        loc2 = (proc_5 ((loc1 - 1)) + proc_5 ((loc1 + 0xfffe)));
    }
    else {
        loc2 = 1;
    }
    return (loc2);
}


long proc_6 (int arg0, int arg1)
/* Takes 4 bytes of parameters.
 * High-level language prologue code.
 * C calling convention.
 */
{
long loc1;
int loc2; /* si */

    if ((arg0 | arg1) == 0) {
        loc2 = 0;

        while ((loc2 < 0x2328)) {
            loc1 = (loc1 + [23798]);
            [23798] = ([23798] + 2);
            loc2 = (loc2 + 1);
        }	/* end of while */
    }
    else {
        loc2 = 0;

        while ((loc2 < 0x2328)) {
            [23798] = ([23798] - 2);
            loc1 = (loc1 - [23798]);
            loc2 = (loc2 + 1);
        }	/* end of while */
    }
    return (loc1);
}


void proc_8 (int arg0)
/* Takes 8 bytes of parameters.
 * High-level language prologue code.
 * Untranslatable routine.  Assembler provided.
 * C calling convention.
 * Contains instructions not normally used by compilers.
 * Contains coprocessor instructions.
 */
{
                    PUSH           bp                 
                    MOV            bp, sp             
                    ESC            FLD qword ptr [126h]
                    ESC            FLD qword ptr [bp+6]
                    ESC            FCOMPP             
                    ESC            FSTSW qword ptr [62Ch]
                    INT            3Dh                	/* Unknown int */

                    MOV            ah, [62Dh]         
                    SAHF                              
                    JAE            L1                 
                    ESC            FLD qword ptr [bp+6]
                    ESC            FCHS               

               L2:  POP            bp                 
                    RETF                              

               L1:  ESC            FLD qword ptr [bp+6]
                    JMP            L2                 ;Synthetic inst
}


void proc_7 (int arg0, int arg1, int arg2, int arg3)
/* Takes 8 bytes of parameters.
 * High-level language prologue code.
 * Untranslatable routine.  Assembler provided.
 * C calling convention.
 * Contains instructions not normally used by compilers.
 * Contains coprocessor instructions.
 */
{
                    PUSH           bp                 
                    MOV            bp, sp             
                    SUB            sp, 10h            
                    ESC            FLD qword ptr [bp+6]
                    ESC            FDIV qword ptr [127h]
                    ESC            FSTP qword ptr [bp-8]
                    INT            3Dh                	/* Unknown int */

                    MOV            ax, [bp+0Ch]       
                    MOV            [bp-0Ah], ax       
                    MOV            ax, [bp+0Ah]       
                    MOV            [bp-0Ch], ax       
                    MOV            ax, [bp+8]         
                    MOV            [bp-0Eh], ax       
                    MOV            ax, [bp+6]         
                    MOV            [bp-10h], ax       

               L1:  ESC            FLD qword ptr [12Fh]
                    ESC            FMUL qword ptr [bp-8]
                    ESC            FCOMP qword ptr [bp-10h]
                    ESC            FSTSW qword ptr [62Ch]
                    INT            3Dh                	/* Unknown int */

                    MOV            ah, [62Dh]         
                    SAHF                              
                    JB             L2                 
                    ESC            FLD qword ptr [bp-8]
                    MOV            sp, bp             
                    POP            bp                 
                    RETF                              

               L2:  ESC            FLD qword ptr [bp+6]
                    ESC            FDIV qword ptr [bp-8]
                    ESC            FSUB qword ptr [bp-8]
                    ESC            FSTP qword ptr [bp-10h]
                    INT            3Dh                	/* Unknown int */

                    PUSH           word ptr [bp-0Ah]  
                    PUSH           word ptr [bp-0Ch]  
                    PUSH           word ptr [bp-0Eh]  
                    PUSH           word ptr [bp-10h]  
                    CALL            far ptr proc_8    
                    ADD            sp, 8              
                    ESC            FSTP qword ptr [bp-10h]
                    INT            3Dh                	/* Unknown int */

                    ESC            FLD qword ptr [bp+6]
                    ESC            FDIV qword ptr [bp-8]
                    ESC            FADD qword ptr [bp-8]
                    ESC            FDIV qword ptr [127h]
                    ESC            FSTP qword ptr [bp-8]
                    INT            3Dh                	/* Unknown int */

                    JMP            L1                 ;Synthetic inst
}


void proc_9 (int arg0)
/* Takes 8 bytes of parameters.
 * High-level language prologue code.
 * C calling convention.
 * Contains instructions not normally used by compilers.
 * Contains coprocessor instructions.
 */
{
int loc1;
int loc2;
int loc3; /* ax */

    loc2 = 100;
    loc3 = loc2;
    loc2 = (loc2 - 1);
    while (((loc3 | loc3) != 0)) {
        loc3 = loc2;
        loc2 = (loc2 - 1);
    }	/* end of while */
}


int proc_10 ()
/* Takes no parameters.
 * High-level language prologue code.
 * Contains instructions not normally used by compilers.
 */
{
int loc1;
int loc2;
int loc3;
int loc4;
int loc5;
int loc6; /* bx */
int loc7; /* dx */
int loc8; /* ax */
    loc6 = proc_2 (&loc1, 345, , );
    fopen ("zyxw.vut", 368);
    loc2 = loc7;
    loc3 = loc8;

    if ((loc7 | loc8) == 0) {
        printf ("Cannot open file");
        exit (0xffff);
    }
l1: 
    if (++loc5 >= 0x3e8) {
        fclose (loc3, loc2);
        return (loc5);
    }
    else {
        loc4 = 0;

        while ((ss[bp+si-0x64] != 0)) {

            if (++es[bx] < 0) {
                es[bx+0xc] = (es[bx+0xc] + 1);
                loc6 = (loc6 - 1);
                es[bx] = ss[bp+si-0x64];
            }
            else {
                _fputc (ss[bp+si-0x64], loc3, loc2);
            }
            loc4 = (loc4 + 1);
        }	/* end of while */
        goto L1;
    }
}


void main ()
/* Takes no parameters.
 * High-level language prologue code.
 * Contains instructions not normally used by compilers.
 * Contains coprocessor instructions.
 */
{
int loc1;
int loc2;
int loc3;
int loc4;
int loc5;
int loc6;
int loc7;
int loc8;
int loc9;
int loc10;
int loc11;
int loc12; /* ax */
int loc13; /* bx */

    loc11 = 0;
    printf ("Start...%c\n\n", 7);
    while ((loc11 < 6)) {
        loc12 = loc11;
        if (loc12 <= 5) {
            loc13 = (loc12 << 1);
            var06278 = proc_1 (&loc2, &loc1, , );
            printf ("\ncresult = %d\n", var06278);
        }
        loc11 = (loc11 + 1);
    }	/* end of while */
    printf ("\n\n...End%c", 7);
}

