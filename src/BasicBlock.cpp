#include "BasicBlock.h"
#include "Procedure.h"
#include "dcc.h"
BB *BB::Create(void *ctx, const std::string &s, Function *parent, BB *insertBefore)
{
    return new BB;
}

BB *BB::Create(Int start, Int ip, byte nodeType, Int numOutEdges, Function *parent)
{
    parent->cfg;
    BB* pnewBB;

    pnewBB = new BB;
    pnewBB->nodeType = nodeType;	/* Initialise */
    pnewBB->start = start;
    pnewBB->length = ip - start + 1;
    pnewBB->numOutEdges = (byte)numOutEdges;
    pnewBB->immedDom = NO_DOM;
    pnewBB->loopHead = pnewBB->caseHead = pnewBB->caseTail =
            pnewBB->latchNode= pnewBB->loopFollow = NO_NODE;

    if (numOutEdges)
        pnewBB->edges.resize(numOutEdges);

    /* Mark the basic block to which the icodes belong to, but only for
         * real code basic blocks (ie. not interval bbs) */
    if(parent)
    {
        if (start >= 0)
            parent->Icode.SetInBB(start, ip, pnewBB);
        parent->heldBBs.push_back(pnewBB);
        parent->cfg.push_back(pnewBB);
    }
    if (start != -1)		/* Only for code BB's */
        stats.numBBbef++;
    return pnewBB;
}
