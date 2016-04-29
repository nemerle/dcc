#ifndef FUNCTIONVIEWWIDGET_H
#define FUNCTIONVIEWWIDGET_H

#include <QWidget>
#include "StructuredTextTarget.h"
#include "RenderTags.h"
//#include "XmlPrt.h"
namespace Ui {
class FunctionViewWidget;
}
class FunctionViewWidget : public QWidget,public IStructuredTextTarget
{
    Q_OBJECT

public:
    explicit FunctionViewWidget(QWidget *parent = 0);
    ~FunctionViewWidget();
    void  prtt(const char * s);
    void  prtt(const std::string &s);
    void  TAGbegin(enum TAG_TYPE tag_type, void * p);
    void  TAGend(enum TAG_TYPE tag_type);
private:
    Ui::FunctionViewWidget *ui;
    QString collected_text;
};

#endif // FUNCTIONVIEWWIDGET_H
