#ifndef FUNCTIONVIEWWIDGET_H
#define FUNCTIONVIEWWIDGET_H

#include "StructuredTextTarget.h"
#include "RenderTags.h"

#include <QWidget>
#include <QTextDocument>
#include <QTextBlockFormat>
//#include "XmlPrt.h"
namespace Ui {
class FunctionViewWidget;
}
class FunctionViewWidget : public QWidget,public IStructuredTextTarget
{
    Q_OBJECT
    QTextDocument *m_current_rendering;
    QTextCursor* m_doc_cursor;
    QTextBlockFormat m_current_format;
    QTextCharFormat m_chars_format;
public:
    explicit FunctionViewWidget(QWidget *parent = 0);
    ~FunctionViewWidget();
    void  prtt(const char * s);
    void  prtt(const QString &s);
    void  addEOL() override;
    void  TAGbegin(enum TAG_TYPE tag_type, void * p);
    void  TAGend(enum TAG_TYPE tag_type);
private:
    Ui::FunctionViewWidget *ui;
    QString collected_text;
};

#endif // FUNCTIONVIEWWIDGET_H
