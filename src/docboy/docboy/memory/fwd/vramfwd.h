#ifndef VRAMFWD_H
#define VRAMFWD_H

#include "docboy/common/specs.h"
#include "docboy/memory/fwd/memoryfwd.h"

using Vram = Memory<Specs::MemoryLayout::VRAM::START, Specs::MemoryLayout::VRAM::END>;

#endif // VRAMFWD_H