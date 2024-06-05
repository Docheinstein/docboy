#ifndef OAMFWD_H
#define OAMFWD_H

#include "docboy/common/specs.h"
#include "docboy/memory/fwd/randmemoryfwd.h"

using Oam = RandomizedMemory<Specs::MemoryLayout::OAM::START, Specs::MemoryLayout::OAM::END>;

#endif // OAMFWD_H