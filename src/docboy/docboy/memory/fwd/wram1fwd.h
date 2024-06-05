#ifndef WRAM1FWD_H
#define WRAM1FWD_H

#include "docboy/common/specs.h"
#include "docboy/memory/fwd/randmemoryfwd.h"

using Wram1 = RandomizedMemory<Specs::MemoryLayout::WRAM1::START, Specs::MemoryLayout::WRAM1::END>;

#endif // WRAM1FWD_H