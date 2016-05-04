#include "FollowControlFlow.h"

#include "project.h"

QString FollowControlFlow::instanceDescription() const
{
    return name() + " @ 0x"+QString::number(m_start_state.IP,16);
}

bool FollowControlFlow::execute(CommandContext *ctx)
{
    Project &proj(*ctx->m_project);
    PtrFunction scanned_func(ctx->m_func);
    scanned_func->FollowCtrl(proj.callGraph, &m_start_state);
    return false;
}
