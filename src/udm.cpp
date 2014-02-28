/*****************************************************************************
 *      dcc project Universal Decompilation Module
 *   This is supposedly a machine independant and language independant module
 *   that just plays with abstract cfg's and intervals and such like.
 * (C) Cristina Cifuentes
 ****************************************************************************/
#include <list>
#include <cassert>
#include <stdio.h>
#include "dcc.h"
#include "disassem.h"
#include "project.h"

extern Project g_proj;
//static void displayCFG(Function * pProc);
//static void displayDfs(BB * pBB);

/****************************************************************************
 * udm
 ****************************************************************************/
void Function::buildCFG(Disassembler &ds)
{
    if(flg & PROC_ISLIB)
        return; // Ignore library functions
    createCFG();
    if (option.VeryVerbose)
        displayCFG();

    compressCFG(); // Remove redundancies and add in-edge information

    if (option.asm2)
    {
        ds.disassem(this); // Print 2nd pass assembler listing
        return;
    }

    /* Idiom analysis and propagation of long type */
    lowLevelAnalysis();

    /* Generate HIGH_LEVEL icodes whenever possible */
    highLevelGen();
}
void Function::controlFlowAnalysis()
{
    if (flg & PROC_ISLIB)
        return;         /* Ignore library functions */
    derSeq *derivedG=nullptr;

    /* Make cfg reducible and build derived sequences */
    derivedG=checkReducibility();

    if (option.VeryVerbose)
        derivedG->display();

    /* Structure the graph */
    structure(derivedG);

    /* Check for compound conditions */
    compoundCond ();

    if (option.verbose)
    {
        printf("\nDepth first traversal - Proc %s\n", name.c_str());
        (*m_actual_cfg.begin())->displayDfs();
        //m_cfg.front()->displayDfs();
    }

    /* Free storage occupied by this procedure */
    freeDerivedSeq(*derivedG);

}
void udm(void)
{

    /* Build the control flow graph, find idioms, and convert low-level
     * icodes to high-level ones */
    Disassembler ds(2);
    for (auto iter = Project::get()->pProcList.rbegin(); iter!=Project::get()->pProcList.rend(); ++iter)
    {
        iter->buildCFG(ds);
    }
    if (option.asm2)
        return;


    /* Data flow analysis - eliminate condition codes, extraneous registers
     * and intermediate instructions.  Find expressions by forward
     * substitution algorithm */
    LivenessSet live_regs;
    Project::get()->pProcList.front().dataFlow (live_regs);

    /* Control flow analysis - structuring algorithm */
    for (auto iter = Project::get()->pProcList.rbegin(); iter!=Project::get()->pProcList.rend(); ++iter)
    {
        iter->controlFlowAnalysis();
    }
}

/****************************************************************************
 * displayCFG - Displays the Basic Block list
 ***************************************************************************/
void Function::displayCFG()
{
    printf("\nBasic Block List - Proc %s", name.c_str());
    for (BB *pBB : /*m_cfg*/m_actual_cfg)
    {
        pBB->display();
    }
}


