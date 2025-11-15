#include "docboy/timers/timers.h"

#include "docboy/bootrom/helpers.h"
#include "docboy/cartridge/header.h"
#include "docboy/interrupts/interrupts.h"

#if defined(ENABLE_CGB) && !defined(ENABLE_BOOTROM)
#include "docboy/cartridge/header.h"
#include <optional>
#endif

#include "utils/asserts.h"
#include "utils/parcel.h"

namespace {
#if defined(ENABLE_CGB) && !defined(ENABLE_BOOTROM)
constexpr uint16_t DMG_MODE_CHECKSUM_DIV16[256] = {
    0x28D4, 0x388C, 0x3784, 0x3784, 0x3784, 0x3784, 0x3784, 0x3784, 0x3784, 0x3784, 0x3784, 0x3784, 0x3634, 0x0000,
    0x3784, 0x3784, 0x34CC, 0x3784, 0x3784, 0x3784, 0x2E44, 0x3144, 0x2D94, 0x347C, 0x0000, 0x29A4, 0x3784, 0x3784,
    0x3784, 0x2954, 0x3784, 0x3784, 0x3784, 0x3784, 0x3784, 0x3784, 0x3784, 0x3784, 0x3784, 0x0000, 0x0000, 0x376C,
    0x3784, 0x3784, 0x3784, 0x3784, 0x3784, 0x3784, 0x3784, 0x3784, 0x3784, 0x3784, 0x2AE4, 0x30F4, 0x2D4C, 0x3784,
    0x3784, 0x2DF4, 0x3784, 0x3784, 0x2AD4, 0x27D4, 0x328C, 0x34A4, 0x3784, 0x3784, 0x3784, 0x34B4, 0x3784, 0x3784,
    0x0000, 0x3784, 0x3784, 0x34DC, 0x3784, 0x2D14, 0x3784, 0x3784, 0x356C, 0x3784, 0x3784, 0x3784, 0x385C, 0x3784,
    0x3784, 0x3784, 0x3784, 0x3784, 0x356C, 0x2F8C, 0x3784, 0x3784, 0x2F6C, 0x397C, 0x3784, 0x3784, 0x3784, 0x0000,
    0x3784, 0x3784, 0x3784, 0x3784, 0x0000, 0x38D4, 0x364C, 0x336C, 0x0000, 0x39DC, 0x3784, 0x39AC, 0x3784, 0x3394,
    0x314C, 0x3664, 0x3784, 0x3784, 0x3784, 0x31B4, 0x3784, 0x3784, 0x3784, 0x3784, 0x3784, 0x3784, 0x3784, 0x3784,
    0x3784, 0x3784, 0x3784, 0x3784, 0x3784, 0x3784, 0x3784, 0x3784, 0x363C, 0x3784, 0x2C24, 0x3784, 0x3784, 0x368C,
    0x29A4, 0x3784, 0x3784, 0x3784, 0x2D44, 0x3784, 0x2EE4, 0x3784, 0x3784, 0x347C, 0x3784, 0x2D04, 0x3784, 0x3214,
    0x3104, 0x3784, 0x3484, 0x387C, 0x3784, 0x3784, 0x3784, 0x3784, 0x35EC, 0x3784, 0x3784, 0x0000, 0x3784, 0x3784,
    0x306C, 0x3784, 0x2FF4, 0x3784, 0x3784, 0x3784, 0x3784, 0x3784, 0x3784, 0x3784, 0x3784, 0x0000, 0x3784, 0x3784,
    0x3784, 0x36C4, 0x3784, 0x3784, 0x3784, 0x3784, 0x3784, 0x3224, 0x3784, 0x0000, 0x3784, 0x3784, 0x3784, 0x3784,
    0x3784, 0x3784, 0x0000, 0x3784, 0x3784, 0x2FEC, 0x3784, 0x3784, 0x3784, 0x3784, 0x356C, 0x3784, 0x3784, 0x2D5C,
    0x3784, 0x0000, 0x3784, 0x3784, 0x3784, 0x3784, 0x3784, 0x3784, 0x3784, 0x2CC4, 0x3784, 0x3784, 0x3784, 0x3784,
    0x382C, 0x3784, 0x3784, 0x3784, 0x3784, 0x3784, 0x3784, 0x3784, 0x36B4, 0x3784, 0x3784, 0x3784, 0x3784, 0x3784,
    0x3784, 0x3784, 0x353C, 0x3784, 0x312C, 0x3784, 0x0000, 0x3784, 0x355C, 0x358C, 0x3784, 0x3784, 0x3784, 0x3784,
    0x3784, 0x3784, 0x3784, 0x3154};

constexpr std::pair<std::optional<uint8_t>, uint16_t> DMG_MODE_AMBIGUOUS_CHECKSUM_DIV16[256][4] = {
    {},
    {},
    {},
    {},
    {},
    {},
    {},
    {},
    {},
    {},
    {},
    {},
    {},
    /* 0x0D */ {{'R', 0x3ec4}, {'E', 0x35e8}, {std::nullopt, 0x381c}},
    {},
    {},
    {},
    {},
    {},
    {},
    {},
    {},
    {},
    {},
    /* 0x18 */ {{'K', 0x3bd4}, {'I', 0x3728}, {std::nullopt, 0x375c}},
    {},
    {},
    {},
    {},
    {},
    {},
    {},
    {},
    {},
    {},
    {},
    {},
    {},
    {},
    /* 0x27 */ {{'B', 0x3af4}, {'N', 0x3c10}, {std::nullopt, 0x36fc}},
    /* 0x28 */ {{'F', 0x33ec}, {'A', 0x3a88}, {std::nullopt, 0x363c}},
    {},
    {},
    {},
    {},
    {},
    {},
    {},
    {},
    {},
    {},
    {},
    {},
    {},
    {},
    {},
    {},
    {},
    {},
    {},
    {},
    {},
    {},
    {},
    {},
    {},
    {},
    {},
    {},
    {},
    /* 0x46 */ {{'E', 0x333c}, {'R', 0x3be0}, {std::nullopt, 0x360c}},
    {},
    {},
    {},
    {},
    {},
    {},
    {},
    {},
    {},
    {},
    {},
    {},
    {},
    {},
    {},
    {},
    {},
    {},
    {},
    {},
    {},
    {},
    {},
    {},
    {},
    {},
    /* 0x61 */ {{'E', 0x374c}, {'A', 0x3c40}, {std::nullopt, 0x372c}},
    {},
    {},
    {},
    {},
    /* 0x66 */ {{'E', 0x33fc}, {'L', 0x3758}, {std::nullopt, 0x378c}},
    {},
    {},
    {},
    /* 0x6A */ {{'K', 0x3c34}, {'I', 0x34a8}, {std::nullopt, 0x37bc}},
    {},
    {},
    {},
    {},
    {},
    {},
    {},
    {},
    {},
    {},
    {},
    {},
    {},
    {},
    {},
    {},
    {},
    {},
    {},
    {},
    {},
    {},
    {},
    {},
    {},
    {},
    {},
    {},
    {},
    {},
    {},
    {},
    {},
    {},
    {},
    {},
    {},
    {},
    {},
    {},
    {},
    {},
    {},
    {},
    {},
    {},
    {},
    {},
    {},
    {},
    {},
    {},
    {},
    {},
    {},
    {},
    {},
    {},
    /* 0xA5 */ {{'A', 0x3a5c}, {'R', 0x34f8}, {std::nullopt, 0x366c}},
    {},
    {},
    {},
    {},
    {},
    {},
    {},
    {},
    {},
    {},
    {},
    {},
    {},
    /* 0xB3 */ {{'B', 0x39d4}, {'U', 0x3228}, {'R', 0x3cfc}, {std::nullopt, 0x3638}},
    {},
    {},
    {},
    {},
    {},
    {},
    {},
    {},
    {},
    {},
    {},
    /* 0xBF */ {{' ', 0x357c}, {'C', 0x3b80}, {std::nullopt, 0x37ec}},
    {},
    {},
    {},
    {},
    {},
    {},
    /* 0xC6 */ {{'A', 0x3994}, {' ', 0x3668}, {std::nullopt, 0x369c}},
    {},
    {},
    {},
    {},
    {},
    {},
    {},
    {},
    {},
    {},
    {},
    {},
    {{'R', 0x372c}, {'I', 0x3cc0}, {std::nullopt, 0x36cc}}, /* 0xD3 */
    {},
    {},
    {},
    {},
    {},
    {},
    {},
    {},
    {},
    {},
    {},
    {},
    {},
    {},
    {},
    {},
    {},
    {},
    {},
    {},
    {},
    {},
    {},
    {},
    {},
    {},
    {},
    {},
    {},
    {},
    {},
    {},
    {{'-', 0x3ec4}, {' ', 0x3518}, {std::nullopt, 0x384c}}, /* 0xF4 */
    {},
    {},
    {},
    {},
    {},
    {},
    {},
    {},
    {},
    {},
    {}};

// Indexed as:
// [cgb_flag & 0x80]
// [0: old_Licensee == 0x01, 1: old_Licensee == 0x33, 2: else]
// [new_licensee[0] == 0x30]
// [new_licensee[1] == 0x31]
constexpr uint16_t DMG_MODE_DIV16_BY_HEADER[2][3][2][2] = {
    {
        {{0x0000, 0x0000}, {0x0000, 0x0000}},
        {{0x2678, 0x2678}, {0x269c, 0x0020}},
        {{0x267c, 0x267c}, {0x267c, 0x267c}},
    },
    {
        {{0x2fa8, 0x2fa8}, {0x2fa8, 0x2fa8}},
        {{0x1e9c, 0x1e9c}, {0x1ec0, 0x2fc8}},
        {{0x1ea0, 0x1ea0}, {0x1ea0, 0x1ea0}},
    },

};

uint16_t compute_initial_cgb_div16(const CartridgeHeader& header) {
    // TODO: does this logic cover all the possible cases, or there's something else of the header to consider?

    static constexpr uint16_t SENTINEL = 0x100;

    // If the rom does not need to compute the title checksum (i.e. either is a CGB game or is a DMG not licensed by
    // nintendo) then the DIV16 is "trivial" to predict and is given by the DMG_MODE_DIV16_BY_HEADER table.
    uint16_t base_div16 =
        DMG_MODE_DIV16_BY_HEADER[CartridgeHeaderHelpers::is_cgb_game(
            header)][header.old_licensee_code == Specs::Cartridge::Header::OldLicensee::NINTENDO
                         ? 0
                         : (header.old_licensee_code == Specs::Cartridge::Header::OldLicensee::USE_NEW_LICENSEE ? 1
                                                                                                                : 2)]
                                [header.new_licensee_code[0] == Specs::Cartridge::Header::NewLicensee::NINTENDO[0]]
                                [header.new_licensee_code[1] == Specs::Cartridge::Header::NewLicensee::NINTENDO[1]];

    if (base_div16 > SENTINEL) {
        // Checksum does not affect the rom's boot time: our prediction is good to go.
        return base_div16;
    }

    // Prediction of DIV16 is more complex: we need to use the title checksum.
    const uint8_t title_checksum = CartridgeHeaderHelpers::title_checksum(header);
    uint16_t checksum_div16 = DMG_MODE_CHECKSUM_DIV16[title_checksum];

    if (checksum_div16 > SENTINEL) {
        // The title checksum is not an ambiguous checksum: the prediction is good to go.
        return base_div16 + checksum_div16;
    }

    // The fourth letter of the title affects the timing.
    const uint8_t fourth_letter = header.title[3];
    const auto& ambiguous_checksum_prediction_table = DMG_MODE_AMBIGUOUS_CHECKSUM_DIV16[title_checksum];
    ASSERT(ambiguous_checksum_prediction_table[0].first && ambiguous_checksum_prediction_table[1].first);

    for (const auto& [letter, div16] : ambiguous_checksum_prediction_table) {
        if (!letter /* default index case */ || *letter == fourth_letter) {
            checksum_div16 = div16;
            break;
        }
    }

    ASSERT(checksum_div16 > SENTINEL);
    return base_div16 + checksum_div16;
}

#endif
} // namespace
/* TIMA overflow timing example.
 *   t                  CPU                              Timer
 * CPU   |
 * Timer |                                      TIMA overflows -> Pending
 *
 * CPU   | [writing TIMA aborts PendingReload]
 *       | [writing TMA is considered]
 * Timer |                                      Pending -> Reload (TIMA = TMA, raise interrupt)
 *
 * CPU   | [writing TIMA is ignored]
 *       | [writing TMA writes TIMA too]
 * DMA   |                                      Reload -> None
 */
#if defined(ENABLE_CGB) && !defined(ENABLE_BOOTROM)
Timers::Timers(Interrupts& interrupts, CartridgeHeader& header) :
#else
Timers::Timers(Interrupts& interrupts) :
#endif
    interrupts {interrupts}
#if defined(ENABLE_CGB) && !defined(ENABLE_BOOTROM)
    ,
    header {header}
#endif
{
}

void Timers::tick() {
    handle_pending_tima_reload();
    set_div(div16 + 4);
}

void Timers::save_state(Parcel& parcel) const {
    PARCEL_WRITE_UINT16(parcel, div16);
    PARCEL_WRITE_UINT8(parcel, tima);
    PARCEL_WRITE_UINT8(parcel, tma);
    PARCEL_WRITE_BOOL(parcel, tac.enable);
    PARCEL_WRITE_UINT8(parcel, tac.clock_selector);
    PARCEL_WRITE_UINT8(parcel, tima_state);
    PARCEL_WRITE_BOOL(parcel, last_div_bit_and_tac_enable);
}

void Timers::load_state(Parcel& parcel) {
    div16 = parcel.read_uint16();
    tima = parcel.read_uint8();
    tma = parcel.read_uint8();
    tac.enable = parcel.read_bool();
    tac.clock_selector = parcel.read_uint8();
    tima_state = parcel.read_uint8();
    last_div_bit_and_tac_enable = parcel.read_bool();
}

void Timers::reset() {
#ifdef ENABLE_CGB
#ifdef ENABLE_BOOTROM
    div16 = 0x0008;
#else
    // On CGB accurate prediction of DIV is fairly complex, that's because the boot rom takes different paths
    // based on the cartridge header (old licensee code, new licensee code, cgb flag, title checksum, ...).
    div16 = compute_initial_cgb_div16(header);
#endif
#else
    div16 = if_bootrom_else(0x0008, 0xABCC); // [mooneye/boot_div-dmgABCmgb.gb]
#endif
    tima = 0;
    tma = 0;
    tac.enable = false;
    tac.clock_selector = 0;

    tima_state = TimaReloadState::None;
    last_div_bit_and_tac_enable = false;
}

uint8_t Timers::read_div() const {
    const uint8_t div_high = get_byte<1>(div16);
#ifdef ENABLE_DEBUGGER
    DebuggerMemoryWatcher::notify_read(Specs::Registers::Timers::DIV);
#endif
    return div_high;
}

void Timers::write_div(uint8_t value) {
#ifdef ENABLE_DEBUGGER
    DebuggerMemoryWatcher::notify_write(Specs::Registers::Timers::DIV);
#endif

    set_div(0);
}

void Timers::write_tima(uint8_t value) {
    // Writing to TIMA in the same cycle it has been reloaded is ignored
    if (tima_state != TimaReloadState::Reload) {
        tima = value;
        tima_state = TimaReloadState::None;
    }
}

void Timers::write_tma(uint8_t value) {
    // Writing TMA in the same cycle TIMA has been reloaded writes TIMA too
    tma = value;
    if (tima_state == TimaReloadState::Reload) {
        tima = value;
    }
}

uint8_t Timers::read_tac() const {
    return 0b11111000 | tac.enable << Specs::Bits::Timers::TAC::ENABLE |
           tac.clock_selector << Specs::Bits::Timers::TAC::CLOCK_SELECTOR;
}

void Timers::write_tac(uint8_t value) {
    tac.enable = test_bit<Specs::Bits::Timers::TAC::ENABLE>(value);
    tac.clock_selector = keep_bits_range<Specs::Bits::Timers::TAC::CLOCK_SELECTOR>(value);
    on_falling_edge_inc_tima();
}

void Timers::set_div(uint16_t value) {
    div16 = value;
    on_falling_edge_inc_tima();
}

inline void Timers::inc_tima() {
    // When TIMA overflows, TMA is reloaded in TIMA: but it is delayed by 1 m-cycle
    if (++tima == 0) {
        tima_state = TimaReloadState::Pending;
    }
}

inline void Timers::on_falling_edge_inc_tima() {
    // TIMA is incremented if (DIV bit selected by TAC && TAC enable)
    // was true and now it's false (on falling edge)
    const bool tac_div_bit = test_bit(div16, Specs::Timers::TAC_DIV_BITS_SELECTOR[tac.clock_selector]);
    const bool div_bit_and_tac_enable = tac_div_bit && tac.enable;
    if (last_div_bit_and_tac_enable > div_bit_and_tac_enable) {
        inc_tima();
    }
    last_div_bit_and_tac_enable = div_bit_and_tac_enable;
}

inline void Timers::handle_pending_tima_reload() {
    if (tima_state != TimaReloadState::None) {
        if (--tima_state == TimaReloadState::Reload) {
            tima = (uint8_t)tma;
            interrupts.raise_interrupt<Interrupts::InterruptType::Timer>();
        }
    }
}