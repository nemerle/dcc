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
#include <QFileInfo>

#include <QtCore/QFile>


/* Global variables - extern to other modules */
extern QString asm1_name, asm2_name;     /* Assembler output filenames     */
extern SYMTAB  symtab;             /* Global symbol table      			  */
extern STATS   stats;              /* cfg statistics       				  */
extern OPTION  option;             /* Command line options     			  */

static void displayTotalStats();
/****************************************************************************
 * main
 ***************************************************************************/
void setupOptions(const QCoreApplication &app) {
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
    option.CustomEntryPoint = parser.value(entryPointOption).toUInt(nullptr,16);
    if(parser.isSet(targetFileOption)) {
        asm1_name = asm2_name = parser.value(targetFileOption);
    }
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

    if(!asm1_name.isEmpty())
        Project::get()->set_output_path(QFileInfo(asm1_name).path());

    DccFrontend fe(&app);
    if(not Project::get()->load()) {
        return -1;
    }
    if (option.verbose)
        Project::get()->prog.displayLoadInfo();
    if(not fe.FrontEnd ())
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
