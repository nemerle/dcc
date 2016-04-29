#ifndef COMMANDQUEUEVIEW_H
#define COMMANDQUEUEVIEW_H

#include <QtWidgets/QDockWidget>

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

protected:
    void changeEvent(QEvent *e);

private:
    Ui::CommandQueueView *ui;
};

#endif // COMMANDQUEUEVIEW_H
