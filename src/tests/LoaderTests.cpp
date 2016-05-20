#include "LoaderTests.h"

#include "project.h"
#include "loader.h"

#include <QTextStream>
#include <QStringList>
#include <QDir>
#include <QProcessEnvironment>
#include <QDebug>

void LoaderTest::testDummy() {
    QVERIFY2(false,"No tests written for loader");
}
QTEST_MAIN(LoaderTest)
