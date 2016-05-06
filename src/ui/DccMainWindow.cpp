#include "DccMainWindow.h"
#include "ui_DccMainWindow.h"
//#include "ui_exe2c_gui.h"
//#include "exe2c_interface.h"
//#include "exe2c.h"
#include "FunctionViewWidget.h"
#include "FunctionListDockWidget.h"
#include "CommandQueueView.h"
#include "dcc_interface.h"
#include "project.h"

#include <QtWidgets>
#include <QLabel>
#include <QFileDialog>
IDcc* g_IDCC = NULL;
extern bool exe2c_Init();

DccMainWindow::DccMainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::DccMainWindow)
{
    ui->setupUi(this);
    ui->statusbar->addPermanentWidget(new QLabel("Test"));

    g_IDCC = IDcc::get();
    g_IDCC->BaseInit();
    g_IDCC->Init(this);

    m_last_display  = nullptr;
    m_command_queue = new CommandQueueView(this);
    m_functionlist_widget = new FunctionListDockWidget(this);
    m_functionlist_widget->setWindowTitle(QApplication::tr("Function list"));
    connect(m_functionlist_widget,SIGNAL(selectFunction(PtrFunction)),SLOT(onFunctionSelected(PtrFunction)));
    connect(m_functionlist_widget,SIGNAL(displayRequested()), SLOT(displayCurrentFunction()));

    // we are beeing signalled when display is requested
    connect(this,SIGNAL(functionListChanged()), m_functionlist_widget->model(),SLOT(updateFunctionList()));
    connect(Project::get(),SIGNAL(newFunctionCreated(PtrFunction)),SLOT(onNewFunction(PtrFunction)));
    connect(Project::get(),SIGNAL(functionUpdate(const PtrFunction &)),SLOT(onFunctionUpdated(const PtrFunction &)));

    this->addDockWidget(Qt::RightDockWidgetArea,m_functionlist_widget);
    this->addDockWidget(Qt::LeftDockWidgetArea,m_command_queue);
    connect(m_functionlist_widget,&FunctionListDockWidget::selectFunction,
            m_command_queue, &CommandQueueView::onCurrentFunctionChanged);
}

DccMainWindow::~DccMainWindow()
{
    delete ui;
}

void DccMainWindow::changeEvent(QEvent *e)
{
    QMainWindow::changeEvent(e);
    switch (e->type()) {
        case QEvent::LanguageChange:
            ui->retranslateUi(this);
            break;
        default:
            break;
    }
}
void DccMainWindow::onFunctionSelected(PtrFunction func) {
    m_selected_func = func;
}
// TODO: consider increasing granularity of change events to reduce redraw/refresh spam
void DccMainWindow::onFunctionUpdated(const PtrFunction &f) {
    // some function was updated refresh the list and the view if open
    auto iter = m_map_function_to_view.find(f);
    if(iter!=m_map_function_to_view.end()) {
        iter->second->renderCurrent();
    }
    emit functionListChanged();
}
void DccMainWindow::onNewFunction(PtrFunction f) {
    emit functionListChanged();
}
void DccMainWindow::onOptim()
{
    Project::get()->processCommands();
    g_IDCC->analysis_Once();
    emit functionListChanged();
    if(m_last_display==m_selected_func)
    {
        displayCurrentFunction();
    }
}
void DccMainWindow::onOptim10()
{
    for(int i=0; i<10; i++)
        g_IDCC->analysis_Once();
    emit functionListChanged();
    if(m_last_display==m_selected_func)
    {
        displayCurrentFunction();
    }
}
void DccMainWindow::onOpenFile_Action()
{
    QFileDialog dlg;
    QString name=dlg.getOpenFileName(0,
                                     tr("Select DOS executable"),
                                     ".",
                                     tr("Executable files (*.exe *.EXE *.com *.COM)"));
    if(!g_IDCC->load(name)) {
        QMessageBox::critical(this,tr("Error"),QString(tr("Cannot open file %1")).arg(name));
    }
    //bool m_bSucc = m_xTextBuffer.LoadFromFile(lpszPathName);
    emit functionListChanged();
}
void DccMainWindow::functionViewClosed() {
    FunctionViewWidget *sndr = qobject_cast<FunctionViewWidget *>(sender());
    for(auto iter = m_mdi_function_views.begin(); iter!=m_mdi_function_views.end(); ++iter) {
        if(*iter==sndr) {
            m_map_function_to_view.erase(sndr->viewedFunction());
            m_mdi_function_views.erase(iter);
            break;
        }
    }
    ui->mdiArea->removeSubWindow(sndr);
    sndr->deleteLater();

}
void DccMainWindow::displayCurrentFunction()
{
    if(m_last_display!=m_selected_func) {
        m_last_display=m_selected_func;
        // Check if function's view is already open, if it is, acivate it
        auto iter = m_map_function_to_view.find(m_selected_func);
        if(iter!=m_map_function_to_view.end()) {
            ui->mdiArea->setActiveSubWindow(qobject_cast<QMdiSubWindow *>(iter->second->parent()));
            return;
        }
        FunctionViewWidget *view = new FunctionViewWidget(m_selected_func,this);
        view->setWindowTitle(QString(tr("Listing for %1")).arg(m_selected_func->name));
        connect(view,SIGNAL(close()),SLOT(functionViewClosed()));
        ui->mdiArea->addSubWindow(view);
        view->show();
        m_mdi_function_views.push_back(view);
        m_map_function_to_view[m_selected_func] = view;
    }
}
void DccMainWindow::prt_log(const char *v)
{
    qDebug()<<v;
}

void DccMainWindow::on_actionExit_triggered()
{
    qApp->exit(0);
}
