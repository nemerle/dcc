#include "Command.h"

#include "dcc.h"
#include "project.h"
#include "Loaders.h"

#include <QFile>

bool LoaderSelection::execute(CommandContext * ctx)
{
    Project *proj=ctx->m_project;
    if(nullptr==proj) {
        ctx->recordFailure(this,"No active project ");
        return false;
    }
    if(m_filename.isEmpty()) {
        ctx->recordFailure(this,"No executable path given to loader selector");
        return false;
    }

    QFile finfo(m_filename);
    /* Open the input file */
    if(not finfo.open(QFile::ReadOnly)) {
        ctx->recordFailure(this,QString("Cannot open file %1").arg(m_filename));
        return false;
    }
    /* Read in first 2 bytes to check EXE signature */
    if (finfo.size()<=2)
    {
        ctx->recordFailure(this,QString("File %1 is too small").arg(m_filename));
    }
    ComLoader com_loader;
    ExeLoader exe_loader;

    if(exe_loader.canLoad(finfo)) {
        proj->setLoader(new ExeLoader);
        return true;
    }
    if(com_loader.canLoad(finfo)) {
        proj->setLoader(new ExeLoader);
        return true;
    }
    ctx->recordFailure(this,QString("None of the available loaders can load file %1").arg(m_filename));

    return true;
}

bool LoaderApplication::execute(CommandContext * ctx)
{
    Project *proj=ctx->m_project;

    if(nullptr==proj) {
        ctx->recordFailure(this,"No active project ");
        return false;
    }
    if(!proj->m_selected_loader) {
        ctx->recordFailure(this,QString("No loader selected for project %1").arg(proj->project_name()));
        return false;
    }
    QFile finfo(m_filename);
    if(not finfo.open(QFile::ReadOnly)) {
        ctx->recordFailure(this,QString("Cannot open file %1").arg(m_filename));
        return false;
    }
    bool load_res = proj->m_selected_loader->load(proj->prog,finfo);
    if(!load_res) {
        ctx->recordFailure(this,QString("Failure during load: %1").arg(m_filename));
        return false;
    }
    if (option.verbose)
        proj->prog.displayLoadInfo();
    return true;
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
        if(false==cmd->execute(ctx)) {
            emit streamCompleted(false);
            break;
        }
        m_recently_executed.push_back(cmd);
    }
    emit streamCompleted(true);
}

bool CommandStream::processOne(CommandContext *ctx)
{
    if(not m_commands.isEmpty()) {
        Command *cmd = m_commands.takeFirst();
        if(false==cmd->execute(ctx)) {
            emit streamChanged();
            return false;
        }
        m_recently_executed.push_back(cmd);
    }
    emit streamChanged();
    return true;
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
