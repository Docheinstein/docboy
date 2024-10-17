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

    void start_hdma_transfer(uint16_t source, uint16_t destination, uint16_t length);
    void start_gdma_transfer(uint16_t source, uint16_t destination, uint16_t length);

    void resume_hdma_transfer();

    void tick_t0();
    void tick_t1();
    void tick_t2();
    void tick_t3();

    void save_state(Parcel& parcel) const;
    void load_state(Parcel& parcel);

    void reset();

    bool is_transferring() const {
        return transfer_state >= TransferState::HdmaTransferring;
    }

private:
    Mmu::View<Device::Hdma> mmu;
    VramBus::View<Device::Hdma> vram;

    struct TransferState {
        using Type = uint8_t;
        static constexpr Type None = 0;
        static constexpr Type HdmaPaused = 1;
        static constexpr Type HdmaTransferring = 2;
        static constexpr Type GdmaTransferring = 3;
    };

    TransferState::Type transfer_state {};
    uint16_t source {};
    uint16_t destination {};
    uint16_t length {};
    uint16_t cursor {};
};

#endif // HDMA_H
