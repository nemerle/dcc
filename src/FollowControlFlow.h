#ifndef FOLLOWCONTROLFLOW_H
#define FOLLOWCONTROLFLOW_H

#include "Command.h"

class FollowControlFlow : public Command
{
    uint32_t m_address;
public:
    FollowControlFlow(uint32_t addr) : Command("Follow control flow",eFunction),m_address(addr) {}

    // Command interface
public:
    QString instanceDescription() const override;
    bool execute(CommandContext *ctx) override;
};

#endif // FOLLOWCONTROLFLOW_H
