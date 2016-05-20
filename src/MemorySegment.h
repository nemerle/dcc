#pragma once

#include "MemoryChunk.h"

#include <QtCore/QString>
/**
 * @brief The MemorySegment represents a single chunk of memory with additional properties.
 */
class MemorySegment : public MemoryChunk
{
    uint16_t m_base;
    int m_flags;
    QString m_name;
public:
    MemorySegment(LinearAddress base,LinearAddress start,LinearAddress fin);
    const QString &getName() const { return m_name; }
    void setName(const QString &v) { m_name = v; }
};

