#include <QtTest/QTest>

class ProjectTest : public QObject {
    Q_OBJECT
private slots:

    void testNewProjectIsInitalized();
    void testCreatedProjectHasValidNames();
    void initTestCase();
};
