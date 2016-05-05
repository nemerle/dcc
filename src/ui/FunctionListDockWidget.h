#ifndef FUNCTIONLISTDOCKWIDGET_H
#define FUNCTIONLISTDOCKWIDGET_H

#include <QAbstractTableModel>
#include <QDockWidget>

#include "Procedure.h"

enum DecompilationStep : uint32_t;
class FunctionListModel : public QAbstractTableModel
{
    Q_OBJECT

    struct function_info
    {
        QString m_name;
        DecompilationStep m_decoding_step;
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
    void add_function(const QString &name,DecompilationStep step,int start_off,int end_off,int stack_purge);
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
    void onDisplayRequested(const QModelIndex &idx);
    void onFunctionSelected(const QModelIndex &idx);

signals:
    void displayRequested();
    void selectFunction(PtrFunction p);
private:
    Ui::FunctionListDockWidget *ui;
    FunctionListModel m_list_model;
};

#endif // FUNCTIONLISTDOCKWIDGET_H
