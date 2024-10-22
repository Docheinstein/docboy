#ifndef DMA_H
#define DMA_H

#include <cstdint>

#include "docboy/bus/oambus.h"
#include "docboy/common/macros.h"
#include "docboy/mmu/mmu.h"

class OamBus;
class Parcel;

class Dma {
    DEBUGGABLE_CLASS()

public:
    explicit Dma(Mmu::View<Device::Dma> mmu, OamBus::View<Device::Dma> oam_bus);

    void write_dma(uint8_t value);

    void tick_t1();
    void tick_t3();

    void save_state(Parcel& parcel) const;
    void load_state(Parcel& parcel);

    void reset();

    UInt8 dma {make_uint8(Specs::Registers::Video::DMA)};

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