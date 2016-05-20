#include "MemoryChunk.h"

#include <boost/icl/interval.hpp>
#include <boost/icl/right_open_interval.hpp>
#include <boost/icl/left_open_interval.hpp>
#include <boost/icl/closed_interval.hpp>
#include <boost/icl/open_interval.hpp>

using namespace boost::icl;
MemoryChunk::MemoryChunk(LinearAddress start, LinearAddress fin) : m_start(start),m_fin(fin)
{
}

bool MemoryChunk::contains(LinearAddress addr) const
{
    return addr>=m_start && addr<m_fin;
}

uint64_t MemoryChunk::size() const
{
    return m_fin-m_start;
}
