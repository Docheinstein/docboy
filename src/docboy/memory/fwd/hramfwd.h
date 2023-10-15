#ifndef HRAMFWD_H
#define HRAMFWD_H

#include "docboy/shared/specs.h"
#include "memoryfwd.h"

using Hram = Memory<Specs::MemoryLayout::HRAM::START, Specs::MemoryLayout::HRAM::END>;

#endif // HRAMFWD_H