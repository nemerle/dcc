#include "CommandQueueView.h"

#include "project.h"
#include "../AutomatedPlanner.h"

#include "ui_CommandQueueView.h"

CommandQueueView::CommandQueueView(QWidget *parent) :
    QDockWidget(parent),
    ui(new Ui::CommandQueueView)
{
    ui->setupUi(this);
    connect(Project::get(),SIGNAL(commandListChanged()),SLOT(onCommandListChanged()));
}

CommandQueueView::~CommandQueueView()
{
    delete ui;
}

void CommandQueueView::changeEvent(QEvent *e)
{
    QDockWidget::changeEvent(e);
    switch (e->type()) {
    case QEvent::LanguageChange:
        ui->retranslateUi(this);
        break;
    default:
        break;
    }
}

void CommandQueueView::onCommandListChanged() {
    Project &project(*Project::get());
    ui->lstQueuedCommands->clear();
    CommandStream * func_stream = project.functionCommands(m_current_function);
    if(func_stream) {
        for(const Command * cmd : func_stream->m_commands) {
            ui->lstQueuedCommands->addItem(cmd->instanceDescription());
        }
    }
    const CommandStream& project_commands(project.m_project_command_stream);
    for(const Command * cmd : project_commands.m_commands) {
        ui->lstQueuedCommands->addItem(cmd->instanceDescription());
    }
}

void CommandQueueView::onCurrentFunctionChanged(PtrFunction func)
{
    m_current_function=func;
    onCommandListChanged();
}

void CommandQueueView::on_btnStep_clicked()
{
    Project &project(*Project::get());
    if(nullptr!=m_current_function and project.hasCommands(m_current_function))
        project.processFunctionCommands(m_current_function,1);
    else
        project.processCommands(1);
}

void CommandQueueView::on_btnPlan_clicked()
{
    AutomatedPlanner planner;
    if(m_current_function!=nullptr) {
        planner.planFor(*m_current_function);
    }
    else
        planner.planFor(*Project::get());
}
