#ifndef DMA_H
#define DMA_H

#include "docboy/mmu/mmu.h"
#include "docboy/shared//macros.h"
#include <cstdint>

#include "docboy/bus/oambus.h"

class OamBus;
class Parcel;

class Dma {
    DEBUGGABLE_CLASS()

public:
    explicit Dma(Mmu::View<Device::Dma> mmu, OamBus::View<Device::Dma> oamBus);

    void startTransfer(uint16_t address);

    void tick_t1();
    void tick_t3();

    void saveState(Parcel& parcel) const;
    void loadState(Parcel& parcel);

    void reset();

private:
    struct RequestState {
        using Type = uint8_t;
        static constexpr Type Requested = 2;
        static constexpr Type Pending = 1;
        static constexpr Type None = 0;
    };

    Mmu::View<Device::Dma> mmu;
    OamBus::View<Device::Dma> oam;

    struct {
        RequestState::Type state {};
        uint16_t source {};
    } request;

    bool transferring {};
    uint16_t source {};
    uint8_t cursor {};
};

#endif // DMA_H