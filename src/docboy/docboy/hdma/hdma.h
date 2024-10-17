#ifndef HDMA_H
#define HDMA_H

#include <cstdint>

#include "docboy/bus/vrambus.h"
#include "docboy/common/macros.h"
#include "docboy/mmu/mmu.h"

class OamBus;
class Parcel;

class Hdma {
    DEBUGGABLE_CLASS()

public:
    explicit Hdma(Mmu::View<Device::Hdma> mmu, VramBus::View<Device::Hdma> vram_bus);

    void start_transfer(uint16_t source, uint16_t destination, uint16_t length);

    void tick_t0();
    void tick_t1();
    void tick_t2();
    void tick_t3();

    void save_state(Parcel& parcel) const;
    void load_state(Parcel& parcel);

    void reset();

    bool is_transferring() const {
        return transferring;
    }

private:
    Mmu::View<Device::Hdma> mmu;
    VramBus::View<Device::Hdma> vram;

    bool transferring {};
    uint16_t source {};
    uint16_t destination {};
    uint16_t length {};
    uint16_t cursor {};
};

#endif // HDMA_H
