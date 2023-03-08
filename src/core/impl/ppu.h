#ifndef PPUIMPL_H
#define PPUIMPL_H

#if ENABLE_DEBUGGER
#include "core/debugger/ppu/ppu.h"
namespace Impl {
using IPPU = IDebuggablePPU;
using PPU = DebuggablePPU;
}
#else
#include "core/ppu/ppu.h"
namespace Impl {
using IPPU = IPPU;
using PPU = PPU;
}
#endif

#endif // PPUIMPL_H