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
// mark instruction at address m_dst_addr as a case m_case_label of switch located at m_src_addr
class MarkAsSwitchCase : public Command
{
    uint32_t m_src_addr;
    uint32_t m_dst_addr;
    int m_case_label;
public:
    MarkAsSwitchCase(uint32_t src_addr,uint32_t dst_addr,int lab) :
        Command("Mark as switch case",eFunction),
        m_src_addr(src_addr),
        m_dst_addr(dst_addr),
        m_case_label(lab)
    {}

    // Command interface
public:
    QString instanceDescription() const override;
    bool execute(CommandContext *ctx) override;

};
#endif // FOLLOWCONTROLFLOW_H
