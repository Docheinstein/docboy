#ifndef VRAM1FWD_H
#define VRAM1FWD_H

#include "docboy/common/specs.h"
#include "docboy/memory/fwd/memoryfwd.h"

using Vram1 = Memory<Specs::MemoryLayout::VRAM::START, Specs::MemoryLayout::VRAM::END>;

#endif // VRAM1FWD_H