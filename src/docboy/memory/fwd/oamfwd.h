#ifndef OAMFWD_H
#define OAMFWD_H

#include "docboy/shared/specs.h"
#include "randmemoryfwd.h"

using Oam = RandomizedMemory<Specs::MemoryLayout::OAM::START, Specs::MemoryLayout::OAM::END>;

#endif // OAMFWD_H