#include <QDebug>
#include <QtCore>
#include "FunctionViewWidget.h"
#include "ui_FunctionViewWidget.h"
#include "RenderTags.h"
//#include "XMLTYPE.h"
FunctionViewWidget::FunctionViewWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::FunctionViewWidget)
{
    ui->setupUi(this);
    //ui->label->setTextFormat(Qt::RichText);
}

FunctionViewWidget::~FunctionViewWidget()
{
    delete ui;
}

void FunctionViewWidget::prtt(const char *s)
{
    collected_text+=s;
    //collected_text+="<br>";
}
void FunctionViewWidget::prtt(const QString &s)
{
    collected_text+=s;
    //collected_text+="<br>";
}
void FunctionViewWidget::TAGbegin(TAG_TYPE tag_type, void *p)
{
    QColor col= RenderTag_2_Color(tag_type);
    switch(tag_type)
    {
        case XT_Function:
            collected_text+="<body style='color: #FFFFFF; background-color: #000000'>";
            break;
        case XT_FuncName:
        case XT_Symbol:
        case XT_Keyword:
        case XT_DataType:
        case XT_Number:
        case XT_AsmOffset:
        case XT_AsmLabel:
            collected_text+="<font color='"+col.name()+"'>";
            break;
        default:
            qDebug()<<"Tag type:"<<tag_type;
    }
}
void FunctionViewWidget::TAGend(TAG_TYPE tag_type)
{
    switch(tag_type)
    {
        case XT_Function:
        {
            collected_text+="</body>";
            // TODO: What about attributes with spaces?
            collected_text.replace("  ", "&nbsp;&nbsp;");
            QFile res("result.html");
            res.open(QFile::WriteOnly);
            res.write(collected_text.toUtf8());
            res.close();
            collected_text.replace(QChar('\n'),"<br>");
            ui->textEdit->setHtml(collected_text);
            collected_text.clear();
            break;
        }
        case XT_FuncName:
        case XT_Symbol:
        case XT_Keyword:
        case XT_DataType:
        case XT_Number:
        case XT_AsmOffset:
        case XT_AsmLabel:
            collected_text+="</font>";
            break;
        default:
            qDebug()<<"Tag end:"<<tag_type;
    }
}
