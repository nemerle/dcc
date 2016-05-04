#ifndef FOLLOWCONTROLFLOW_H
#define FOLLOWCONTROLFLOW_H

#include "Command.h"

#include "state.h"

class FollowControlFlow : public Command
{
    STATE m_start_state;
public:
    FollowControlFlow(STATE addr) : Command("Follow control flow",eFunction),m_start_state(addr) {}

    // Command interface
public:
    QString instanceDescription() const override;
    bool execute(CommandContext *ctx) override;
};

#endif // FOLLOWCONTROLFLOW_H
