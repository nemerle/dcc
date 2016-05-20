#include "MemorySegment.h"

MemorySegment::MemorySegment(LinearAddress base, LinearAddress start, LinearAddress fin) : MemoryChunk(start,fin) {
    m_base = base;
}
