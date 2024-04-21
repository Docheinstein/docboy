#ifndef VRAM_H
#define VRAM_H

#include "fwd/vramfwd.h"
#include "memory.hpp"

class Vram : public Memory<Specs::MemoryLayout::VRAM::START, Specs::MemoryLayout::VRAM::END> {
public:
    void reset();
};

#endif // VRAM_H