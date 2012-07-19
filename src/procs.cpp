/*
 * File:    procs.c
 * Purpose: Functions to support Call graphs and procedures
 * Date:    November 1993
 * (C) Cristina Cifuentes
 */

#include <cstring>
#include <cassert>
#include "dcc.h"
#include "project.h"

extern Project g_proj;
/* Static indentation buffer */
static constexpr int indSize=81;          /* size of indentation buffer; max 20 */
static char indentBuf[indSize] =
        "                                                                                ";
// not static, used in icode.cpp at emitGotoLabel
const char *indentStr(int indLevel) // Indentation according to the depth of the statement
{
    return (&indentBuf[indSize-(indLevel*4)-1]);
}


/* Inserts an outEdge at the current callGraph pointer if the newProc does
 * not exist.  */
void CALL_GRAPH::insertArc (ilFunction newProc)
{
    CALL_GRAPH *pcg;


    /* Check if procedure already exists */
    auto res=std::find_if(outEdges.begin(),outEdges.end(),[newProc](CALL_GRAPH *e) {return e->proc==newProc;});
    if(res!=outEdges.end())
        return;
    /* Include new arc */
    pcg = new CALL_GRAPH;
    pcg->proc = newProc;
    outEdges.push_back(pcg);
}


/* Inserts a (caller, callee) arc in the call graph tree. */
bool CALL_GRAPH::insertCallGraph(ilFunction caller, ilFunction callee)
{
    if (proc == caller)
    {
        insertArc (callee);
        return true;
    }
    else
    {
        for (CALL_GRAPH *edg : outEdges)
            if (edg->insertCallGraph (caller, callee))
                return true;
        return (false);
    }
}

bool CALL_GRAPH::insertCallGraph(Function *caller, ilFunction callee)
{
    return insertCallGraph(Project::get()->funcIter(caller),callee);
}


/* Displays the current node of the call graph, and invokes recursively on
 * the nodes the procedure invokes. */
void CALL_GRAPH::writeNodeCallGraph(int indIdx)
{
    printf ("%s%s\n", indentStr(indIdx), proc->name.c_str());
    for (CALL_GRAPH *cg : outEdges)
        cg->writeNodeCallGraph (indIdx + 1);
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
void LOCAL_ID::newRegArg(iICODE picode, iICODE ticode) const
{
    AstIdent *lhs;
    STKFRAME * call_args_stackframe, *target_stackframe;
    const ID *id;
    int tidx;
    bool regExist=false;
    condId type;
    Function * tproc;
    eReg regL, regH;		/* Registers involved in arguments */

    /* Flag ticode as having register arguments */
    tproc = ticode->hl()->call.proc;
    tproc->flg |= REG_ARGS;

    /* Get registers and index into target procedure's local list */
    call_args_stackframe = ticode->hl()->call.args;
    target_stackframe = &tproc->args;
    lhs = dynamic_cast<AstIdent *>(picode->hl()->asgn.lhs());
    RegisterNode *lhs_reg = dynamic_cast<RegisterNode *>(lhs);
    assert(lhs);
    type = lhs->ident.type();
    if (lhs_reg)
    {
        regL = id_arr[lhs_reg->regiIdx].id.regi;
        if (regL < rAL)
            tidx = tproc->localId.newByteWordReg(TYPE_WORD_SIGN, regL);
        else
            tidx = tproc->localId.newByteWordReg(TYPE_BYTE_SIGN, regL);
        /* Check if register argument already on the formal argument list */
        for(STKSYM &tgt_sym : *target_stackframe)
        {
            RegisterNode *tgt_sym_regs = dynamic_cast<RegisterNode *>(tgt_sym.regs);
            if( tgt_sym_regs == NULL ) // both REGISTER and LONG_VAR require this precondition
                continue;
            if ( tgt_sym_regs->regiIdx == tidx )
            {
                regExist = true;
                break;
            }
        }
    }
    else if (type == LONG_VAR)
    {
        int longIdx = lhs->ident.idNode.longIdx;
        regL = id_arr[longIdx].id.longId.l;
        regH = id_arr[longIdx].id.longId.h;
        tidx = tproc->localId.newLongReg(TYPE_LONG_SIGN, regH, regL, tproc->Icode.begin() /*0*/);
        /* Check if register argument already on the formal argument list */
        for(STKSYM &tgt_sym : *target_stackframe)
        {
            if( tgt_sym.regs == NULL ) // both REGISTER and LONG_VAR require this precondition
                continue;
            if ( tgt_sym.regs->ident.idNode.longIdx == tidx )
            {
                regExist = true;
                break;
            }
        }
    }
    else
        ;//regExist = false;
    /* Do ts (formal arguments) */
    if (regExist == false)
    {
        STKSYM newsym;

        newsym.setArgName(target_stackframe->size());

        if (type == REGISTER)
        {
            if (regL < rAL)
            {
                newsym.type = TYPE_WORD_SIGN;
                newsym.regs = new RegisterNode(tidx, WORD_REG);
            }
            else
            {
                newsym.type = TYPE_BYTE_SIGN;
                newsym.regs = new RegisterNode(tidx, BYTE_REG);
            }
            tproc->localId.id_arr[tidx].name = newsym.name;
        }
        else if (type == LONG_VAR)
        {
            newsym.regs = AstIdent::LongIdx (tidx);
            newsym.type = TYPE_LONG_SIGN;
            tproc->localId.id_arr[tidx].name = newsym.name;
            tproc->localId.propLongId (regL, regH, tproc->localId.id_arr[tidx].name.c_str());
        }
        target_stackframe->push_back(newsym);
        target_stackframe->numArgs++;
    }

    /* Do ps (actual arguments) */
    STKSYM newsym;
    newsym.setArgName(call_args_stackframe->size());
    newsym.actual = picode->hl()->asgn.rhs;
    newsym.regs = lhs;
    /* Mask off high and low register(s) in picode */
    switch (type) {
        case REGISTER:
            id = &id_arr[lhs_reg->regiIdx];
            picode->du.def &= maskDuReg[id->id.regi];
            if (id->id.regi < rAL)
                newsym.type = TYPE_WORD_SIGN;
            else
                newsym.type = TYPE_BYTE_SIGN;
            break;
        case LONG_VAR:
            id = &id_arr[lhs->ident.idNode.longIdx];
            picode->du.def &= maskDuReg[id->id.longId.h];
            picode->du.def &= maskDuReg[id->id.longId.l];
            newsym.type = TYPE_LONG_SIGN;
            break;
        default:
            fprintf(stderr,"LOCAL_ID::newRegArg unhandled type %d in masking low\n",type);
    }
    call_args_stackframe->push_back(newsym);
    call_args_stackframe->numArgs++;
}


/** Inserts the new expression (ie. the actual parameter) on the argument
 * list.
 * @return true if it was a near call that made use of a segment register.
 *         false elsewhere
*/
bool CallType::newStkArg(Expr *exp, llIcode opcode, Function * pproc)
{
    RegisterNode *expr = dynamic_cast<RegisterNode *>(exp);

    uint8_t regi;
    /* Check for far procedure call, in which case, references to segment
         * registers are not be considered another parameter (i.e. they are
         * long references to another segment) */
    if (expr)
    {
        regi =  pproc->localId.id_arr[expr->regiIdx].id.regi;
        if ((regi >= rES) && (regi <= rDS))
        {
            if (opcode == iCALLF)
                return false;
            else
                return true;
        }
    }

    /* Place register argument on the argument list */
    STKSYM newsym;
    newsym.actual = exp;
    args->push_back(newsym);
    args->numArgs++;
    return false;
}


/* Places the actual argument exp in the position given by pos in the
 * argument list of picode.	*/
void CallType::placeStkArg (Expr *exp, int pos)
{
    (*args)[pos].actual = exp;
    (*args)[pos].setArgName(pos);
}

Expr *CallType::toAst()
{
    return new FuncNode( proc, args);
}


/* Checks to determine whether the expression (actual argument) has the
 * same type as the given type (from the procedure's formal list).  If not,
 * the actual argument gets modified */
Expr *Function::adjustActArgType (Expr *_exp, hlType forType)
{
    AstIdent *expr = dynamic_cast<AstIdent *>(_exp);
    PROG &prog(Project::get()->prog);
    hlType actType;
    int offset, offL;

    if (expr == NULL)
        return _exp;

    actType = expr-> expType (this);
    if (actType == forType)
        return _exp;
    switch (forType)
    {
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
                    /* It's an offset into image where a string is found.  Point to the string.	*/
                {
                    Constant *c=dynamic_cast<Constant *>(expr);
                    assert(c);
                    offL = c->kte.kte;
                    if (prog.fCOM)
                        offset = (state.r[rDS]<<4) + offL + 0x100;
                    else
                        offset = (state.r[rDS]<<4) + offL;
                    expr->ident.idNode.strIdx = offset;
                    expr->ident.type(STRING);
                    delete c;
                    return AstIdent::String(offset);
                }

                case TYPE_PTR:
                    /* It's a pointer to a char rather than a pointer to
                                         * an integer */
                    /***HERE - modify the type ****/
                    break;

                case TYPE_WORD_SIGN:

                    break;
                default:
                    fprintf(stderr,"adjustForArgType unhandled actType_ %d \n",actType);
            } /* eos */
            break;
        default:
            fprintf(stderr,"adjustForArgType unhandled forType %d \n",forType);
    }
    return _exp;
}


/* Determines whether the formal argument has the same type as the given
 * type (type of the actual argument).  If not, the formal argument is
 * changed its type */
void STKFRAME::adjustForArgType(size_t numArg_, hlType actType_)
{
    hlType forType;
    STKSYM * psym, * nsym;
    int off;
    /* If formal argument does not exist, do not create new ones, just
     * ignore actual argument
     */
    if(numArg_>size())
        return;

    /* Find stack offset for this argument */
    off = m_minOff;
    size_t i=0;
    for(STKSYM &s : *this) // walk formal arguments upto numArg_
    {
        if(i>=numArg_)
            break;
        off+=s.size;
        i++;
    }

    /* Find formal argument */
    //psym = &at(numArg_);
    //i = numArg_;
        //auto iter=std::find_if(sym.begin(),sym.end(),[off](STKSYM &s)->bool {s.off==off;});
    auto iter=std::find_if(begin()+numArg_,end(),[off](STKSYM &s)->bool {return s.label==off;});
    if(iter==end()) // symbol not found
            return;
        psym = &(*iter);

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
                    nsym->macro = "HI";
                    psym->macro = "LO";
                    nsym->hasMacro = true;
                    psym->hasMacro = true;
                    nsym->name = psym->name;
                    nsym->invalid = true;
                    numArgs--;
                }
                break;

            case TYPE_PTR:
            case TYPE_CONST:
            case TYPE_STR:
                break;
            default:
                fprintf(stderr,"STKFRAME::adjustForArgType unhandled actType_ %d \n",actType_);
        } /* eos */
    }
}

