#pragma once

#include "MemorySegment.h"

struct SegmentHolder {
    SegmentHolder() : val(nullptr) {}
    SegmentHolder(MemorySegment *inf) : val(inf) {}

    MemorySegment *operator->() { return val;}
    MemorySegment &operator*() const { return *val;}
    operator MemorySegment *() { return val;}
    operator const MemorySegment *() const { return val;}
    SegmentHolder operator+=(const SegmentHolder &/*s*/) {
        throw std::runtime_error("Cannot aggregate MemorySegments !");
    }

    MemorySegment *val;
};

/**
 * @brief The MemorySegmentCoordinator class is responsible for:
 * - Managing the lifetime of MemorySegments
 * - Providing convenience functions for querying the segment-related data
 */
class MemorySegmentCoordinator
{
    class MemorySegmentCoordinatorImpl *m_impl;
public:
    MemorySegmentCoordinator();

    bool addSegment(LinearAddress base,LinearAddress start,LinearAddress fin,const char *name,int flags);
    uint32_t size();
    MemorySegment *getSegment(LinearAddress addr);
};
