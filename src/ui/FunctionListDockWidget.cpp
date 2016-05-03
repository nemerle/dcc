#include "FunctionListDockWidget.h"
#include "ui_FunctionListDockWidget.h"

#include "dcc.h"
#include "dcc_interface.h"
#include "Procedure.h"
#include "project.h"

#include <QtCore>
//#include "exe2c.h"
extern IDcc *g_EXE2C;
FunctionListDockWidget::FunctionListDockWidget(QWidget *parent) :
    QDockWidget(parent),
    ui(new Ui::FunctionListDockWidget)
{
    ui->setupUi(this);
    ui->m_func_list_view->setModel(&m_list_model);
}

FunctionListDockWidget::~FunctionListDockWidget()
{
    delete ui;
}
void FunctionListDockWidget::functionSelected(const QModelIndex &idx)
{

    QVariant v=m_list_model.data(m_list_model.index(idx.row(),0),Qt::DisplayRole);
    qDebug()<<"changed function to "<<v;
    g_EXE2C->SetCurFunc_by_Name(v.toString());
}
// signalled by m_func_list_view accepted signal
void FunctionListDockWidget::displayRequest(const QModelIndex &)
{
    // argument ignored since functionSelected must've been called before us
    emit displayRequested();
}
void FunctionListModel::updateFunctionList()
{
    rebuildFunctionList();
}
void FunctionListModel::rebuildFunctionList()
{
    Project &project(*Project::get());
    const lFunction &funcs(project.functions());
    clear();
    beginInsertRows(QModelIndex(),0,funcs.size());
    for(const PtrFunction &info : funcs)
    {
        //g_EXE2C->GetFuncInfo(iter, &info);
        if (info->name.isEmpty())
            continue;
        // fixme
        add_function(info->name,info->nStep,info->procEntry,info->procEntry+10,info->cbParam);
    }
    endInsertRows();
}
QVariant FunctionListModel::data(const QModelIndex &idx,int role) const
{
    int row=idx.row();
    int column=idx.column();
    const function_info &inf=m_list[row];
    if(Qt::DisplayRole==role)
    {
        switch(column)
        {
        case 0: // name
        {
            return QVariant(inf.m_name);
        }
        case 1: { // step
            switch(inf.m_decoding_step) {
            case eNotDecoded:
                return "Undecoded";
            case eDisassemblyInProgress:
                return "Disassembly in progress";
            case eDissassembled:
                return "Disassembled";
            default:
                return "UNKNOWN STATE";
            }
        }
        case 2: // start offset
        {
            QString in_base_16=QString("%1").arg(inf.m_start_off,0,16);
            return QVariant(in_base_16);
        }
        default:
            return QVariant();

        }
    }
    return QVariant();
}
QVariant FunctionListModel::headerData(int section, Qt::Orientation orientation,int role) const
{
    if(Qt::DisplayRole==role && orientation==Qt::Horizontal)
    {
        switch(section)
        {
        case 0: // name
            return QObject::tr("Function name");
        case 1: // step
            return QObject::tr("Decoding step");
        case 2: // start offset
            return QObject::tr("Start offset");
        default:
            return QVariant();

        }
    }
    return QVariant();
}
