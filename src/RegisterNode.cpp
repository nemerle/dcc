#include "types.h"
#include "msvc_fixes.h"
#include "ast.h"
#include "bundle.h"
#include "machine_x86.h"
#include "project.h"

#include <stdint.h>
#include <string>
#include <sstream>
#include <iostream>
#include <cassert>
#include <boost/range.hpp>
#include <boost/range/adaptors.hpp>
//#include <boost/range/algorithm.hpp>
//#include <boost/assign.hpp>

using namespace std;
using namespace boost::adaptors;
RegisterNode::RegisterNode(const LLOperand &op, LOCAL_ID *locsym)
{
    m_syms = locsym;
    ident.type(REGISTER);
    hlType type_sel;
    regType reg_type;
    if (op.byteWidth()==1)
    {
        type_sel = TYPE_BYTE_SIGN;
        reg_type = BYTE_REG;
    }
    else    /* uint16_t */
    {
        type_sel = TYPE_WORD_SIGN;
        reg_type = WORD_REG;
    }
    regiIdx = locsym->newByteWordReg(type_sel, op.regi);
    regiType = reg_type;
}

//RegisterNode::RegisterNode(eReg regi, uint32_t icodeFlg, LOCAL_ID *locsym)
//{
//    ident.type(REGISTER);
//    hlType type_sel;
//    regType reg_type;
//    if ((icodeFlg & B) or (icodeFlg & SRC_B))
//    {
//        type_sel = TYPE_BYTE_SIGN;
//        reg_type = BYTE_REG;
//    }
//    else    /* uint16_t */
//    {
//        type_sel = TYPE_WORD_SIGN;
//        reg_type = WORD_REG;
//    }
//    regiIdx = locsym->newByteWordReg(type_sel, regi);
//    regiType = reg_type;
//}

QString RegisterNode::walkCondExpr(Function *pProc, int *numLoc) const
{
    QString codeOut;

    QString o;
    assert(&pProc->localId==m_syms);
    ID *id = &pProc->localId.id_arr[regiIdx];
    if (id->name[0] == '\0')	/* no name */
    {
        id->setLocalName(++(*numLoc));
        codeOut += QString("%1 %2; ").arg(TypeContainer::typeName(id->type)).arg(id->name);
        codeOut += QString("/* %1 */\n").arg(Machine_X86::regName(id->id.regi));
    }
    if (id->hasMacro)
        o += QString("%1(%2)").arg(id->macro).arg(id->name);
    else
        o += id->name;

    cCode.appendDecl(codeOut);
    return o;
}

int RegisterNode::hlTypeSize(Function *) const
{
    if (regiType == BYTE_REG)
        return 1;
    else
        return 2;
}

hlType RegisterNode::expType(Function *pproc) const
{
    if (regiType == BYTE_REG)
        return TYPE_BYTE_SIGN;
    else
        return TYPE_WORD_SIGN;
}

Expr *RegisterNode::insertSubTreeReg(Expr *_expr, eReg regi, const LOCAL_ID *locsym)
{
    assert(locsym==m_syms);
    eReg treeReg = locsym->id_arr[regiIdx].id.regi;
    if (treeReg == regi)                        /* uint16_t reg */
    {
        return _expr;
    }
    else if(Machine_X86::isSubRegisterOf(treeReg,regi))    /* uint16_t/uint8_t reg */
    {
        return _expr;
    }
    return nullptr;
}
bool RegisterNode::xClear(rICODE range_to_check, iICODE lastBBinst, const LOCAL_ID &locId)
{
    uint8_t regi = locId.id_arr[regiIdx].id.regi;
    range_to_check.advance_begin(1);
    auto all_valid_and_high_level_after_start = range_to_check | filtered(ICODE::select_valid_high_level);
    for (ICODE &i : all_valid_and_high_level_after_start)
        if (i.du.def.testRegAndSubregs(regi))
            return false;
    if (all_valid_and_high_level_after_start.end().base() != lastBBinst)
        return true;
    return false;
}
