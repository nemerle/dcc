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
    const CommandStream& cs(project.m_project_command_stream);
    for(const Command * cmd : cs.m_commands) {
        ui->lstQueuedCommands->addItem(cmd->instanceDescription());
    }
}

void CommandQueueView::on_btnStep_clicked()
{
    Project &project(*Project::get());
    project.processCommands(1);
}

void CommandQueueView::on_btnPlan_clicked()
{
    AutomatedPlanner planner;
    // TODO: Use some kind of UI context to retrieve currently selected function
    //planner.planFor(func);
}
