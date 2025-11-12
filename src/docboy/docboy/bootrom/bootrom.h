#ifndef BOOTROM_H
#define BOOTROM_H

#include "docboy/common/specs.h"
#include "docboy/memory/memory.h"

using BootRom = Memory<Specs::MemoryLayout::BOOTROM::START, Specs::MemoryLayout::BOOTROM::END>;

#endif // BOOTROM_H