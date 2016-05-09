#include "FunctionViewWidget.h"
#include "ui_FunctionViewWidget.h"
#include "RenderTags.h"
#include "Procedure.h"

#include <QDebug>
#include <QtCore>
#include "dcc_interface.h"
extern IDcc* g_IDCC;
//#include "XMLTYPE.h"
FunctionViewWidget::FunctionViewWidget(PtrFunction func, QWidget *parent) :
    QWidget(parent),
    ui(new Ui::FunctionViewWidget),
    m_viewed_function(func)
{
    ui->setupUi(this);
    m_assembly_rendering = new QTextDocument(ui->textEdit);
    m_doc_cursor = new QTextCursor(m_assembly_rendering);
    ui->textEdit->setTextBackgroundColor(Qt::black);
    m_current_format =  m_doc_cursor->blockFormat();
    m_current_format.setNonBreakableLines(true); // each block is single line
    m_current_format.setBackground(Qt::black);
    m_chars_format.setBackground(Qt::black);
    m_chars_format.setForeground(Qt::white);
    //ui->label->setTextFormat(Qt::RichText);
}

FunctionViewWidget::~FunctionViewWidget()
{
    delete ui;
}

void FunctionViewWidget::renderCurrent()
{
    m_viewed_function->toStructuredText(this,0);
}

void FunctionViewWidget::prtt(const char *s)
{
    m_doc_cursor->insertText(s);
    collected_text+=s;
    //collected_text+="<br>";
}
void FunctionViewWidget::prtt(const QString &s)
{
    m_doc_cursor->insertText(s);
    collected_text+=s;
    //collected_text+="<br>";
}
void FunctionViewWidget::delChars(int v) {
    assert(v>0);
    collected_text = collected_text.remove(collected_text.size()-v,v);
    while(v--)
        m_doc_cursor->deletePreviousChar();
}
void FunctionViewWidget::addEOL()
{
    m_doc_cursor->insertBlock(m_current_format);
    m_doc_cursor->setBlockFormat(m_current_format);
}
void FunctionViewWidget::TAGbegin(TAG_TYPE tag_type, void *p)
{
    QColor col= RenderTag_2_Color(tag_type);

    switch(tag_type)
    {
        case XT_Function:
            m_assembly_rendering->clear();
            m_chars_format.setForeground(Qt::white);
            m_doc_cursor->setBlockFormat(m_current_format);
            m_doc_cursor->setCharFormat(m_chars_format);
            break;
        case XT_FuncName:
        case XT_Symbol:
        case XT_Keyword:
        case XT_DataType:
        case XT_Number:
        case XT_AsmOffset:
        case XT_AsmLabel:
            m_chars_format.setForeground(col);
            m_doc_cursor->setCharFormat(m_chars_format);
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
            QFile res("result.html");
            res.open(QFile::WriteOnly);
            res.write(m_assembly_rendering->toHtml().toUtf8());
            res.close();
            ui->textEdit->setDocument(m_assembly_rendering);
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
            m_chars_format.setForeground(Qt::white);
            m_doc_cursor->setCharFormat(m_chars_format);
            m_doc_cursor->setBlockFormat(m_current_format);
            break;
        default:
            qDebug()<<"Tag end:"<<tag_type;
    }
}
void FunctionViewWidget::on_tabWidget_currentChanged(int index)
{
    renderCurrent();
}
