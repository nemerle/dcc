/*****************************************************************************
 *      dcc project Universal Decompilation Module
 *   This is supposedly a machine independant and language independant module
 *   that just plays with abstract cfg's and intervals and such like.
 * (C) Cristina Cifuentes
 ****************************************************************************/
#include "dcc.h"
#include "disassem.h"
#include "project.h"

#include <QtCore/QDebug>
#include <list>
#include <cassert>
#include <stdio.h>
#include <CallGraph.h>
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
        qDebug() <<"\nDepth first traversal - Proc" <<name;
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
    Project *proj = Project::get();
    Disassembler ds(2);
    for (auto iter = proj->pProcList.rbegin(); iter!=proj->pProcList.rend(); ++iter)
    {
        Function &f(*iter);
        if(option.CustomEntryPoint) {
            if(f.procEntry!=option.CustomEntryPoint) {
                continue;
            }
        }
        iter->buildCFG(ds);
    }
    if (option.asm2)
        return;


    /* Data flow analysis - eliminate condition codes, extraneous registers
     * and intermediate instructions.  Find expressions by forward
     * substitution algorithm */
    LivenessSet live_regs;
    if(option.CustomEntryPoint) {
        ilFunction iter = proj->findByEntry(option.CustomEntryPoint);
        if(iter==proj->pProcList.end()) {
            qCritical()<< "No function found at entry point" << QString::number(option.CustomEntryPoint,16);
            return;
        }
        iter->dataFlow(live_regs);
        iter->controlFlowAnalysis();
        delete proj->callGraph;
        proj->callGraph = new CALL_GRAPH;
        proj->callGraph->proc = iter;
        return;
    }
    proj->pProcList.front().dataFlow (live_regs);

    /* Control flow analysis - structuring algorithm */
    for (auto iter = proj->pProcList.rbegin(); iter!=proj->pProcList.rend(); ++iter)
    {
        iter->controlFlowAnalysis();
    }
}

/****************************************************************************
 * displayCFG - Displays the Basic Block list
 ***************************************************************************/
void Function::displayCFG()
{
    qDebug() << "\nBasic Block List - Proc"<<name;
    for (BB *pBB : /*m_cfg*/m_actual_cfg)
    {
        pBB->display();
    }
}


