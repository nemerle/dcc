#include <QtTest/QTest>

class MemorySegmentCoordinatorTest : public QObject {
    Q_OBJECT
private slots:
    void testAddingSegments();
    void testAddingIntersectingSegments();
    void testSimpleQueries();
};
