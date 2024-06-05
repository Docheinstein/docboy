#ifndef BOOTROMFWD_H
#define BOOTROMFWD_H

#include "docboy/common/specs.h"
#include "docboy/memory/fwd/memoryfwd.h"

using BootRom = Memory<Specs::MemoryLayout::BOOTROM::START, Specs::MemoryLayout::BOOTROM::END>;

#endif // BOOTROMFWD_H