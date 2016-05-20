#include "MemorySegmentCoordinatorTests.h"

#include "MemorySegmentCoordinator.h"
#include "project.h"
#include "loader.h"

#include <QTextStream>
#include <QStringList>
#include <QDir>
#include <QProcessEnvironment>
#include <QDebug>

void MemorySegmentCoordinatorTest::testSimpleQueries()
{
    MemorySegmentCoordinator segmenter;
    segmenter.addSegment(
                LinearAddress(0x10),
                LinearAddress(0x13),LinearAddress(0x20),
                ".text",4);
    MemorySegment * seg;
    QCOMPARE(segmenter.getSegment(LinearAddress(0x9)),(MemorySegment *)nullptr);
    seg = segmenter.getSegment(0x14);
    QVERIFY(seg!=nullptr);
    if(seg) {
        QCOMPARE(seg->getName(),QString(".text"));
    }
}
void MemorySegmentCoordinatorTest::testAddingSegments()
{
    MemorySegmentCoordinator segmenter;
    QVERIFY(segmenter.addSegment(
                LinearAddress(0x10),
                LinearAddress(0x13),LinearAddress(0x20),
                ".text",4));
    QCOMPARE(segmenter.size(),uint32_t(1));
    QVERIFY(segmenter.addSegment(
                LinearAddress(0x20),
                LinearAddress(0x22),LinearAddress(0x33),
                ".text",4));
    QVERIFY2(not segmenter.addSegment(
                LinearAddress(0x20),
                LinearAddress(0x40),LinearAddress(0x20),
                ".text",4),"Adding segment with start>fin should fail");
    QVERIFY2(not segmenter.addSegment(
                LinearAddress(0x20),
                LinearAddress(0x10),LinearAddress(0x20),
                ".text",4),"Segment start should be >= base");
    QCOMPARE(segmenter.size(),uint32_t(2));
}
void MemorySegmentCoordinatorTest::testAddingIntersectingSegments()
{

}

QTEST_MAIN(MemorySegmentCoordinatorTest)
