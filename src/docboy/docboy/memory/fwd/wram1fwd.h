#ifndef WRAM1FWD_H
#define WRAM1FWD_H

#include "docboy/common/specs.h"
#include "docboy/memory/fwd/memoryfwd.h"

using Wram1 = Memory<Specs::MemoryLayout::WRAM1::START, Specs::MemoryLayout::WRAM1::END>;

#endif // WRAM1FWD_H