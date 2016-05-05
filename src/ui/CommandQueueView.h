#ifndef COMMANDQUEUEVIEW_H
#define COMMANDQUEUEVIEW_H

#include <QtWidgets/QDockWidget>
#include "Procedure.h"


namespace Ui {
class CommandQueueView;
}

class CommandQueueView : public QDockWidget
{
    Q_OBJECT

public:
    explicit CommandQueueView(QWidget *parent = 0);
    ~CommandQueueView();

public slots:
    void onCommandListChanged();
    void onCurrentFunctionChanged(PtrFunction func);

protected:
    void changeEvent(QEvent *e);

private slots:
    void on_btnStep_clicked();

    void on_btnPlan_clicked();

private:
    Ui::CommandQueueView *ui;
    PtrFunction m_current_function;
};

#endif // COMMANDQUEUEVIEW_H
