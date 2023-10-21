#ifndef DMA_H
#define DMA_H

#include "docboy/debugger/macros.h"
#include "docboy/memory/fwd/oamfwd.h"
#include <cstdint>

class Bus;

class Parcel;

class Dma {
public:
    DEBUGGABLE_CLASS();

    explicit Dma(Bus& bus, Oam& oam);

    void transfer(uint16_t address);
    void tick();

    void saveState(Parcel& parcel) const;
    void loadState(Parcel& parcel);

private:
    Bus& bus;
    Oam& oam;

    bool active {};
    uint16_t source {};
    uint8_t cursor {};
};

#endif // DMA_H