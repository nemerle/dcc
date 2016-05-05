#ifndef EXE2C_MAINWINDOW_H
#define EXE2C_MAINWINDOW_H

#include "Procedure.h"
#include "DccFrontend.h"

#include <QMainWindow>
#include <QAbstractTableModel>
#include <QVariant>
#include <vector>

class FunctionViewWidget;
class FunctionListDockWidget;
class CommandQueueView;

namespace Ui {
class DccMainWindow;
}

class DccMainWindow : public QMainWindow/*,public I_E2COUT*/ {
    Q_OBJECT
public:
    explicit DccMainWindow(QWidget *parent = 0);
    ~DccMainWindow();
    void prt_log(const char * str);
public slots:
    void onOptim();
    void onOptim10();
    void onOpenFile_Action();
    void displayCurrentFunction();
signals:
    void functionListChanged();
protected:
    void changeEvent(QEvent *e);
private slots:
    void on_actionExit_triggered();
    void onNewFunction(PtrFunction f);
    void onFunctionSelected(PtrFunction func);
private:

    FunctionViewWidget *m_asm_view;
    //  FunctionViewWidget *m_internal_view;
    FunctionViewWidget *m_c_view;
    CommandQueueView *m_command_queue;
    FunctionListDockWidget *m_functionlist_widget;
    Ui::DccMainWindow *ui;
    PtrFunction m_last_display;
    PtrFunction m_selected_func;
};

#endif // EXE2C_MAINWINDOW_H
