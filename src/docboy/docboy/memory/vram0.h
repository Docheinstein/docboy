#ifndef VRAM0_H
#define VRAM0_H

#include "docboy/common/specs.h"
#include "docboy/memory/memory.h"

struct CartridgeHeader;

class Vram0 : public Memory<Specs::MemoryLayout::VRAM::START, Specs::MemoryLayout::VRAM::END> {
public:
#if defined(ENABLE_CGB) && !defined(ENABLE_BOOTROM)
    explicit Vram0(const CartridgeHeader& header);
#endif

    void reset();

private:
#if defined(ENABLE_CGB) && !defined(ENABLE_BOOTROM)
    const CartridgeHeader& header;
#endif
};

#endif // VRAM0_H