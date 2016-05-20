#include "MemoryChunkTests.h"

#include "MemoryChunk.h"
#include "project.h"
#include "loader.h"

#include <QTextStream>
#include <QStringList>
#include <QDir>
#include <QProcessEnvironment>
#include <QDebug>

void MemoryChunkTest::testIfConstructionWorksProperly() {
    MemoryChunk mc(0,10);
    QCOMPARE(mc.size(),size_t(10));
    QVERIFY(mc.contains(1));
    QVERIFY(not mc.contains(10));
    QVERIFY(not mc.contains(-1));
    QVERIFY(not mc.contains(100));
}

QTEST_MAIN(MemoryChunkTest)
