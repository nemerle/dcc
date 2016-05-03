#ifndef COMMAND_H
#define COMMAND_H

#include <memory>
#include <QtCore/QObject>
#include <QtCore/QVector>
#include <QtCore/QPair>

class Project;
struct Function;
typedef std::shared_ptr<Function> PtrFunction;
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

    Project *m_project;
    PtrFunction *m_func;
    QVector<QPair<Command *,QString>> m_failures;
    void reset();
};

class Command
{
    QString m_command_name;
    CommandLevel m_level;
public:
    Command(QString n,CommandLevel level) : m_command_name(n),m_level(level) {}
    virtual ~Command() {}

    QString name() const { return m_command_name;}
    virtual QString instanceDescription() const { return m_command_name; }
    virtual bool execute(CommandContext *) { return false; }
};
class CompoundCommand : public Command {
    QVector<Command *> m_contained;
public:
    CompoundCommand(QString n,CommandLevel l) : Command(n,l) {
    }
    void addCommand(Command *c) {
        m_contained.push_back(c);
    }
    bool execute(CommandContext * ctx) {
        for(Command * c : m_contained) {
            if(!c->execute(ctx))
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
    bool processOne(CommandContext *ctx);
    void processAll(CommandContext *ctx);
    void clear();
signals:
    void streamCompleted(bool success);
    void streamChanged();
};
// Effect: loader has been selected and set in current project
class LoaderSelection : public Command {
    QString m_filename;
public:
    virtual ~LoaderSelection() {}
    LoaderSelection(QString f) : Command("Select loader",eProject),m_filename(f) {}
    bool execute(CommandContext * ctx) override;
};
// trigger Project->m_selected_loader has changed
// Effect: the PROG object is loaded using the current loader
class LoaderApplication : public Command {
    QString m_filename;
public:
    virtual ~LoaderApplication() {}
    LoaderApplication(QString f) : Command("Apply loader",eProject),m_filename(f) {}
    bool execute(CommandContext * ctx) override;
};
#endif // COMMAND_H
