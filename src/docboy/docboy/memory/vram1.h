#ifndef VRAM1_H
#define VRAM1_H

#include "docboy/common/specs.h"
#include "docboy/memory/memory.h"

class Vram1 : public Memory<Specs::MemoryLayout::VRAM::START, Specs::MemoryLayout::VRAM::END> {};

#endif // VRAM0_H