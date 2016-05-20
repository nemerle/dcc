#include "MemorySegmentCoordinator.h"

#include <boost/icl/interval_map.hpp>
#include <boost/icl/split_interval_map.hpp>
#include <utility>
using namespace boost::icl;
class MemorySegmentCoordinatorImpl {
    boost::icl::interval_map<LinearAddress,SegmentHolder> m_segmentation_map;
public:
    bool addSegment(LinearAddress base, LinearAddress start, LinearAddress fin, const char * name, int flags) {
        if(start>fin)
            return false;
        if(start<base)
            return false;
        MemorySegment *seg = new MemorySegment(base,start,fin);
        seg->setName(name);
        //
        auto segment_bounds(seg->bounds());
        m_segmentation_map.add(std::make_pair(
                                   interval<LinearAddress>::right_open(segment_bounds.first,segment_bounds.second),
                                   seg)
                              );
        return true;
    }
    uint32_t numberOfSegments() const { return interval_count(m_segmentation_map); }
    const MemorySegment *get(LinearAddress addr) {
        auto iter = m_segmentation_map.find(addr);
        if(iter==m_segmentation_map.end()) {
            return nullptr;
        }
        return iter->second;

    }
};

MemorySegmentCoordinator::MemorySegmentCoordinator()
{
    m_impl = new MemorySegmentCoordinatorImpl;
}

bool MemorySegmentCoordinator::addSegment(LinearAddress base, LinearAddress start, LinearAddress fin, const char * name, int flags)
{
    return m_impl->addSegment(base,start,fin,name,flags);
}

uint32_t MemorySegmentCoordinator::size()
{
    return m_impl->numberOfSegments();
}

MemorySegment *MemorySegmentCoordinator::getSegment(LinearAddress addr)
{
    return const_cast<MemorySegment *>(m_impl->get(addr));
}
