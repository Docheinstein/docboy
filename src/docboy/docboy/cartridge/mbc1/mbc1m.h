#ifndef MBC1M_H
#define MBC1M_H

#include "mbc1.h"

template <uint32_t RomSize, uint32_t RamSize, bool Battery>
using Mbc1M = Mbc1<RomSize, RamSize, Battery, 4>;

#endif // MBC1M_H
