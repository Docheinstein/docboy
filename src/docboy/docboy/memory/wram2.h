#ifndef WRAM2_H
#define WRAM2_H

#include "docboy/common/specs.h"
#include "docboy/memory/memory.h"

class Wram2 : public Memory<Specs::MemoryLayout::WRAM2::START, Specs::MemoryLayout::WRAM2::END> {
public:
    void reset();
};

#endif // WRAM2_H