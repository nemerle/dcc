/*
 * File:    procs.c
 * Purpose: Functions to support Call graphs and procedures
 * Date:    November 1993
 * (C) Cristina Cifuentes
 */

#include <cstring>
#include <cassert>
#include "dcc.h"


/* Static indentation buffer */
#define indSize     61          /* size of indentation buffer; max 20 */
static char indentBuf[indSize] =
        "                                                            ";

static char *indent (Int indLevel)
/* Indentation according to the depth of the statement */
{
    return (&indentBuf[indSize-(indLevel*3)-1]);
}


/* Inserts an outEdge at the current callGraph pointer if the newProc does
 * not exist.  */
void CALL_GRAPH::insertArc (ilFunction newProc)
{
    CALL_GRAPH *pcg;
    Int i;

    /* Check if procedure already exists */
    for (i = 0;  i < outEdges.size(); i++)
        if (outEdges[i]->proc == newProc)
            return;

    /* Include new arc */
    pcg = new CALL_GRAPH;
    pcg->proc = newProc;
    outEdges.push_back(pcg);
}


/* Inserts a (caller, callee) arc in the call graph tree. */
boolT CALL_GRAPH::insertCallGraph(ilFunction caller, ilFunction callee)
{
    Int i;

    if (proc == caller)
    {
        insertArc (callee);
        return true;
    }
    else
    {
        for (i = 0; i < outEdges.size(); i++)
            if (outEdges[i]->insertCallGraph (caller, callee))
                return true;
        return (false);
    }
}

boolT CALL_GRAPH::insertCallGraph(Function *caller, ilFunction callee)
{
    auto iter = std::find_if(pProcList.begin(),pProcList.end(),
                             [caller](const Function &f)->bool {return caller==&f;});
    assert(iter!=pProcList.end());
    return insertCallGraph(iter,callee);
}


/* Displays the current node of the call graph, and invokes recursively on
 * the nodes the procedure invokes. */
void CALL_GRAPH::writeNodeCallGraph(Int indIdx)
{
    Int i;

    printf ("%s%s\n", indent(indIdx), proc->name);
    for (i = 0; i < outEdges.size(); i++)
        outEdges[i]->writeNodeCallGraph (indIdx + 1);
}


/* Writes the header and invokes recursive procedure */
void CALL_GRAPH::write()
{
    printf ("\nCall Graph:\n");
    writeNodeCallGraph (0);
}


/**************************************************************************
 *  Routines to support arguments
 *************************************************************************/

/* Updates the argument table by including the register(s) (ie. lhs of
 * picode) and the actual expression (ie. rhs of picode).
 * Note: register(s) are only included once in the table.   */
void Function::newRegArg(ICODE *picode, ICODE *ticode)
{
    COND_EXPR *lhs;
    STKFRAME * ps, *ts;
    ID *id;
    Int i, tidx;
    boolT regExist;
    condId type;
    Function * tproc;
    byte regL, regH;		/* Registers involved in arguments */

    /* Flag ticode as having register arguments */
    tproc = ticode->ic.hl.oper.call.proc;
    tproc->flg |= REG_ARGS;

    /* Get registers and index into target procedure's local list */
    ps = ticode->ic.hl.oper.call.args;
    ts = &tproc->args;
    lhs = picode->ic.hl.oper.asgn.lhs;
    type = lhs->expr.ident.idType;
    if (type == REGISTER)
    {
        regL = localId.id_arr[lhs->expr.ident.idNode.regiIdx].id.regi;
        if (regL < rAL)
            tidx = tproc->localId.newByteWordReg(TYPE_WORD_SIGN, regL);
        else
            tidx = tproc->localId.newByteWordReg(TYPE_BYTE_SIGN, regL);
    }
    else if (type == LONG_VAR)
    {
        regL = localId.id_arr[lhs->expr.ident.idNode.longIdx].id.longId.l;
        regH = localId.id_arr[lhs->expr.ident.idNode.longIdx].id.longId.h;
        tidx = tproc->localId.newLongReg(TYPE_LONG_SIGN, regH, regL, 0);
    }

    /* Check if register argument already on the formal argument list */
    regExist = FALSE;
    for (i = 0; i < ts->sym.size(); i++)
    {
        if (type == REGISTER)
        {
            if ((ts->sym[i].regs != NULL) &&
                    (ts->sym[i].regs->expr.ident.idNode.regiIdx == tidx))
            {
                regExist = TRUE;
                i = ts->sym.size();
            }
        }
        else if (type == LONG_VAR)
        {
            if ((ts->sym[i].regs != NULL) &&
                    (ts->sym[i].regs->expr.ident.idNode.longIdx == tidx))
            {
                regExist = TRUE;
                i = ts->sym.size();
            }
        }
    }

    /* Do ts (formal arguments) */
    if (regExist == FALSE)
    {
        STKSYM newsym;
        sprintf (newsym.name, "arg%ld", ts->sym.size());
        if (type == REGISTER)
        {
            if (regL < rAL)
            {
                newsym.type = TYPE_WORD_SIGN;
                newsym.regs = COND_EXPR::idRegIdx(tidx, WORD_REG);
            }
            else
            {
                newsym.type = TYPE_BYTE_SIGN;
                newsym.regs = COND_EXPR::idRegIdx(tidx, BYTE_REG);
            }
            sprintf (tproc->localId.id_arr[tidx].name, "arg%ld", ts->sym.size());
        }
        else if (type == LONG_VAR)
        {
            newsym.regs = COND_EXPR::idLongIdx (tidx);
            newsym.type = TYPE_LONG_SIGN;
            sprintf (tproc->localId.id_arr[tidx].name, "arg%ld", ts->sym.size());
            tproc->localId.propLongId (regL, regH,
                        tproc->localId.id_arr[tidx].name);
        }
        ts->sym.push_back(newsym);
        ts->numArgs++;
    }

    /* Do ps (actual arguments) */
    STKSYM newsym;
    sprintf (newsym.name, "arg%ld", ps->sym.size());
    newsym.actual = picode->ic.hl.oper.asgn.rhs;
    newsym.regs = lhs;
    /* Mask off high and low register(s) in picode */
    switch (type) {
        case REGISTER:
            id = &localId.id_arr[lhs->expr.ident.idNode.regiIdx];
            picode->du.def &= maskDuReg[id->id.regi];
            if (id->id.regi < rAL)
                newsym.type = TYPE_WORD_SIGN;
            else
                newsym.type = TYPE_BYTE_SIGN;
            break;
        case LONG_VAR:
            id = &localId.id_arr[lhs->expr.ident.idNode.longIdx];
            picode->du.def &= maskDuReg[id->id.longId.h];
            picode->du.def &= maskDuReg[id->id.longId.l];
            newsym.type = TYPE_LONG_SIGN;
            break;
    }
    ps->sym.push_back(newsym);
    ps->numArgs++;
}


/* Allocates num arguments in the actual argument list of the current
 * icode picode.	*/
/** NOTE: this function is not used ****/
void allocStkArgs (ICODE *picode, Int num)
{
    STKFRAME * ps;
    ps = picode->ic.hl.oper.call.args;
    ps->numArgs = num;
    ps->sym.resize(num);
}


/* Inserts the new expression (ie. the actual parameter) on the argument
 * list.
 * Returns: TRUE if it was a near call that made use of a segment register.
 *			FALSE elsewhere	*/
boolT newStkArg (ICODE *picode, COND_EXPR *exp, llIcode opcode, Function * pproc)
{
	STKFRAME * ps;
    byte regi;

    /* Check for far procedure call, in which case, references to segment
         * registers are not be considered another parameter (i.e. they are
         * long references to another segment) */
    if (exp)
    {
        if ((exp->type == IDENTIFIER) && (exp->expr.ident.idType == REGISTER))
        {
            regi =  pproc->localId.id_arr[exp->expr.ident.idNode.regiIdx].id.regi;
            if ((regi >= rES) && (regi <= rDS))
                if (opcode == iCALLF)
                    return false;
                else
                    return true;
        }
    }

    /* Place register argument on the argument list */
    ps = picode->ic.hl.oper.call.args;
    STKSYM newsym;
    newsym.actual = exp;
    ps->sym.push_back(newsym);
    ps->numArgs++;
    return false;
}


/* Places the actual argument exp in the position given by pos in the
 * argument list of picode.	*/
void placeStkArg (ICODE *picode, COND_EXPR *exp, Int pos)
{ STKFRAME * ps;

    ps = picode->ic.hl.oper.call.args;
    ps->sym[pos].actual = exp;
    sprintf (ps->sym[pos].name, "arg%ld", pos);
}


/* Checks to determine whether the expression (actual argument) has the
 * same type as the given type (from the procedure's formal list).  If not,
 * the actual argument gets modified */
void adjustActArgType (COND_EXPR *exp, hlType forType, Function * pproc)
{ hlType actType;
    Int offset, offL;

    if (exp == NULL)
        return;

    actType = expType (exp, pproc);
    if ((actType != forType) && (exp->type == IDENTIFIER))
    {
        switch (forType) {
            case TYPE_UNKNOWN: case TYPE_BYTE_SIGN:
            case TYPE_BYTE_UNSIGN: case TYPE_WORD_SIGN:
            case TYPE_WORD_UNSIGN: case TYPE_LONG_SIGN:
            case TYPE_LONG_UNSIGN: case TYPE_RECORD:
                break;

            case TYPE_PTR:
            case TYPE_CONST:
                break;

            case TYPE_STR:
                switch (actType) {
                    case TYPE_CONST:
                        /* It's an offset into image where a string is
                                         * found.  Point to the string.	*/
                        offL = exp->expr.ident.idNode.kte.kte;
                        if (prog.fCOM)
                            offset = (pproc->state.r[rDS]<<4) + offL + 0x100;
                        else
                            offset = (pproc->state.r[rDS]<<4) + offL;
                        exp->expr.ident.idNode.strIdx = offset;
                        exp->expr.ident.idType = STRING;
                        break;

                    case TYPE_PTR:
                        /* It's a pointer to a char rather than a pointer to
                                         * an integer */
                        /***HERE - modify the type ****/
                        break;

                    case TYPE_WORD_SIGN:

                        break;
                } /* eos */
                break;
        }
    }
}


/* Determines whether the formal argument has the same type as the given
 * type (type of the actual argument).  If not, the formal argument is
 * changed its type */
void STKFRAME::adjustForArgType(Int numArg_, hlType actType_)
{
    hlType forType;
    STKSYM * psym, * nsym;
    Int off, i;

    /* Find stack offset for this argument */
    off = minOff;
    for (i = 0; i < numArg_; i++)
        off += sym[i].size;

    /* Find formal argument */
    if (numArg_ < sym.size())
    {
        psym = &sym[numArg_];
        i = numArg_;
        while ((i < sym.size()) && (psym->off != off))
        {
            psym++;
            i++;
        }
        if (numArg_ == sym.size())
            return;
    }
    /* If formal argument does not exist, do not create new ones, just
         * ignore actual argument */
    else
        return;

    forType = psym->type;
    if (forType != actType_)
    {
        switch (actType_) {
            case TYPE_UNKNOWN: case TYPE_BYTE_SIGN:
            case TYPE_BYTE_UNSIGN: case TYPE_WORD_SIGN:
            case TYPE_WORD_UNSIGN: case TYPE_RECORD:
                break;

            case TYPE_LONG_UNSIGN: case TYPE_LONG_SIGN:
                if ((forType == TYPE_WORD_UNSIGN) ||
                        (forType == TYPE_WORD_SIGN) ||
                        (forType == TYPE_UNKNOWN))
                {
                    /* Merge low and high */
                    psym->type = actType_;
                    psym->size = 4;
                    nsym = psym + 1;
                    sprintf (nsym->macro, "HI");
                    sprintf (psym->macro, "LO");
                    nsym->hasMacro = TRUE;
                    psym->hasMacro = TRUE;
                    sprintf (nsym->name, "%s", psym->name);
                    nsym->invalid = TRUE;
                    numArgs--;
                }
                break;

            case TYPE_PTR:
            case TYPE_CONST:
            case TYPE_STR:
                break;
        } /* eos */
    }
}

