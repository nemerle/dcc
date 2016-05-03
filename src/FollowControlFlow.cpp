#include "FollowControlFlow.h"

QString FollowControlFlow::instanceDescription() const
{
    return name() + " @ 0x"+QString::number(m_address,16);
}

bool FollowControlFlow::execute(CommandContext *ctx)
{
    return false;
}
