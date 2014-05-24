#pragma once
#include <QObject>
class Project;
class DccFrontend : public QObject
{
    Q_OBJECT
    void    LoadImage();
    void    parse(Project &proj);
    std::string m_fname;
public:
    explicit DccFrontend(QObject *parent = 0);
    bool FrontEnd();            /* frontend.c   */

signals:

public slots:
};
