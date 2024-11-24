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
    Hdma(ExtBus::View<Device::Hdma> ext_bus, VramBus::View<Device::Hdma> vram_bus, const UInt8& stat_mode);

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

    bool has_active_or_pending_transfer() const {
        return active_or_pending_transfer;
    }

    void save_state(Parcel& parcel) const;
    void load_state(Parcel& parcel);

    void reset();

private:
    struct TransferMode {
        using Type = uint8_t;
        static constexpr Type GeneralPurpose = 1;
        static constexpr Type HBlank = 0;
    };

    struct TransferPhase {
        using Type = uint8_t;
        static constexpr Type Read = 0;
        static constexpr Type Write = 1;
    };

    struct TransferState {
        using Type = uint8_t;
        static constexpr Type Paused = 2;
        static constexpr Type Transferring = 1;
        static constexpr Type None = 0;
    };

    bool has_active_transfer() const;

    ExtBus::View<Device::Hdma> ext_bus;
    VramBus::View<Device::Hdma> vram;

    const UInt8& stat_mode;
    uint8_t last_stat_mode {};

    UInt8 hdma1 {make_uint8(Specs::Registers::Hdma::HDMA1)};
    UInt8 hdma2 {make_uint8(Specs::Registers::Hdma::HDMA2)};
    UInt8 hdma3 {make_uint8(Specs::Registers::Hdma::HDMA3)};
    UInt8 hdma4 {make_uint8(Specs::Registers::Hdma::HDMA4)};

    bool active_or_pending_transfer {};

    uint8_t request_delay {};

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
        uint8_t count {};
        uint8_t scheduled {};
        uint8_t delay {};
    } remaining_chunks;
};

#endif // HDMA_H
