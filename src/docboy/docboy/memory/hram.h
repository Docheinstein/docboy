#ifndef HRAM_H
#define HRAM_H

#include "docboy/common/specs.h"
#include "docboy/memory/memory.h"

class Hram : public Memory<Specs::MemoryLayout::HRAM::START, Specs::MemoryLayout::HRAM::END> {
public:
    void reset();
};

#endif // HRAM_H