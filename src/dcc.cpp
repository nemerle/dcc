/*****************************************************************************
 *              dcc decompiler
 * Reads the command line switches and then executes each major section in turn
 * (C) Cristina Cifuentes
 ****************************************************************************/

#include "dcc.h"
#include "project.h"
#include <string.h>

/* Global variables - extern to other modules */
char    *asm1_name, *asm2_name;     /* Assembler output filenames     */
SYMTAB  symtab;             /* Global symbol table      			  */
STATS   stats;              /* cfg statistics       				  */
PROG    prog;               /* programs fields      				  */
OPTION  option;             /* Command line options     			  */
//Function *   pProcList;			/* List of procedures, topologically sort */
//Function *	pLastProc;			/* Pointer to last node in procedure list */
//FunctionListType pProcList;
//CALL_GRAPH	*callGraph;		/* Call graph of the program			  */

static char *initargs(int argc, char *argv[]);
static void displayTotalStats(void);
#include <llvm/Support/raw_os_ostream.h>

/****************************************************************************
 * main
 ***************************************************************************/
#include <iostream>
extern Project g_proj;
int main(int argc, char *argv[])
{
//     llvm::MCOperand op=llvm::MCOperand::CreateImm(11);
//     llvm::MCAsmInfo info;
//     llvm::raw_os_ostream wrap(std::cerr);
//     op.print(wrap,&info);
//     wrap.flush();
    /* Extract switches and filename */
    strcpy(option.filename, initargs(argc, argv));

    /* Front end reads in EXE or COM file, parses it into I-code while
     * building the call graph and attaching appropriate bits of code for
     * each procedure.
    */
    DccFrontend fe(option.filename);
    if(false==fe.FrontEnd ())
        return -1;

    /* In the middle is a so called Universal Decompiling Machine.
     * It processes the procedure list and I-code and attaches where it can
     * to each procedure an optimised cfg and ud lists
    */
    udm();

    /* Back end converts each procedure into C using I-code, interval
     * analysis, data flow etc. and outputs it to output file ready for
     * re-compilation.
    */
    BackEnd(option.filename, g_proj.callGraph);

    g_proj.callGraph->write();

    if (option.Stats)
        displayTotalStats();

    /*
    freeDataStructures(pProcList);
*/
    return 0;
}

/****************************************************************************
 * initargs - Extract command line arguments
 ***************************************************************************/
static char *initargs(int argc, char *argv[])
{
    char *pc;

    while (--argc > 0 && (*++argv)[0] == '-')
    {
        for (pc = argv[0]+1; *pc; pc++)
            switch (*pc)
            {
                case 'a':       /* Print assembler listing */
                    if (*(pc+1) == '2')
                        option.asm2 = true;
                    else
                        option.asm1 = true;
                    if (*(pc+1) == '1' || *(pc+1) == '2')
                        pc++;
                    break;
                case 'c':
                    option.Calls = true;
                    break;
                case 'i':
                    option.Interact = true;
                    break;
                case 'm':       /* Print memory map */
                    option.Map = true;
                    break;
                case 's':       /* Print Stats */
                    option.Stats = true;
                    break;
                case 'V':       /* Very verbose => verbose */
                    option.VeryVerbose = true;
                case 'v':
                    option.verbose = true; /* Make everything verbose */
                    break;
                case 'o':       /* assembler output file */
                    if (*(pc+1)) {
                        asm1_name = asm2_name = pc+1;
                        goto NextArg;
                    }
                    else if (--argc > 0) {
                        asm1_name = asm2_name = *++argv;
                        goto NextArg;
                    }
                default:
                    fatalError(INVALID_ARG, *pc);
                    return *argv;
            }
NextArg:;
    }

    if (argc == 1)
    {
        if (option.asm1 || option.asm2)
        {
            if (! asm1_name)
            {
                asm1_name = strcpy((char*)malloc(strlen(*argv)+4), *argv);
                pc = strrchr(asm1_name, '.');
                if (pc > strrchr(asm1_name, '/'))
                {
                    *pc = '\0';
                }
                asm2_name = (char*)malloc(strlen(asm1_name)+4) ;
                strcat(strcpy(asm2_name, asm1_name), ".a2");
                unlink(asm2_name);
                strcat(asm1_name, ".a1");
            }
            unlink(asm1_name);  /* Remove asm output files */
        }
        return *argv;       /* filename of the program to decompile */
    }

    fatalError(USAGE);
    return *argv; // does not reach this.
}

static void
displayTotalStats ()
/* Displays final statistics for the complete program */
{
    printf ("\nFinal Program Statistics\n");
    printf ("  Total number of low-level Icodes : %ld\n", stats.totalLL);
    printf ("  Total number of high-level Icodes: %ld\n", stats.totalHL);
    printf ("  Total reduction of instructions  : %2.2f%%\n", 100.0 -
            (stats.totalHL * 100.0) / stats.totalLL);
}




