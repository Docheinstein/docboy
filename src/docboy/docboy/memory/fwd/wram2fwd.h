#ifndef WRAM2FWD_H
#define WRAM2FWD_H

#include "docboy/common/specs.h"
#include "docboy/memory/fwd/randmemoryfwd.h"

using Wram2 = RandomizedMemory<Specs::MemoryLayout::WRAM2::START, Specs::MemoryLayout::WRAM2::END>;

#endif // WRAM2FWD_H