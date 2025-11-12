#ifndef HRAM_H
#define HRAM_H

#include "docboy/common/specs.h"
#include "docboy/memory/memory.h"

struct CartridgeHeader;

class Hram : public Memory<Specs::MemoryLayout::HRAM::START, Specs::MemoryLayout::HRAM::END> {
public:
#if defined(ENABLE_CGB) && !defined(ENABLE_BOOTROM)
    explicit Hram(const CartridgeHeader& header);
#endif

    void reset();

private:
#if defined(ENABLE_CGB) && !defined(ENABLE_BOOTROM)
    const CartridgeHeader& header;
#endif
};

#endif // HRAM_H