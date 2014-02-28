#include "icode.h"
#include "ast.h"

void HLTYPE::replaceExpr(Expr *e)
{
    assert(e);
    delete exp.v;
    exp.v=e;
}


HlTypeSupport *HLTYPE::get()
{
    switch(opcode)
    {
    case HLI_ASSIGN: return &asgn;
    case HLI_RET:
    case HLI_POP:
    case HLI_JCOND:
    case HLI_PUSH:   return &exp;
    case HLI_CALL:   return &call;
    default:
        return nullptr;
    }
}
