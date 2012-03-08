/*****************************************************************************
 * Project: 	dcc
 * File:	backend.c
 * Purpose:	Back-end module.  Generates C code for each procedure.
 * (C) Cristina Cifuentes
 ****************************************************************************/
#include <cassert>
#include <string>

#include "dcc.h"
#include <fstream>
#include <sstream>
#include <string.h>
#include <stdio.h>

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
static void printGlobVar (SYM * psym)
{
    int j;
    uint32_t relocOp = prog.fCOM ? psym->label : psym->label + 0x100;

    switch (psym->size)
    {
        case 1: cCode.appendDecl( "uint8_t\t%s = %ld;\n",
                                  psym->name, prog.Image[relocOp]);
            break;
        case 2: cCode.appendDecl( "uint16_t\t%s = %ld;\n",
                                  psym->name, LH(prog.Image+relocOp));
            break;
        case 4: if (psym->type == TYPE_PTR)  /* pointer */
                cCode.appendDecl( "uint16_t\t*%s = %ld;\n",
                                  psym->name, LH(prog.Image+relocOp));
            else 			/* char */
                cCode.appendDecl(
                            "char\t%s[4] = \"%c%c%c%c\";\n",
                            psym->name, prog.Image[relocOp],
                            prog.Image[relocOp+1], prog.Image[relocOp+2],
                            prog.Image[relocOp+3]);
            break;
        default:
        {
            ostringstream strContents;
            for (j=0; j < psym->size; j++)
                strContents << cChar(prog.Image[relocOp + j]);
            cCode.appendDecl( "char\t*%s = \"%s\";\n", psym->name, strContents.str().c_str());
        }
    }
}


// Note: Not called at present.
/* Writes the contents of the symbol table, along with any variable
 * initialization. */
static void writeGlobSymTable()
{
    char type[10];

    if (not symtab.empty())
    {
        cCode.appendDecl( "/* Global variables */\n");
#ifdef _lint
        for (auto iter=symtab.begin(); iter!=symtab.end(); ++iter)
        {
            SYM &sym(*iter);
#else
        for (SYM &sym : symtab)
        {
#endif
//            pSym = &symtab[idx];
            if (sym.duVal.isUSE_VAL())	/* first used */
                printGlobVar (&sym);
            else {					/* first defined */
                switch (sym.size) {
                    case 1:  strcpy (type, "uint8_t\t"); break;
                    case 2:  strcpy (type, "int\t"); break;
                    case 4:  if (sym.type == TYPE_PTR)
                            strcpy (type, "int\t*");
                        else
                            strcpy (type, "char\t*");
                        break;
                    default: strcpy (type, "char\t*");
                }
                cCode.appendDecl( "%s%s;\t/* size = %ld */\n",
                                  type, sym.name, sym.size);
            }
        }
        cCode.appendDecl( "\n");
    }
}


/* Writes the header information and global variables to the output C file
 * fp. */
static void writeHeader (std::ostream &_ios, char *fileName)
{
    /* Write header information */
    newBundle (&cCode);
    cCode.appendDecl( "/*\n");
    cCode.appendDecl( " * Input file\t: %s\n", fileName);
    cCode.appendDecl( " * File type\t: %s\n", (prog.fCOM)?"COM":"EXE");
    cCode.appendDecl( " */\n\n#include \"dcc.h\"\n\n");

    /* Write global symbol table */
    /** writeGlobSymTable(); *** need to change them into locident fmt ***/
    writeBundle (_ios, cCode);
    freeBundle (&cCode);
}


/* Writes the registers that are set in the bitvector */
static void writeBitVector (const std::bitset<32> &regi)
{
    int j;

    for (j = rAX; j < INDEX_BX_SI; j++)
    {
        if (regi.test(j))
            printf ("%s ", allRegs[j-1]);
    }
}

// Note: Not currently called!
/* Checks the given icode to determine whether it has a label associated
 * to it.  If so, a goto is emitted to this label; otherwise, a new label
 * is created and a goto is also emitted.
 * Note: this procedure is to be used when the label is to be forward on
 *		 the code; that is, the target code has not been traversed yet. */
static void emitFwdGotoLabel (ICODE * pt, int indLevel)
{
    if ( ! pt->ll()->testFlags(HLL_LABEL) ) /* node hasn't got a lab */
    {
        /* Generate new label */
        pt->ll()->hllLabNum = getNextLabel();
        pt->ll()->setFlags(HLL_LABEL);
    }
    cCode.appendCode( "%sgoto l%ld;\n", indentStr(indLevel), pt->ll()->hllLabNum);
}


/* Writes the procedure's declaration (including arguments), local variables,
 * and invokes the procedure that writes the code of the given record *hli */
void Function::codeGen (std::ostream &fs)
{
    int numLoc;
    char buf[200],        /* Procedure's definition           */
         arg[30];         /* One argument                     */
    BB *pBB;              /* Pointer to basic block           */

    /* Write procedure/function header */
    newBundle (&cCode);
    if (flg & PROC_IS_FUNC)      /* Function */
        cCode.appendDecl( "\n%s %s (", hlTypes[retVal.type],name.c_str());
    else                                /* Procedure */
        cCode.appendDecl( "\nvoid %s (", name.c_str());

    /* Write arguments */
    memset (buf, 0, sizeof(buf));
    for (size_t i = 0; i < args.sym.size(); i++)
    {
        if (args.sym[i].invalid == FALSE)
        {
            sprintf (arg,"%s %s",hlTypes[args.sym[i].type], args.sym[i].name);
            strcat (buf, arg);
            if (i < (args.sym.size() - 1))
                strcat (buf, ", ");
        }
    }
    strcat (buf, ")\n");
    cCode.appendDecl( "%s", buf);

    /* Write comments */
    writeProcComments();

    /* Write local variables */
    if (! (flg & PROC_ASM))
    {
        numLoc = 0;
#ifdef _lint
        for (size_t i = 0; i < localId.csym(); i++)
        {
            ID &refId(localId.id_arr[i]);
#else
        for (ID &refId : localId )
        {
#endif
            /* Output only non-invalidated entries */
            if (refId.illegal == FALSE)
            {
                if (refId.loc == REG_FRAME)
                {
                    /* Register variables are assigned to a local variable */
                    if (((flg & SI_REGVAR) && (refId.id.regi == rSI)) ||
                        ((flg & DI_REGVAR) && (refId.id.regi == rDI)))
                    {
                        sprintf (refId.name, "loc%ld", ++numLoc);
                        cCode.appendDecl( "int %s;\n", refId.name);
                    }
                    /* Other registers are named when they are first used in
                     * the output C code, and appended to the proc decl. */
                }

                else if (refId.loc == STK_FRAME)
                {
                    /* Name local variables and output appropriate type */
                    sprintf (refId.name, "loc%ld", ++numLoc);
                    cCode.appendDecl( "%s %s;\n",hlTypes[refId.type], refId.name);
                }
            }
        }
    }
    /* Write procedure's code */
    if (flg & PROC_ASM)		/* generate assembler */
        disassem (3, this);
    else							/* generate C */
        m_cfg.front()->writeCode (1, this, &numLoc, MAX, UN_INIT);

    cCode.appendCode( "}\n\n");
    writeBundle (fs, cCode);
    freeBundle (&cCode);

    /* Write Live register analysis information */
    if (option.verbose)
        for (size_t i = 0; i < numBBs; i++)
        {
            pBB = m_dfsLast[i];
            if (pBB->flg & INVALID_BB)	continue;	/* skip invalid BBs */
            printf ("BB %d\n", i);
            printf ("  Start = %d, end = %d\n", pBB->begin()->loc_ip, pBB->begin()->loc_ip+pBB->size());
            printf ("  LiveUse = ");
            writeBitVector (pBB->liveUse);
            printf ("\n  Def = ");
            writeBitVector (pBB->def);
            printf ("\n  LiveOut = ");
            writeBitVector (pBB->liveOut);
            printf ("\n  LiveIn = ");
            writeBitVector (pBB->liveIn);
            printf ("\n\n");
        }
}


/* Recursive procedure. Displays the procedure's code in depth-first order
 * of the call graph.	*/
static void backBackEnd (char *filename, CALL_GRAPH * pcallGraph, std::ostream &_ios)
{
    //	IFace.Yield();			/* This is a good place to yield to other apps */

    /* Check if this procedure has been processed already */
    if ((pcallGraph->proc->flg & PROC_OUTPUT) ||
        (pcallGraph->proc->flg & PROC_ISLIB))
        return;
    pcallGraph->proc->flg |= PROC_OUTPUT;

    /* Dfs if this procedure has any successors */
    for (size_t i = 0; i < pcallGraph->outEdges.size(); i++)
    {
        backBackEnd (filename, pcallGraph->outEdges[i], _ios);
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
void BackEnd (char *fileName, CALL_GRAPH * pcallGraph)
{
    std::ofstream fs; /* Output C file 	*/

    /* Get output file name */
    std::string outNam(fileName);
    outNam = outNam.substr(0,outNam.rfind("."))+".b"; /* b for beta */

    /* Open output file */
    fs.open(outNam);
    if(!fs.is_open())
        fatalError (CANNOT_OPEN, outNam.c_str());
    printf ("dcc: Writing C beta file %s\n", outNam.c_str());

    /* Header information */
    writeHeader (fs, fileName);

    /* Initialize total Icode instructions statistics */
    stats.totalLL = 0;
    stats.totalHL = 0;

    /* Process each procedure at a time */
    backBackEnd (fileName, pcallGraph, fs);

    /* Close output file */
    fs.close();
    printf ("dcc: Finished writing C beta file\n");
}


