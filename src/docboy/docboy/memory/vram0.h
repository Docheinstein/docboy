#ifndef VRAM0_H
#define VRAM0_H

#include "docboy/memory/fwd/vram0fwd.h"
#include "docboy/memory/memory.h"

class Vram0 : public Memory<Specs::MemoryLayout::VRAM::START, Specs::MemoryLayout::VRAM::END> {
public:
    void reset();
};

#endif // VRAM0_H