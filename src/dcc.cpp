/*****************************************************************************
 *              dcc decompiler
 * Reads the command line switches and then executes each major section in turn
 * (C) Cristina Cifuentes
 ****************************************************************************/

#include "dcc.h"

#include "msvc_fixes.h"
#include "project.h"
#include "CallGraph.h"
#include "DccFrontend.h"

#include <cstring>
#include <iostream>
#include <QtCore/QCoreApplication>
#include <QCommandLineParser>

#ifdef LLVM_EXPERIMENTAL
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
#include <llvm/TableGen/TableGenBackend.h>
#include <llvm/TableGen/Record.h>
#endif
#include <QtCore/QFile>


/* Global variables - extern to other modules */
extern QString asm1_name, asm2_name;     /* Assembler output filenames     */
extern SYMTAB  symtab;             /* Global symbol table      			  */
extern STATS   stats;              /* cfg statistics       				  */
extern OPTION  option;             /* Command line options     			  */

static char *initargs(int argc, char *argv[]);
static void displayTotalStats(void);
/****************************************************************************
 * main
 ***************************************************************************/
#ifdef LLVM_EXPERIMENTAL
using namespace llvm;
bool TVisitor(raw_ostream &OS, RecordKeeper &Records)
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
//    for(auto val : Records.getDefs())
//    {
//        //std::cout<< "Def "<<val.first<<"\n";
//    }
    return false;
}
int testTblGen(int argc, char **argv)
{
//    using namespace llvm;
//    sys::PrintStackTraceOnErrorSignal();
//    PrettyStackTraceProgram(argc,argv);
//    cl::ParseCommandLineOptions(argc,argv);
//    return llvm::TableGenMain(argv[0],TVisitor);
//    InitializeNativeTarget();
//    Triple TheTriple;
//    std::string  def = sys::getDefaultTargetTriple();
//    std::string MCPU="i386";
//    std::string MARCH="x86";
//    InitializeAllTargetInfos();
//    InitializeAllTargetMCs();
//    InitializeAllAsmPrinters();
//    InitializeAllAsmParsers();
//    InitializeAllDisassemblers();
//    std::string TargetTriple("i386-pc-linux-gnu");
//    TheTriple = Triple(Triple::normalize(TargetTriple));
//    MCOperand op=llvm::MCOperand::CreateImm(11);
//    MCAsmInfo info;
//    raw_os_ostream wrap(std::cerr);
//    op.print(wrap,&info);
//    wrap.flush();
//    std::cerr<<"\n";
//    std::string lookuperr;
//    TargetRegistry::printRegisteredTargetsForVersion();
//    const Target *t = TargetRegistry::lookupTarget(MARCH,TheTriple,lookuperr);
//    TargetOptions opts;
//    std::string Features;
//    opts.PrintMachineCode=1;
//    TargetMachine *tm = t->createTargetMachine(TheTriple.getTriple(),MCPU,Features,opts);
//    std::cerr<<tm->getInstrInfo()->getName(97)<<"\n";
//    const MCInstrDesc &ds(tm->getInstrInfo()->get(97));
//    const MCOperandInfo *op1=ds.OpInfo;
//    uint16_t impl_def = ds.getImplicitDefs()[0];
//    std::cerr<<lookuperr<<"\n";

//    exit(0);

}
#endif
void setupOptions(QCoreApplication &app) {
    //[-a1a2cmsi]
    QCommandLineParser parser;
    parser.setApplicationDescription("dcc");
    parser.addHelpOption();
    //parser.addVersionOption();
    //QCommandLineOption showProgressOption("p", QCoreApplication::translate("main", "Show progress during copy"));
    QCommandLineOption boolOpts[] {
        QCommandLineOption {"v", QCoreApplication::translate("main", "verbose")},
        QCommandLineOption {"V", QCoreApplication::translate("main", "very verbose")},
        QCommandLineOption {"c", QCoreApplication::translate("main", "Follow register indirect calls")},
        QCommandLineOption {"m", QCoreApplication::translate("main", "Print memory maps of program")},
        QCommandLineOption {"s", QCoreApplication::translate("main", "Print stats")}
    };
    for(QCommandLineOption &o : boolOpts) {
        parser.addOption(o);
    }
    QCommandLineOption assembly("a", QCoreApplication::translate("main", "Produce assembly"),"assembly_level");
    QCommandLineOption targetFileOption(QStringList() << "o" << "output",
                                        QCoreApplication::translate("main", "Place output into <file>."),
                                        QCoreApplication::translate("main", "file"));
    QCommandLineOption entryPointOption(QStringList() << "E",
                                        QCoreApplication::translate("main", "Custom entry point as hex"),
                                        QCoreApplication::translate("main", "offset"),
                                        "0"
                                        );
    parser.addOption(targetFileOption);
    parser.addOption(assembly);
    parser.addOption(entryPointOption);
    //parser.addOption(forceOption);
    // Process the actual command line arguments given by the user
    parser.addPositionalArgument("source", QCoreApplication::translate("main", "Dos Executable file to decompile."));
    parser.process(app);

    const QStringList args = parser.positionalArguments();
    if(args.empty()) {
        parser.showHelp();
    }
    // source is args.at(0), destination is args.at(1)
    option.verbose = parser.isSet(boolOpts[0]);
    option.VeryVerbose = parser.isSet(boolOpts[1]);
    if(parser.isSet(assembly)) {
        option.asm1 = parser.value(assembly).toInt()==1;
        option.asm2 = parser.value(assembly).toInt()==2;
    }
    option.Map = parser.isSet(boolOpts[3]);
    option.Stats = parser.isSet(boolOpts[4]);
    option.Interact = false;
    option.Calls = parser.isSet(boolOpts[2]);
    option.filename = args.first();
    option.CustomEntryPoint = parser.value(entryPointOption).toUInt(0,16);
    if(parser.isSet(targetFileOption))
        asm1_name = asm2_name = parser.value(targetFileOption);
    else if(option.asm1 or option.asm2) {
        asm1_name = option.filename+".a1";
        asm2_name = option.filename+".a2";
    }

}
int main(int argc, char **argv)
{
    QCoreApplication app(argc,argv);

    QCoreApplication::setApplicationVersion("0.1");
    setupOptions(app);

    /* Front end reads in EXE or COM file, parses it into I-code while
     * building the call graph and attaching appropriate bits of code for
     * each procedure.
    */
    Project::get()->create(option.filename);

    DccFrontend fe(&app);
    if(not Project::get()->load()) {
        return -1;
    }
    if (option.verbose)
        Project::get()->prog.displayLoadInfo();
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
    BackEnd(Project::get()->callGraph);

    Project::get()->callGraph->write();

    if (option.Stats)
        displayTotalStats();

    return 0;
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




