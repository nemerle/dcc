#ifndef BYTECHUNK_H
#define BYTECHUNK_H

#include "Address.h"

#include <utility>
#include <inttypes.h>
/**
 * @brief The MemoryChunk class represents a continuous range of Addresses
 */
class MemoryChunk
{
private:
    LinearAddress m_start;
    LinearAddress m_fin;
public:
    MemoryChunk(LinearAddress start,LinearAddress fin);
    bool contains(LinearAddress addr) const;
    uint64_t size() const;

    std::pair<LinearAddress,LinearAddress> bounds() const { return std::make_pair(m_start,m_fin); }
};

#endif // BYTECHUNK_H
