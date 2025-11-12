#ifndef WRAM1_H
#define WRAM1_H

#include "docboy/common/specs.h"
#include "docboy/memory/memory.h"

class Wram1 : public Memory<Specs::MemoryLayout::WRAM1::START, Specs::MemoryLayout::WRAM1::END> {
public:
    void reset();
};

#endif // WRAM1_H