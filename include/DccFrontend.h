#pragma once
#include <QtCore/QObject>
#include "src/Command.h"
#include "project.h"

class Project;
class DccFrontend : public QObject
{
    Q_OBJECT
    void    LoadImage();
    void    parse(Project &proj);
public:
    explicit DccFrontend(QObject *parent = 0);
    bool FrontEnd();            /* frontend.c   */

signals:

public slots:
};

struct MachineStateInitialization : public Command {

    MachineStateInitialization() : Command("Initialize simulated machine state",eProject) {}
    bool execute(CommandContext *ctx) override;
};

struct FindMain : public Command {
    FindMain() : Command("Locate the main entry point",eProject) {}
    bool execute(CommandContext *ctx);
};

struct CreateFunction : public Command {
    QString m_name;
    SegOffAddr m_addr;
    FunctionType *m_type;
    CreateFunction(QString name,SegOffAddr address,FunctionType *f) : Command("Create function",eProject),
        m_name(name),
        m_addr(address),
        m_type(f)
    {}
    QString instanceDescription() const override;

    bool execute(CommandContext *ctx) override;
};
