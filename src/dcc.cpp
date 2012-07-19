/*****************************************************************************
 *              dcc decompiler
 * Reads the command line switches and then executes each major section in turn
 * (C) Cristina Cifuentes
 ****************************************************************************/

#include <cstring>
#include "dcc.h"
#include "project.h"

/* Global variables - extern to other modules */
extern char    *asm1_name, *asm2_name;     /* Assembler output filenames     */
extern SYMTAB  symtab;             /* Global symbol table      			  */
extern STATS   stats;              /* cfg statistics       				  */
//PROG    prog;               /* programs fields      				  */
extern OPTION  option;             /* Command line options     			  */
//Function *   pProcList;			/* List of procedures, topologically sort */
//Function *	pLastProc;			/* Pointer to last node in procedure list */
//FunctionListType pProcList;
//CALL_GRAPH	*callGraph;		/* Call graph of the program			  */

static char *initargs(int argc, char *argv[]);
static void displayTotalStats(void);
#include <llvm/Support/raw_os_ostream.h>
#include <llvm/Support/CommandLine.h>
#include <llvm/Support/TargetSelect.h>
#include <llvm/Support/TargetRegistry.h>
#include <llvm/Support/PrettyStackTrace.h>
#include <llvm/Support/Signals.h>
#include <llvm/Support/Host.h>
#include <llvm/Target/TargetMachine.h>
#include <llvm/Target/TargetInstrInfo.h>
#include <llvm/MC/MCAsmInfo.h>
#include <llvm/CodeGen/MachineInstrBuilder.h>

#include <llvm/TableGen/Main.h>
#include <llvm/TableGen/TableGenAction.h>
#include <llvm/TableGen/Record.h>
/****************************************************************************
 * main
 ***************************************************************************/
#include <iostream>
using namespace llvm;
class TVisitor : public TableGenAction {
public:
    virtual bool operator()(raw_ostream &OS, RecordKeeper &Records)
    {
        Record *rec = Records.getDef("ADD8i8");
        if(rec)
        {
            if(not rec->getTemplateArgs().empty())
                std::cout << "Has template args\n";
            auto classes(rec->getSuperClasses());
            for(auto val : rec->getSuperClasses())
                std::cout << "Super "<<val->getName()<<"\n";

//          DagInit * in = rec->getValueAsDag(val.getName());
//          in->dump();
            for(const RecordVal &val : rec->getValues())
            {
//                val.dump();
            }
            rec->dump();

        }
        //        rec = Records.getDef("CCR");
        //        if(rec)
        //            rec->dump();
        for(auto val : Records.getDefs())
        {
            //std::cout<< "Def "<<val.first<<"\n";
        }
        return false;
    }
};
int testTblGen(int argc, char **argv)
{
    using namespace llvm;
    sys::PrintStackTraceOnErrorSignal();
    PrettyStackTraceProgram(argc,argv);
    cl::ParseCommandLineOptions(argc,argv);
    TVisitor tz;

    return llvm::TableGenMain(argv[0],tz);
    InitializeNativeTarget();
    Triple TheTriple;
    std::string  def = sys::getDefaultTargetTriple();
    std::string MCPU="i386";
    std::string MARCH="x86";
    InitializeAllTargetInfos();
    InitializeAllTargetMCs();
    InitializeAllAsmPrinters();
    InitializeAllAsmParsers();
    InitializeAllDisassemblers();
    std::string TargetTriple("i386-pc-linux-gnu");
    TheTriple = Triple(Triple::normalize(TargetTriple));
    MCOperand op=llvm::MCOperand::CreateImm(11);
    MCAsmInfo info;
    raw_os_ostream wrap(std::cerr);
    op.print(wrap,&info);
    wrap.flush();
    std::cerr<<"\n";
    std::string lookuperr;
    TargetRegistry::printRegisteredTargetsForVersion();
    const Target *t = TargetRegistry::lookupTarget(MARCH,TheTriple,lookuperr);
    TargetOptions opts;
    std::string Features;
    opts.PrintMachineCode=1;
    TargetMachine *tm = t->createTargetMachine(TheTriple.getTriple(),MCPU,Features,opts);
    std::cerr<<tm->getInstrInfo()->getName(97)<<"\n";
    const MCInstrDesc &ds(tm->getInstrInfo()->get(97));
    const MCOperandInfo *op1=ds.OpInfo;
    uint16_t impl_def = ds.getImplicitDefs()[0];
    std::cerr<<lookuperr<<"\n";

    exit(0);

}
int main(int argc, char **argv)
{
    /* Extract switches and filename */
    strcpy(option.filename, initargs(argc, argv));

    /* Front end reads in EXE or COM file, parses it into I-code while
     * building the call graph and attaching appropriate bits of code for
     * each procedure.
    */
    DccFrontend fe(option.filename);
    if(false==fe.FrontEnd ())
        return -1;
    if(option.asm1)
        return 0;
    /* In the middle is a so called Universal Decompiling Machine.
     * It processes the procedure list and I-code and attaches where it can
     * to each procedure an optimised cfg and ud lists
    */
    udm();
    if(option.asm2)
        return 0;

    /* Back end converts each procedure into C using I-code, interval
     * analysis, data flow etc. and outputs it to output file ready for
     * re-compilation.
    */
    BackEnd(asm1_name ? asm1_name:option.filename, Project::get()->callGraph);

    Project::get()->callGraph->write();

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
    printf ("  Total number of low-level Icodes : %d\n", stats.totalLL);
    printf ("  Total number of high-level Icodes: %d\n", stats.totalHL);
    printf ("  Total reduction of instructions  : %2.2f%%\n", 100.0 -
            (stats.totalHL * 100.0) / stats.totalLL);
}




