#ifndef DMA_H
#define DMA_H

#include "docboy/debugger/macros.h"
#include "docboy/memory/fwd/oamfwd.h"
#include "docboy/mmu/mmu.h"
#include <cstdint>

class OamBus;
class Parcel;

class Dma {
    DEBUGGABLE_CLASS()

public:
    using OamBusView = AcquirableBusView<OamBus, AcquirableBusDevice::Dma>;

    explicit Dma(MmuSocket<MmuDevice::Dma> mmu, OamBusView oamBus);

    void startTransfer(uint16_t address);

    void tickRead();
    void tickWrite();

    void saveState(Parcel& parcel) const;
    void loadState(Parcel& parcel);

private:
    struct RequestState {
        using Type = uint8_t;
        static constexpr Type Requested = 2;
        static constexpr Type Pending = 1;
        static constexpr Type None = 0;
    };

    MmuView<MmuDevice::Dma> mmu;
    OamBusView oam;

    struct {
        RequestState::Type state {};
        uint16_t source {};
    } request;

    bool transferring {};
    uint16_t source {};
    uint8_t cursor {};

    uint8_t busData {};
};

#endif // DMA_H