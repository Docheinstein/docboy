#ifndef HRAMFWD_H
#define HRAMFWD_H

#include "docboy/shared/specs.h"
#include "randmemoryfwd.h"

using Hram = RandomizedMemory<Specs::MemoryLayout::HRAM::START, Specs::MemoryLayout::HRAM::END>;

#endif // HRAMFWD_H