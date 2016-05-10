#include "Procedure.h"

#include "msvc_fixes.h"
#include "project.h"
#include "scanner.h"
#include "ui/StructuredTextTarget.h"

#include <QtCore/QDebug>

//FunctionType *Function::getFunctionType() const
//{
//    return &m_type;
//}

/* Does some heuristic pruning.  Looks for ptrs. into the table
 * and for addresses that don't appear to point to valid code.
*/
void JumpTable::pruneEntries(uint16_t cs)
{
    PROG *prg(Project::get()->binary());
    for (uint32_t i = start; i < finish; i += 2)
    {
        uint32_t target = cs + LH(&prg->image()[i]);
        if (target < finish and target >= start)
            finish = target;
        else if (target >= (uint32_t)prg->cbImage)
            finish = i;
    }
    ICODE _Icode; // used as scan input
    for (uint32_t i = start; i < finish; i += 2)
    {
        uint32_t target = cs + LH(&prg->image()[i]);
        /* Be wary of 00 00 as code - it's probably data */
        if (not (prg->image()[target] or prg->image()[target+1]) or scan(target, _Icode))
            finish = i;
    }

}


void Function::callingConv(CConv::CC_Type v) {
    type->setCallingConvention(v);
    getFunctionType()->m_call_conv->calculateStackLayout(this);
}
static QString sizeToPtrName(int size)
{
    switch(size)
    {
        case 1:
            return "BYTE ptr" ;
        case 2:
            return "WORD ptr";
        case 4:
            return "DWORD ptr";
    }
    return "UNKOWN ptr";
}
static void toStructuredText(STKFRAME &stk,IStructuredTextTarget *out, int level) {
    int curlevel = 0;
    int maxlevel = stk.m_maxOff - stk.m_minOff;

    for(STKSYM & p : stk)
    {
        if (curlevel > p.label)
        {
            qWarning() << "error, var collapse!!!";
            curlevel = p.label;
        }
        else if (curlevel < p.label)
        {
            out->addSpace(4);
            out->prtt(QString("gap len = %1").arg(p.label - curlevel,0,16));
            curlevel = p.label;
            out->addEOL();
        }
        out->addSpace(4);
        out->addTaggedString(XT_Symbol,p.name,&p);
        out->prtt("equ");
        out->addSpace();
        out->prtt(sizeToPtrName(p.size));
        out->addSpace();
        if (p.arrayMembers>1)
        {
            out->addTaggedString(XT_Number,QString::number(p.arrayMembers,16));
            out->prtt("dup (?)");
            out->addSpace();
        }
        out->TAGbegin(XT_Number, NULL);
        out->prtt(QString("%1h").arg(p.label,0,16));
        out->TAGend(XT_Number);
        out->addEOL();

        curlevel += p.size * p.arrayMembers;
    }

    if (curlevel < maxlevel)
    {
        out->prtt(QString("    gap len = %1h").arg(maxlevel - curlevel,0,16));
    }
}
extern void toStructuredText(LLInst *insn,IStructuredTextTarget *out, int level);

static void toStructuredText(ICODE &stk,IStructuredTextTarget *out, int level) {
    if(level==0) {
        toStructuredText(stk.ll(),out,level);
    }
}

void Function::toStructuredText(IStructuredTextTarget *out, int level)
{

    out->TAGbegin(XT_Function, this);
    out->addTaggedString(XT_FuncName,name);
    out->prtt(" proc");
    out->addEOL();
    ::toStructuredText(args,out,level);
    out->addEOL();
    for(ICODE &ic : Icode) {
        ::toStructuredText(ic,out,level);
    }

    out->addTaggedString(XT_FuncName,name);
    out->addSpace();
    out->prtt("endp");
    out->addEOL();
    out->TAGend(XT_Function);
}

void FunctionType::setCallingConvention(CConv::CC_Type cc)
{
    m_call_conv=CConv::create(cc);
    assert(m_call_conv);
}
void Function::switchState(DecompilationStep s)
{
    nStep = s;
}
