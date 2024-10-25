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

    void resume();

    bool has_active_or_pending_transfer() const {
        return transferring || request == RequestState::Pending || pause == PauseState::ResumePending;
    }

    bool has_active_transfer() const;

    void save_state(Parcel& parcel) const;
    void load_state(Parcel& parcel);

    void reset();

private:
    struct RequestState {
        using Type = uint8_t;
        static constexpr Type Requested = 2;
        static constexpr Type Pending = 1;
        static constexpr Type None = 0;
    };

    struct PauseState {
        using Type = uint8_t;
        static constexpr Type ResumeRequested = 3;
        static constexpr Type ResumePending = 2;
        static constexpr Type None = 1;
        static constexpr Type Paused = 0;
    };

    struct TransferMode {
        using Type = uint8_t;
        static constexpr Type GeneralPurpose = 1;
        static constexpr Type HBlank = 0;
    };

    struct RemainingChunksUpdateState {
        using Type = uint8_t;
        static constexpr Type Requested = 2;
        static constexpr Type Pending = 1;
        static constexpr Type None = 0;
    };

    Mmu::View<Device::Hdma> mmu;
    VramBus::View<Device::Hdma> vram;

    UInt8 hdma1 {make_uint8(Specs::Registers::Hdma::HDMA1)};
    UInt8 hdma2 {make_uint8(Specs::Registers::Hdma::HDMA2)};
    UInt8 hdma3 {make_uint8(Specs::Registers::Hdma::HDMA3)};
    UInt8 hdma4 {make_uint8(Specs::Registers::Hdma::HDMA4)};

    RequestState::Type request {};
    TransferMode::Type mode {};
    PauseState::Type pause {};

    bool transferring {};

    struct {
        uint16_t address {};
        uint16_t cursor {};
    } source;

    struct {
        uint16_t address {};
        uint16_t cursor {};
    } destination;

    uint16_t remaining_bytes {};

    struct {
        RemainingChunksUpdateState::Type state {};
        uint16_t count {};
    } remaining_chunks;
};

#endif // HDMA_H
