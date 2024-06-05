#ifndef HRAMFWD_H
#define HRAMFWD_H

#include "docboy/common/specs.h"
#include "docboy/memory/fwd/randmemoryfwd.h"

using Hram = RandomizedMemory<Specs::MemoryLayout::HRAM::START, Specs::MemoryLayout::HRAM::END>;

#endif // HRAMFWD_H