#include "ProjectTests.h"

#include "project.h"

#include <QTextStream>
#include <QStringList>
#include <QDir>
#include <QProcessEnvironment>
#include <QDebug>

static bool logset = false;
static QString TEST_BASE;
static QDir baseDir;

void ProjectTest::initTestCase() {
    if (!logset) {
        TEST_BASE = QProcessEnvironment::systemEnvironment().value("DCC_TEST_BASE", "");
        baseDir = QDir(TEST_BASE);
        if (TEST_BASE.isEmpty()) {
            qWarning() << "DCC_TEST_BASE environment variable not set, will assume '..', many test may fail";
            TEST_BASE = "..";
            baseDir = QDir("..");
        }
        logset = true;
//        Boomerang::get()->setProgPath(TEST_BASE);
//        Boomerang::get()->setPluginPath(TEST_BASE + "/out");
//        Boomerang::get()->setLogger(new NullLogger());
    }
}
void ProjectTest::testNewProjectIsInitalized() {
    Project p;
    QCOMPARE((CALL_GRAPH *)nullptr,p.callGraph);
    QVERIFY(p.pProcList.empty());
    QVERIFY(p.binary_path().isEmpty());
    QVERIFY(p.project_name().isEmpty());
    QVERIFY(p.symtab.empty());
}

void ProjectTest::testCreatedProjectHasValidNames() {
    Project p;
    QStringList strs     = {"./Project1.EXE","/home/Project2.EXE","/home/Pro\\ ject3"};
    QStringList expected = {"Project1","Project2","Pro\\ ject3"};
    for(size_t i=0; i<strs.size(); i++)
    {
        p.create(strs[i]);
        QCOMPARE((CALL_GRAPH *)nullptr,p.callGraph);
        QVERIFY(p.pProcList.empty());
        QCOMPARE(expected[i],p.project_name());
        QCOMPARE(strs[i],p.binary_path());
        QVERIFY(p.symtab.empty());
    }
}
QTEST_MAIN(ProjectTest)
