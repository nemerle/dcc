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
IDcc* g_EXE2C = NULL;
extern bool exe2c_Init();

DccMainWindow::DccMainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::DccMainWindow)
{
    ui->setupUi(this);
    ui->statusbar->addPermanentWidget(new QLabel("Test"));

    g_EXE2C = IDcc::get();
    g_EXE2C->BaseInit();
    g_EXE2C->Init(this);

    m_last_display        = g_EXE2C->GetFirstFuncHandle();
    m_command_queue = new CommandQueueView(this);
    m_functionlist_widget = new FunctionListDockWidget(this);
    m_functionlist_widget->setWindowTitle(QApplication::tr("Function list"));
    connect(m_functionlist_widget,SIGNAL(displayRequested()), SLOT(displayCurrentFunction()));
    // we are beeing signalled when display is requested
    connect(this,SIGNAL(functionListChanged()), m_functionlist_widget->model(),SLOT(updateFunctionList()));
    this->addDockWidget(Qt::RightDockWidgetArea,m_functionlist_widget);
    this->addDockWidget(Qt::LeftDockWidgetArea,m_command_queue);
    m_asm_view = new FunctionViewWidget(this);
    m_asm_view->setWindowTitle(tr("Assembly listing"));
    ui->mdiArea->addSubWindow(m_asm_view);
    //m_internal_view = new FunctionViewWidget;
    //m_internal_view->setWindowTitle(QApplication::tr("Internal listing"));
    //ui->mdiArea->addSubWindow(m_internal_view);
    m_c_view = new FunctionViewWidget;
    m_c_view->setWindowTitle(tr("Decompiled"));
    ui->mdiArea->addSubWindow(m_c_view);
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
void DccMainWindow::onOptim()
{
    Project::get()->processCommands();
    g_EXE2C->analysis_Once();
    emit functionListChanged();
    if(m_last_display==g_EXE2C->GetCurFuncHandle())
    {
        displayCurrentFunction();
    }
}
void DccMainWindow::onOptim10()
{
    for(int i=0; i<10; i++)
        g_EXE2C->analysis_Once();
    emit functionListChanged();
    if(m_last_display==g_EXE2C->GetCurFuncHandle())
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
    if(!g_EXE2C->load(name)) {
        QMessageBox::critical(this,tr("Error"),QString(tr("Cannot open file %1")).arg(name));
    }
    //bool m_bSucc = m_xTextBuffer.LoadFromFile(lpszPathName);
    emit functionListChanged();
}

void DccMainWindow::displayCurrentFunction()
{
    if(m_last_display!=g_EXE2C->GetCurFuncHandle())
        m_last_display=g_EXE2C->GetCurFuncHandle();
    g_EXE2C->prtout_asm(m_asm_view);
    //g_EXE2C->prtout_itn(m_internal_view);
    g_EXE2C->prtout_cpp(m_c_view);
}
void DccMainWindow::prt_log(const char *v)
{
    qDebug()<<v;
}

void DccMainWindow::on_actionExit_triggered()
{
    qApp->exit(0);
}
