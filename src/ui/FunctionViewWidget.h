#ifndef FUNCTIONVIEWWIDGET_H
#define FUNCTIONVIEWWIDGET_H

#include "StructuredTextTarget.h"
#include "RenderTags.h"

#include <memory>
#include <QWidget>
#include <QTextDocument>
#include <QTextBlockFormat>

class Function;
typedef std::shared_ptr<Function> PtrFunction;
namespace Ui {
class FunctionViewWidget;
}
//TODO: convert 'controllers' of each  hex/asm/high-level areas into standalone classes
class FunctionViewWidget : public QWidget,public IStructuredTextTarget
{
    Q_OBJECT
    PtrFunction m_viewed_function; // 'model' this view is bound to
    QTextDocument *m_assembly_rendering;
    QTextCursor* m_doc_cursor;
    QTextBlockFormat m_current_format;
    QTextCharFormat m_chars_format;
public:
    explicit FunctionViewWidget(PtrFunction func,QWidget *parent = 0);
    ~FunctionViewWidget();
    const PtrFunction &viewedFunction() const { return m_viewed_function; }
    void renderCurrent();

    // IStructuredTextTarget interface
    void  prtt(const char * s);
    void  prtt(const QString &s);
    void  addEOL() override;
    void  TAGbegin(enum TAG_TYPE tag_type, void * p);
    void  TAGend(enum TAG_TYPE tag_type);


private slots:
    void on_tabWidget_currentChanged(int index);

private:
    Ui::FunctionViewWidget *ui;
    QString collected_text;
};

#endif // FUNCTIONVIEWWIDGET_H
