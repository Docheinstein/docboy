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

    void resume();

    void tick_t0();
    void tick_t1();
    void tick_t2();
    void tick_t3();

    void save_state(Parcel& parcel) const;
    void load_state(Parcel& parcel);

    void reset();

    bool is_active() const {
        return transferring || request == RequestState::Pending || pause == PauseState::ResumePending;
    }

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

    struct Hdma5 : Composite<Specs::Registers::Hdma::HDMA5> {
        Bool hblank_mode {make_bool()};
        UInt8 length {make_uint8()};
    } hdma5 {};

    RequestState::Type request {};
    TransferMode::Type mode {};
    PauseState::Type pause {};

    bool transferring {};

    uint16_t source {};
    uint16_t destination {};
    uint16_t length {};
    uint16_t cursor {};

    struct {
        RemainingChunksUpdateState::Type state {};
        uint8_t count {};
    } remaining_chunks;
};

#endif // HDMA_H
