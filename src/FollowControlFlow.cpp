#include "FollowControlFlow.h"

#include "project.h"
#include "parser.h"

QString FollowControlFlow::instanceDescription() const
{
    return name() + " @ 0x"+QString::number(m_start_state.IP,16);
}

bool FollowControlFlow::execute(CommandContext *ctx)
{
    Project &proj(*ctx->m_project);
    PtrFunction scanned_func(ctx->m_func);
    scanned_func->switchState(eDisassemblyInProgress);
    FollowCtrl(*scanned_func,proj.callGraph, &m_start_state);
    return false;
}

QString MarkAsSwitchCase::instanceDescription() const
{
    return name() + QString(" 0x%1 -> 0x%2 ; case %3")
            .arg(m_src_addr,8,16,QChar('0'))
            .arg(m_dst_addr,8,16,QChar('0'))
            .arg(m_case_label);

}

bool MarkAsSwitchCase::execute(CommandContext * ctx)
{
    //TODO: record code/data referneces in project for navigation UI purposes ?
    auto switch_insn = ctx->m_func->Icode.labelSrch(m_src_addr);
    if(switch_insn==ctx->m_func->Icode.end()) {
        ctx->recordFailure(this,QString("switch instruction @ 0x%1 not found in procedure's instructions ?")
                           .arg(m_src_addr,8,16,QChar('0')));
        return false;
    }
    auto insn = ctx->m_func->Icode.labelSrch(m_dst_addr);
    if(insn==ctx->m_func->Icode.end()) {
        ctx->recordFailure(this,QString("switch target instruction 0x%1 not found in procedure's instructions ?")
                           .arg(m_dst_addr,8,16,QChar('0')));
        return false;
    }
    insn->ll()->caseEntry = m_case_label;
    insn->ll()->setFlags(CASE);
    switch_insn->ll()->caseTbl2.push_back( m_dst_addr );
    return true;

}
