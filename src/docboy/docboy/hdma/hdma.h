#ifndef HDMA_H
#define HDMA_H

#include <cstdint>

#include "docboy/bus/extbus.h"
#include "docboy/bus/vrambus.h"
#include "docboy/common/macros.h"
#include "docboy/mmu/mmu.h"

class Parcel;

class Hdma {
    DEBUGGABLE_CLASS()

public:
    Hdma(MmuView<Device::Hdma> mmu, ExtBus::View<Device::Hdma> ext_bus, VramBus::View<Device::Hdma> vram_bus,
         const UInt8& stat_mode, const bool& fetching, const bool& halted, const bool& stopped);

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

    void tick();

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

    struct TransferPhase {
        using Type = uint8_t;
        static constexpr Type Tick = 0;
        static constexpr Type Tock = 1;
    };

    struct TransferState {
        using Type = uint8_t;
        static constexpr Type None = 0;
        static constexpr Type Requested = 1;
        static constexpr Type Ready = 2;
        static constexpr Type Transferring = 3;
        static constexpr Type Paused = 4;
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

    Mmu::View<Device::Hdma> mmu;
    ExtBus::View<Device::Hdma> ext_bus;
    VramBus::View<Device::Hdma> vram;
    const UInt8& stat_mode;
    const bool& fetching;
    const bool& halted;
    const bool& stopped;

    UInt8 hdma1 {make_uint8(Specs::Registers::Hdma::HDMA1)};
    UInt8 hdma2 {make_uint8(Specs::Registers::Hdma::HDMA2)};
    UInt8 hdma3 {make_uint8(Specs::Registers::Hdma::HDMA3)};
    UInt8 hdma4 {make_uint8(Specs::Registers::Hdma::HDMA4)};

    uint8_t last_stat_mode {};
    bool last_halted {};

    bool active {};

    TransferState::Type state {};
    TransferMode::Type mode {};
    TransferPhase::Type phase {};

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
};

#endif // HDMA_H
