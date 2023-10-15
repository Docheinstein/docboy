#ifndef BOOTROMFWD_H
#define BOOTROMFWD_H

#include "docboy/memory/fwd/memoryfwd.h"
#include "docboy/shared/specs.h"

using BootRom = Memory<Specs::MemoryLayout::BOOTROM::START, Specs::MemoryLayout::BOOTROM::END>;

#endif // BOOTROMFWD_H