#ifndef VRAMFWD_H
#define VRAMFWD_H

#include "docboy/shared/specs.h"
#include "memoryfwd.h"

using Vram = Memory<Specs::MemoryLayout::VRAM::START, Specs::MemoryLayout::VRAM::END>;

#endif // VRAMFWD_H