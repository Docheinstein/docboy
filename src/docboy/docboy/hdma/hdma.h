#ifndef HDMA_H
#define HDMA_H

#include <cstdint>

#include "docboy/bus/extbus.h"
#include "docboy/bus/oambus.h"
#include "docboy/bus/vrambus.h"
#include "docboy/common/macros.h"
#include "docboy/mmu/mmu.h"

class Parcel;
class Dma;
class SpeedSwitchController;

class Hdma {
    DEBUGGABLE_CLASS()

public:
    Hdma(MmuView<Device::Hdma> mmu, ExtBus::View<Device::Hdma> ext_bus, VramBus::View<Device::Hdma> vram_bus,
         OamBus::View<Device::Hdma> oam_bus, const UInt8& stat_mode, const bool& fetching, const bool& halted,
         const bool& stopped, SpeedSwitchController& speed_switch_controller);

    void write_hdma1(uint8_t value);
    void write_hdma2(uint8_t value);
    void write_hdma3(uint8_t value);
    void write_hdma4(uint8_t value);

    uint8_t read_hdma5() const;
    void write_hdma5(uint8_t value);

    void tick_t0();
    void tick_t1();
    void tick_t2();
    void tick_t3();

    bool is_active() const {
        return active;
    }

    void save_state(Parcel& parcel) const;
    void load_state(Parcel& parcel);

    void reset();

private:
    struct TransferMode {
        using Type = uint8_t;
        static constexpr Type HBlank = 0;
        static constexpr Type GeneralPurpose = 1;
    };

    struct TransferState {
        using Type = uint8_t;
        static constexpr Type None = 0;
        static constexpr Type Requested = 1;
        static constexpr Type Pending = 2;
        static constexpr Type Ready = 3;
        static constexpr Type Transferring = 4;
        static constexpr Type Paused = 5;
    };

    struct RemainingChunksUpdateState {
        using Type = uint8_t;
        static constexpr Type None = 0;
        static constexpr Type Requested = 1;
    };

    struct UnblockState {
        using Type = uint8_t;
        static constexpr Type None = 0;
        static constexpr Type Requested = 1;
    };

    void tick_even();
    void tick_odd();

    void tick_state();
    void tick_request();
    void tick_phase();

    Mmu::View<Device::Hdma> mmu;
    ExtBus::View<Device::Hdma> ext_bus;
    VramBus::View<Device::Hdma> vram;
    OamBus::View<Device::Hdma> oam;

    const UInt8& stat_mode;
    const bool& fetching;
    const bool& halted;
    const bool& stopped;

    // TODO: bad: shouldn't know speed_switch_controller?
    SpeedSwitchController& speed_switch_controller;

    UInt8 hdma1 {make_uint8(Specs::Registers::Hdma::HDMA1)};
    UInt8 hdma2 {make_uint8(Specs::Registers::Hdma::HDMA2)};
    UInt8 hdma3 {make_uint8(Specs::Registers::Hdma::HDMA3)};
    UInt8 hdma4 {make_uint8(Specs::Registers::Hdma::HDMA4)};

    uint8_t last_stat_mode {};
    bool last_halted {};

    bool active {};

    TransferState::Type state {};
    TransferMode::Type mode {};

    struct {
        uint16_t address {};
        uint16_t cursor {};
        bool valid {};
    } source;

    struct {
        uint16_t address {};
        uint16_t cursor {};
    } destination;

    uint16_t remaining_bytes {};

    struct {
        RemainingChunksUpdateState::Type state {};
        uint8_t count {};
    } remaining_chunks;

    UnblockState::Type unblock {};

    bool dma_oam_conflict {};
};

#endif // HDMA_H
