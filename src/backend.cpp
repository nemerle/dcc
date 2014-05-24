/*****************************************************************************
 * Project: 	dcc
 * File:	backend.c
 * Purpose:	Back-end module.  Generates C code for each procedure.
 * (C) Cristina Cifuentes
 ****************************************************************************/
#include <QDir>
#include <QFile>
#include <cassert>
#include <string>
#include <boost/range.hpp>
#include <boost/range/adaptors.hpp>
#include <boost/range/algorithm.hpp>

#include <fstream>
#include <iostream>
#include <sstream>
#include <string.h>
#include <stdio.h>
#include "dcc.h"
#include "disassem.h"
#include "project.h"
#include "CallGraph.h"
using namespace boost;
using namespace boost::adaptors;
bundle cCode;			/* Procedure declaration and code */
using namespace std;

/* Returns a unique index to the next label */
int getNextLabel()
{
    static int labelIdx = 1;	/* index of the next label		*/
    return (labelIdx++);
}


/* displays statistics on the subroutine */
void Function::displayStats ()
{
    printf("\nStatistics - Subroutine %s\n", name.c_str());
    printf ("Number of Icode instructions:\n");
    printf ("  Low-level : %4d\n", stats.numLLIcode);
    if (! (flg & PROC_ASM))
    {
        printf ("  High-level: %4d\n", stats.numHLIcode);
        printf ("  Percentage reduction: %2.2f%%\n", 100.0 - (stats.numHLIcode *
                                                              100.0) / stats.numLLIcode);
    }
}


/**** this proc is not required any more?? ****/
#if 0
static void fixupLabels (PPROC pProc)
/* Checks the graph (pProc->cfg) for any nodes that have labels, and gives
 * a unique label number for it.  This label is placed in the associated
 * icode for the node (pProc->Icode).  The procedure is done in sequential
 * order of dsfLast numbering.	*/
{ int i;				/* index into the dfsLast array */
    PBB *dfsLast;			/* pointer to the dfsLast array */

    dfsLast = pProc->dfsLast;
    for (i = 0; i < pProc->numBBs; i++)
        if (dfsLast[i]->flg/* & BB_HAS_LABEL*/) {
            pProc->Icode.icode[dfsLast[i]->start].ll()->flg |= HLL_LABEL;
            pProc->Icode.icode[dfsLast[i]->start].ll()->hllLabNum = getNextLabel();
        }
}
#endif


/* Returns the corresponding C string for the given character c.  Character
 * constants such as carriage return and line feed, require 2 C characters. */
char *cChar (uint8_t c)
{
    static char res[3];

    switch (c) {
        case 0x8:		/* backspace */
            sprintf (res, "\\b");
            break;
        case 0x9:		/* horizontal tab */
            sprintf (res, "\\t");
            break;
        case 0x0A:	/* new line */
            sprintf (res, "\\n");
            break;
        case 0x0C:	/* form feed */
            sprintf (res, "\\f");
            break;
        case 0x0D:	/* carriage return */
            sprintf (res, "\\r");
            break;
        default: 		/* any other character*/
            sprintf (res, "%c", c);
    }
    return (res);
}


/* Prints the variable's name and initial contents on the file.
 * Note: to get to the value of the variable:
 *		com file: prog.Image[operand]
 *		exe file: prog.Image[operand+0x100] 	*/
static void printGlobVar (std::ostream &ostr,SYM * psym)
{
    int j;
    PROG &prog(Project::get()->prog);
    uint32_t relocOp = prog.fCOM ? psym->label : psym->label + 0x100;

    switch (psym->size)
    {
        case 1:
            ostr << "uint8_t\t"<<psym->name<<" = "<<prog.image()[relocOp]<<";\n";
            break;
        case 2:
            ostr << "uint16_t\t"<<psym->name<<" = "<<LH(prog.image()+relocOp)<<";\n";
            break;
        case 4: if (psym->type == TYPE_PTR)  /* pointer */
                ostr << "uint16_t *\t"<<psym->name<<" = "<<LH(prog.image()+relocOp)<<";\n";
            else 			/* char */
                ostr << "char\t"<<psym->name<<"[4] = \""<<
                        prog.image()[relocOp]<<prog.image()[relocOp+1]<<
                        prog.image()[relocOp+2]<<prog.image()[relocOp+3]<<";\n";
            break;
        default:
        {
            ostringstream strContents;
            for (j=0; j < psym->size; j++)
                strContents << cChar(prog.image()[relocOp + j]);
            ostr << "char\t*"<<psym->name<<" = \""<<strContents.str()<<"\";\n";
        }
    }
}


// Note: Not called at present.
/* Writes the contents of the symbol table, along with any variable
 * initialization. */
void Project::writeGlobSymTable()
{
    std::ostringstream ostr;

    if (symtab.empty())
        return;
    ostr<<"/* Global variables */\n";
        for (SYM &sym : symtab)
        {
            if (sym.duVal.isUSE_VAL())	/* first used */
            printGlobVar (ostr,&sym);
            else {					/* first defined */
                switch (sym.size) {
                case 1:  ostr<<"uint8_t\t"; break;
                case 2:  ostr<<"int\t"; break;
                    case 4:  if (sym.type == TYPE_PTR)
                        ostr<<"int\t*";
                        else
                        ostr<<"char\t*";
                        break;
                default: ostr<<"char\t*";
                }
            ostr<<sym.name<<";\t/* size = "<<sym.size<<" */\n";
            }
        }
    ostr<< "\n";
    cCode.appendDecl( ostr.str() );
}


/* Writes the header information and global variables to the output C file
 * fp. */
static void writeHeader (std::ostream &_ios, const std::string &fileName)
{
    PROG &prog(Project::get()->prog);
    /* Write header information */
    cCode.init();
    cCode.appendDecl( "/*\n");
    cCode.appendDecl( " * Input file\t: %s\n", fileName.c_str());
    cCode.appendDecl( " * File type\t: %s\n", (prog.fCOM)?"COM":"EXE");
    cCode.appendDecl( " */\n\n#include \"dcc.h\"\n\n");

    /* Write global symbol table */
    /** writeGlobSymTable(); *** need to change them into locident fmt ***/
    writeBundle (_ios, cCode);
    freeBundle (&cCode);
}

// Note: Not currently called!
/** Checks the given icode to determine whether it has a label associated
 * to it.  If so, a goto is emitted to this label; otherwise, a new label
 * is created and a goto is also emitted.
 * Note: this procedure is to be used when the label is to be forward on
 *		 the code; that is, the target code has not been traversed yet. */
#if 0
static void emitFwdGotoLabel (ICODE * pt, int indLevel)
{
    if ( not pt->ll()->testFlags(HLL_LABEL)) /* node hasn't got a lab */
    {
        /* Generate new label */
        pt->ll()->hllLabNum = getNextLabel();
        pt->ll()->setFlags(HLL_LABEL);
    }
    cCode.appendCode( "%sgoto l%ld;\n", indentStr(indLevel), pt->ll()->hllLabNum);
}
#endif

/* Writes the procedure's declaration (including arguments), local variables,
 * and invokes the procedure that writes the code of the given record *hli */
void Function::codeGen (std::ostream &fs)
{
    int numLoc;
    ostringstream ostr;
    //STKFRAME * args;       /* Procedure arguments              */
    //char buf[200],        /* Procedure's definition           */
    //        arg[30];         /* One argument                     */
    BB *pBB;              /* Pointer to basic block           */

    /* Write procedure/function header */
    cCode.init();
    if (flg & PROC_IS_FUNC)      /* Function */
        ostr<< "\n"<<TypeContainer::typeName(retVal.type)<<" "<<name<<" (";
    else                                /* Procedure */
        ostr<< "\nvoid "<<name<<" (";

    /* Write arguments */
    struct validArg
    {
        bool operator()(STKSYM &s) { return s.invalid==false;}
    };
    auto valid_args = args | filtered(validArg());
    int count_valid = std::distance(valid_args.begin(),valid_args.end());
    for (STKSYM &arg : valid_args)
    {
        ostr<<hlTypes[arg.type]<<" "<<arg.name;
        if(--count_valid!=0)
            ostr<<", ";
    }
    ostr<<")\n";

    /* Write comments */
    writeProcComments( ostr );

    /* Write local variables */
    if (! (flg & PROC_ASM))
    {
        numLoc = 0;
        for (ID &refId : localId )
        {
            /* Output only non-invalidated entries */
            if ( refId.illegal )
                continue;
            if (refId.loc == REG_FRAME)
            {
                /* Register variables are assigned to a local variable */
                if (((flg & SI_REGVAR) && (refId.id.regi == rSI)) ||
                        ((flg & DI_REGVAR) && (refId.id.regi == rDI)))
                {
                    refId.setLocalName(++numLoc);
                    ostr << "int "<<refId.name<<";\n";
                }
                /* Other registers are named when they are first used in
                     * the output C code, and appended to the proc decl. */
            }
            else if (refId.loc == STK_FRAME)
            {
                /* Name local variables and output appropriate type */
                    refId.setLocalName(++numLoc);
                ostr << TypeContainer::typeName(refId.type)<<" "<<refId.name<<";\n";
            }
        }
    }
    fs<<ostr.str();
    /* Write procedure's code */
    if (flg & PROC_ASM)		/* generate assembler */
    {
        Disassembler ds(3);
        ds.disassem(this);
    }
    else							/* generate C */
    {
        m_actual_cfg.front()->writeCode (1, this, &numLoc, MAX, UN_INIT);
    }

    cCode.appendCode( "}\n\n");
    writeBundle (fs, cCode);
    freeBundle (&cCode);

    /* Write Live register analysis information */
    if (option.verbose)
        for (size_t i = 0; i < numBBs; i++)
        {
            pBB = m_dfsLast[i];
            if (pBB->flg & INVALID_BB)	continue;	/* skip invalid BBs */
            cout << "BB "<<i<<"\n";
            cout << "  Start = "<<pBB->begin()->loc_ip;
            cout << ", end = "<<pBB->begin()->loc_ip+pBB->size()<<"\n";
            cout << "  LiveUse = ";
            Machine_X86::writeRegVector(cout,pBB->liveUse);
            cout << "\n  Def = ";
            Machine_X86::writeRegVector(cout,pBB->def);
            cout << "\n  LiveOut = ";
            Machine_X86::writeRegVector(cout,pBB->liveOut);
            cout << "\n  LiveIn = ";
            Machine_X86::writeRegVector(cout,pBB->liveIn);
            cout <<"\n\n";
        }
}


/* Recursive procedure. Displays the procedure's code in depth-first order
 * of the call graph.	*/
static void backBackEnd (CALL_GRAPH * pcallGraph, std::ostream &_ios)
{

    //	IFace.Yield();			/* This is a good place to yield to other apps */

    /* Check if this procedure has been processed already */
    if ((pcallGraph->proc->flg & PROC_OUTPUT) ||
        (pcallGraph->proc->flg & PROC_ISLIB))
        return;
    pcallGraph->proc->flg |= PROC_OUTPUT;

    /* Dfs if this procedure has any successors */
    for (auto & elem : pcallGraph->outEdges)
    {
        backBackEnd (elem, _ios);
    }

    /* Generate code for this procedure */
    stats.numLLIcode = pcallGraph->proc->Icode.size();
    stats.numHLIcode = 0;
    pcallGraph->proc->codeGen (_ios);

    /* Generate statistics */
    if (option.Stats)
        pcallGraph->proc->displayStats ();
    if (! (pcallGraph->proc->flg & PROC_ASM))
    {
        stats.totalLL += stats.numLLIcode;
        stats.totalHL += stats.numHLIcode;
    }
}


/* Invokes the necessary routines to produce code one procedure at a time. */
void BackEnd(CALL_GRAPH * pcallGraph)
{
    std::ofstream fs; /* Output C file 	*/

    /* Get output file name */
    QString outNam(Project::get()->output_name("b")); /* b for beta */

    /* Open output file */
    fs.open(outNam.toStdString());
    if(!fs.is_open())
        fatalError (CANNOT_OPEN, outNam.toStdString().c_str());
    std::cout<<"dcc: Writing C beta file "<<outNam.toStdString()<<"\n";

    /* Header information */
    writeHeader (fs, option.filename.toStdString());

    /* Initialize total Icode instructions statistics */
    stats.totalLL = 0;
    stats.totalHL = 0;

    /* Process each procedure at a time */
    backBackEnd (pcallGraph, fs);

    /* Close output file */
    fs.close();
    std::cout << "dcc: Finished writing C beta file\n";
}


