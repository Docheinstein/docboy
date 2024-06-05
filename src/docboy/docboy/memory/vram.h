#ifndef VRAM_H
#define VRAM_H

#include "docboy/memory/fwd/vramfwd.h"
#include "docboy/memory/memory.h"

class Vram : public Memory<Specs::MemoryLayout::VRAM::START, Specs::MemoryLayout::VRAM::END> {
public:
    void reset();
};

#endif // VRAM_H