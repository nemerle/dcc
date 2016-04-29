#ifndef FUNCTIONLISTDOCKWIDGET_H
#define FUNCTIONLISTDOCKWIDGET_H

#include <QAbstractTableModel>
#include <QDockWidget>
//#include "exe2c.h"

class FunctionListModel : public QAbstractTableModel
{
    Q_OBJECT

    struct function_info
    {
        QString m_name;
        int m_decoding_step;
        int m_start_off, m_end_off, m_stack_purge;
    };
    std::vector<function_info> m_list;
public:
    int rowCount(const QModelIndex &/*idx*/) const {return m_list.size();}
    int columnCount(const QModelIndex &/*idx*/) const {return 3;}
    QVariant data(const QModelIndex &,int role) const;
    void clear()
    {
        beginResetModel();
        m_list.clear();
        endResetModel();
    }
    QVariant headerData(int section, Qt::Orientation orientation,int role) const;
public slots:
    void updateFunctionList();

protected:
    void add_function(const QString &name,int step,int start_off,int end_off,int stack_purge)
    {

        function_info info;
        info.m_name=name;
        info.m_decoding_step=step;
        info.m_start_off=start_off;
        info.m_end_off=end_off;
        info.m_stack_purge=stack_purge;
        m_list.push_back(info);
    }
    void rebuildFunctionList();

};

namespace Ui {
class FunctionListDockWidget;
}

class FunctionListDockWidget : public QDockWidget
{
    Q_OBJECT

public:
    explicit FunctionListDockWidget(QWidget *parent = 0);
    ~FunctionListDockWidget();
    FunctionListModel *model() {return &m_list_model;}
public slots:
    void displayRequest(const QModelIndex &idx);
    void functionSelected(const QModelIndex &idx);

signals:
    void displayRequested();
private:
    Ui::FunctionListDockWidget *ui;
    FunctionListModel m_list_model;
};

#endif // FUNCTIONLISTDOCKWIDGET_H
