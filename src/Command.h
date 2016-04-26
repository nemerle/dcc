#ifndef COMMAND_H
#define COMMAND_H

#include <QtCore/QObject>
#include <QtCore/QVector>
#include <QtCore/QPair>

class Project;
enum CommandLevel {
    eProject,
    eBinary,
    eFunction,
    eBasicBlock,
    eInstruction
};
class Command;

class CommandContext {
public:
    void recordFailure(Command *cmd,QString error_message) {
        m_failures.push_back({cmd,error_message});
    }

    Project *proj;
    QVector<QPair<Command *,QString>> m_failures;
    void reset();
};

class Command
{
    QString m_command_name;
    CommandLevel m_level;
public:
    Command(QString n,CommandLevel level) : m_command_name(n),m_level(level) {}
    QString name() const { return m_command_name;}
    virtual bool execute(CommandContext *,Project *) { return false; }
};
class CompoundCommand : public Command {
    QVector<Command *> m_contained;
public:
    CompoundCommand(QString n,CommandLevel l) : Command(n,l) {
    }
    void addCommand(Command *c) {
        m_contained.push_back(c);
    }
    bool execute(CommandContext * ctx,Project *v) {
        for(Command * c : m_contained) {
            if(!c->execute(ctx,v))
                return false;
        }
        return true;
    }
};
class CommandStream : public QObject
{
    Q_OBJECT
    int m_maximum_command_count;
public:
    QVector<Command *> m_recently_executed;
    QVector<Command *> m_commands;
    bool add(Command *c);
    void setMaximumCommandCount(int maximum_command_count);
    void processAll(CommandContext *ctx);
    void clear();
signals:
    void streamCompleted(bool success);
};

class LoaderSelection : public Command {
public:
    virtual ~LoaderSelection() {}
    LoaderSelection() : Command("Select loader",eProject) {}
    bool execute(CommandContext * ctx,Project *) override;
};
class LoaderApplication : public Command {
public:
    virtual ~LoaderApplication() {}
    LoaderApplication() : Command("Apply loader",eProject) {}
    bool execute(CommandContext * ctx,Project *) override;
};
#endif // COMMAND_H
