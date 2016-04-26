#include "Command.h"

#include "project.h"
#include "Loaders.h"

#include <QFile>

bool LoaderSelection::execute(CommandContext * ctx, Project *p)
{
    if(p->binary_path().isEmpty()) {
        ctx->recordFailure(this,QString("No executable path set in project %1").arg(p->project_name()));
        return false;
    }

    QFile finfo(p->binary_path());
    /* Open the input file */
    if(not finfo.open(QFile::ReadOnly)) {
        ctx->recordFailure(this,QString("Cannot open file %1").arg(p->binary_path()));
        return false;
    }
    /* Read in first 2 bytes to check EXE signature */
    if (finfo.size()<=2)
    {
        ctx->recordFailure(this,QString("File %1 is too small").arg(p->binary_path()));
    }
    ComLoader com_loader;
    ExeLoader exe_loader;

    if(exe_loader.canLoad(finfo)) {
        p->m_selected_loader = new ExeLoader;
        return true;
    }
    if(com_loader.canLoad(finfo)) {
        p->m_selected_loader = new ComLoader;
        return true;
    }
    ctx->recordFailure(this,QString("None of the available loaders can load file %1").arg(p->binary_path()));

    return true;
}

bool LoaderApplication::execute(CommandContext * ctx, Project *p)
{
    if(!p)
        return false;
    if(!p->m_selected_loader) {
        ctx->recordFailure(this,QString("No loader selected for project %1").arg(p->project_name()));
        return false;
    }
    QFile finfo(p->binary_path());
    if(not finfo.open(QFile::ReadOnly)) {
        ctx->recordFailure(this,QString("Cannot open file %1").arg(p->binary_path()));
        return false;
    }
    return p->m_selected_loader->load(p->prog,finfo);
}

bool CommandStream::add(Command * c) {
    if(m_commands.size()>=m_maximum_command_count)
        return false;
    m_commands.push_back(c);
    return true;
}

void CommandStream::setMaximumCommandCount(int maximum_command_count) {
    m_maximum_command_count = maximum_command_count;
}

void CommandStream::processAll(CommandContext *ctx)
{
    while(not m_commands.isEmpty()) {
        Command *cmd = m_commands.takeFirst();
        if(false==cmd->execute(ctx,ctx->proj)) {
            emit streamCompleted(false);
            break;
        }
        m_recently_executed.push_back(cmd);
    }
    emit streamCompleted(true);
}

void CommandStream::clear()
{
    qDeleteAll(m_commands);
    qDeleteAll(m_recently_executed);
    m_commands.clear();
    m_recently_executed.clear();
}


void CommandContext::reset()
{
    for(int i=0; i<m_failures.size(); ++i) {
        delete m_failures[i].first;
    }
    m_failures.clear();
}
