#ifndef HRAMFWD_H
#define HRAMFWD_H

#include "docboy/common/specs.h"
#include "docboy/memory/fwd/memoryfwd.h"

using Hram = Memory<Specs::MemoryLayout::HRAM::START, Specs::MemoryLayout::HRAM::END>;

#endif // HRAMFWD_H