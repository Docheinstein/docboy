#ifndef OAMFWD_H
#define OAMFWD_H

#include "docboy/shared/specs.h"
#include "memoryfwd.h"

using Oam = Memory<Specs::MemoryLayout::OAM::START, Specs::MemoryLayout::OAM::END>;

#endif // OAMFWD_H