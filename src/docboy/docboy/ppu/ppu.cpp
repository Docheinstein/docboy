#include "docboy/ppu/ppu.h"

#include "docboy/bootrom/helpers.h"
#include "docboy/common/randomdata.h"
#include "docboy/dma/dma.h"
#include "docboy/interrupts/interrupts.h"
#include "docboy/lcd/lcd.h"
#include "docboy/memory/memory.h"
#include "docboy/ppu/pixelmap.h"

#ifdef ENABLE_CGB
#include "docboy/hdma/hdma.h"
#include "docboy/speedswitch/speedswitchcontroller.h"
#endif

using namespace Specs::Bits::Video::LCDC;
using namespace Specs::Bits::Video::STAT;

using namespace Specs::Ppu::Modes;

namespace {
constexpr uint8_t TILE_WIDTH = 8;
constexpr uint8_t TILE_HEIGHT = 8;
constexpr uint8_t TILE_DOUBLE_HEIGHT = 16;
constexpr uint8_t TILE_BYTES = 16;
constexpr uint8_t TILE_ROW_BYTES = 2;

constexpr uint8_t TILEMAP_WIDTH = 32;
constexpr uint8_t TILEMAP_HEIGHT = 32;
constexpr uint8_t TILEMAP_CELL_BYTES = 1;

constexpr uint8_t OAM_ENTRY_BYTES = 4;

constexpr uint8_t OBJ_COLOR_INDEX_TRANSPARENT = 0;

#ifdef ENABLE_CGB
constexpr uint16_t NUMBER_OF_COLORS = 32768;
#else
constexpr uint8_t NUMBER_OF_COLORS = 4;
#endif

constexpr uint8_t BITS_PER_PIXEL = 2;
constexpr uint8_t NUMBER_OF_COLOR_INDEXES = 4;

constexpr std::array<uint8_t, 153> generate_ly_update_table() {
    constexpr uint8_t LAST_3_BITS[] {
        /* 0x00 */ 0x00,
        /* 0x01 */ 0x00,
        /* 0x02 */ 0x02,
        /* 0x03 */ 0x00,
        /* 0x04 */ 0x04,
        /* 0x05 */ 0x04,
        /* 0x06 */ 0x06,
        /* 0x07 */ 0x00,
    };

    std::array<uint8_t, 153> table {};

    for (uint8_t ly = 0; ly < 153; ly++) {
        const uint8_t ly_last_3_bits = keep_bits<3>(ly);

        uint8_t ly_out {};

        if (keep_bits<4>(ly) == 0x0F) {
            if (ly == 0x8F) {
                ly_out = 0x80;
            } else {
                ly_out = LAST_3_BITS[get_bits_range<7, 4>(ly)] << 4;
            }
        } else {
            ly_out = discard_bits<3>(ly) | LAST_3_BITS[ly_last_3_bits];
        }

        table[ly] = ly_out;
    }

    return table;
}

inline bool is_obj_opaque(const uint8_t color_index) {
    return color_index != OBJ_COLOR_INDEX_TRANSPARENT;
}
} // namespace

const Ppu::TickSelector Ppu::TICK_SELECTORS[] = {
    &Ppu::oam_scan_even,
    &Ppu::oam_scan_odd,
    &Ppu::oam_scan_77,
    &Ppu::oam_scan_done,
    &Ppu::oam_scan_after_turn_on,
    &Ppu::pixel_transfer_dummy_lx0,
    &Ppu::pixel_transfer_discard_lx0,
    &Ppu::pixel_transfer_discard_lx0_wx0_scx7,
    &Ppu::pixel_transfer_lx0,
    &Ppu::pixel_transfer_lx8,
    &Ppu::hblank,
    &Ppu::hblank_453,
    &Ppu::hblank_454,
    &Ppu::hblank_455,
    &Ppu::hblank_last_line,
    &Ppu::hblank_last_line_453,
    &Ppu::hblank_last_line_454,
    &Ppu::hblank_last_line_455,
    &Ppu::hblank_first_line_after_turn_on,
    &Ppu::vblank,
    &Ppu::vblank_453,
    &Ppu::vblank_454,
    &Ppu::vblank_455,
    &Ppu::vblank_last_line,
    &Ppu::vblank_last_line_2,
    &Ppu::vblank_last_line_3,
    &Ppu::vblank_last_line_7,
    &Ppu::vblank_last_line_453,
    &Ppu::vblank_last_line_454,
    &Ppu::vblank_last_line_455,
};

const Ppu::FetcherTickSelector Ppu::FETCHER_TICK_SELECTORS[] = {
    &Ppu::bgwin_prefetcher_get_tile_0,
    &Ppu::bg_prefetcher_get_tile_0,
    &Ppu::bg_prefetcher_get_tile_1,
    &Ppu::bg_pixel_slice_fetcher_get_tile_data_low_0,
    &Ppu::bg_pixel_slice_fetcher_get_tile_data_low_1,
    &Ppu::bg_pixel_slice_fetcher_get_tile_data_high_0,
    &Ppu::win_prefetcher_activating,
    &Ppu::win_prefetcher_get_tile_0,
    &Ppu::win_prefetcher_get_tile_1,
    &Ppu::win_pixel_slice_fetcher_get_tile_data_low_0,
    &Ppu::win_pixel_slice_fetcher_get_tile_data_low_1,
    &Ppu::win_pixel_slice_fetcher_get_tile_data_high_0,
    &Ppu::bgwin_pixel_slice_fetcher_get_tile_data_high_1,
    &Ppu::bgwin_pixel_slice_fetcher_push,
    &Ppu::obj_prefetcher_get_tile_0,
    &Ppu::obj_prefetcher_get_tile_1,
    &Ppu::obj_pixel_slice_fetcher_get_tile_data_low_0,
    &Ppu::obj_pixel_slice_fetcher_get_tile_data_low_1,
    &Ppu::obj_pixel_slice_fetcher_get_tile_data_high_0,
    &Ppu::obj_pixel_slice_fetcher_get_tile_data_high_1_and_merge_with_obj_fifo};

#ifdef ENABLE_CGB
Ppu::Ppu(Lcd& lcd, Interrupts& interrupts, Hdma& hdma, VramBus::View<Device::Ppu> vram_bus,
         OamBus::View<Device::Ppu> oam_bus, Dma& dma_controller, SpeedSwitchController& speed_switch_controller) :
#else
Ppu::Ppu(Lcd& lcd, Interrupts& interrupts, VramBus::View<Device::Ppu> vram_bus, OamBus::View<Device::Ppu> oam_bus,
         Dma& dma_controller) :
#endif
    lcd {lcd},
    interrupts {interrupts},
#ifdef ENABLE_CGB
    hdma {hdma},
#endif
    vram {vram_bus},
    oam {oam_bus},
    dma_controller {dma_controller}
#ifdef ENABLE_CGB
    ,
    speed_switch_controller {speed_switch_controller}
#endif
{
}

void Ppu::tick() {
    // Do nothing if PPU is off
    if (lcdc.enable) {
        // Tick PPU by one dot
        (this->*(tick_selector))();

        // Check for window activation (the state holds for entire frame).
        // The activation conditions are checked every dot,
        // (not only during pixel transfer or at LY == WX).
        // This is also reflected in the fact that window glitches for
        // the rest of the frame by pushing 00 pixels if WX == LX during the fetched
        // push stage if at some point in the entire frame the condition
        // WY = LY was satisfied.
        tick_window();

        // Update STAT's LYC_EQ_LY and eventually raise STAT interrupt.
        raise_stat_irq();

        // There is 1 T-cycle delay between BGP and the BGP value the PPU sees.
        // During this T-Cycle, the PPU sees the last BGP ORed with the new one.
        // Therefore, we store the last BGP value here so that we can use (BGP | LAST_BGP)
        // for resolving BG color.
        // [mealybug/m3_bgp_change]
        last_bgp = bgp;

        // It seems that there is 1 T-cycle delay between WX and the WX value the PPU sees.
        // Therefore, we store the last WX here and we use it for the next T-cycle.
        // (TODO: Still not sure about this)
        // [mealybug/m3_wx_5_change]
        last_wx = wx;

        // This is needed for:
        // 1) LCDC.WIN_ENABLE, because it seems that the t-cycle window is turned on
        //    (and it was off), window is activated also for LX == WX + 1, not only for LX == WX.
        //    [mealybug/m3_lcdc_win_en_change_multiple_wx]
        // 2) LCDC.BG_WIN_ENABLE, because it seems that there is a 1 T-cycle delay for this bit
        //    [mealybug/m3_lcdc_bg_en_change]
        // 3) LCDC.OBJ_ENABLE, because it seems that there is a 1 T-cycle delay for this bit
        //        [mealybug/m3_lcdc_obj_en_change]
        last_lcdc = lcdc;

        // It seems that there is a 1 T-cycle delay for the LY value used for the comparison LYC == LY.
        // This is supported by the fact that for LY = 153, LYC_EQ_LY STAT flag is raised for LYC = 153
        // the very same T-cycle LY is 0 (but was 153 the T-cycle before).
        last_ly = ly;

        // It seems that there is a 1 T-cycle delay for the LYC value used for the comparison LYC == LY.
        last_lyc = lyc;

#ifdef ENABLE_DEBUGGER
        ++cycles;
#endif
    }

    // Update STAT's mode (usually it matches the "internal" PPU mode, but there are few exceptions).
    tick_stat();

    ASSERT(dots < 456);
}

#ifdef ENABLE_CGB
void Ppu::enable_color_resolver() {
    color_resolver_enabled = true;
}

void Ppu::disable_color_resolver() {
    ASSERT(color_resolver_enabled);
    color_resolver_enabled = false;
}
#endif

// ------- PPU helpers -------

void Ppu::turn_on() {
    ASSERT(ly == 0);

    last_lyc = lyc;

    // STAT's LYC_EQ_LY is updated properly (but interrupt is not raised)
    stat.lyc_eq_ly = is_lyc_eq_ly();

#ifdef ENABLE_CGB
    if (speed_switch_controller.is_double_speed_mode()) {
        // It seems that PPU is earlier by one dot in double speed mode.
        // TODO: further investigations.
        dots = 1;
    }
#endif
}

void Ppu::turn_off() {
    dots = 0;
    ly = 0;
    last_ly = 0;

    lcd.rewind();
    lcd.clear();

    // Clear oam entries eventually still there
    for (uint32_t i = 0; i < array_size(oam_entries); i++) {
        oam_entries[i].clear();
    }

    reset_fetcher();

#ifdef ENABLE_ASSERTS
    oam_entries_count = 0;
    oam_entries_not_served_count = 0;
#endif

#ifdef ENABLE_DEBUGGER
    scanline_oam_entries.clear();
#endif

    // The first line after PPU is turn on behaves differently.
    // STAT's mode remains in HBLANK during OAM Scan, OAM Bus is not acquired,
    // OAM Scan does not check for any sprite and STAT's update for HBLANK is late by 2 T-cycles.
    update_mode<OAM_SCAN>();

    tick_selector = &Ppu::oam_scan_after_turn_on;

    is_glitched_line_0 = true;
#ifndef ENABLE_CGB
    glitched_line_0_hblank_delay = false;
#endif

    enable_lyc_eq_ly_irq = true;

    vram.release();
    oam.release();
}

inline bool Ppu::is_lyc_eq_ly() const {
    // In addition to the condition (LYC == LY), LYC_EQ_LY IRQ might be disabled for several reasons.
    // Remarkably:
    // * LYC_EQ_LY is always 0 at dot 454
    // * The last scanline (LY = 153 = 0) the LY values differs from what LYC_EQ_LY reads and from the related IRQ.
    // [mooneye/lcdon_timing, daid/ppu_scanline_bgp]

    return (last_lyc == last_ly && enable_lyc_eq_ly_irq);
}

inline void Ppu::raise_stat_irq() {
    // STAT interrupt request is checked every dot (not only during modes or lines transitions).
    // OAM Stat interrupt seems to be an exception to this rule:
    // it is raised only as a consequence of a transition of mode, not always
    // (e.g. manually writing STAT's OAM interrupt flag while in OAM does not raise a request).
    // Note that the interrupt request is done only on raising edge
    // [mooneye/stat_irq_blocking, mooneye/stat_lyc_onoff].
    const bool lyc_eq_ly = is_lyc_eq_ly();

    if (pending_stat_irq) {
        // Actually raise STAT interrupt.
        interrupts.raise_interrupt<Interrupts::InterruptType::Stat>();
        pending_stat_irq = false;
    } else {
        const bool lyc_eq_ly_irq = stat.lyc_eq_ly_int && lyc_eq_ly;

        const bool hblank_irq = stat.hblank_int && mode == HBLANK;

        // VBlank interrupt is raised either with VBLANK or OAM STAT's flag when entering VBLANK.
        const bool vblank_irq = (stat.vblank_int || stat.oam_int) && mode == VBLANK;

        // Eventually schedule raise of STAT interrupt.
        update_stat_irq(lyc_eq_ly_irq || hblank_irq || vblank_irq);
    }

    // Update STAT's LYC=LY Flag according to the current comparison
    stat.lyc_eq_ly = lyc_eq_ly;
}

inline void Ppu::tick_stat() {
#ifndef ENABLE_CGB
    // On DMG, STAT write takes 1 T-Cycle to have effect.
    if (stat_write.pending) {
        stat_write.pending = false;
        write_stat_real(stat_write.value);
    }
#endif

    // Usually, reading from STAT yields the correct "internal" PPU mode.
    // There are a few exceptions, though:
    // 1) For the first line after PPU is turned off, STAT's mode appears as HBlank during OAM Scan.
    // 2) During DMA transfer, STAT's mode appears as HBlank during OAM Scan.
    // 3) On DMG, during the transition between VBlank and OAM Scan, STAT's mode appears as HBlank.
    // Note that the "internal" PPU mode remains the same (e.g. STAT interrupt does not take into account this quirk).
    const bool force_hblank_mode = mode == OAM_SCAN && (is_glitched_line_0 || dma_controller.is_active()
#ifndef ENABLE_CGB
                                                        || vblank_last_line_stat_mode_hblank_glitch
#endif
                                                       );

    stat.mode = force_hblank_mode ? HBLANK : mode;

    // The difference between stat_mode and stat.mode is subtle:
    // * stat.mode contains the "unglitched" STAT mode (as seen by HDMA, for example).
    // * stat_mode contains the mode that is read by reading STAT (FF41).
    // These two are always the same, except on CGB double speed mode,
    // where HBlank seems to take 1 extra T-Cycle to be visible through STAT's read.

#ifdef ENABLE_CGB
    if (!delay_stat_mode_update) {
        stat_mode = stat.mode;
    } else {
        delay_stat_mode_update = false;
    }
#endif
}

inline void Ppu::update_stat_irq(bool irq) {
    // Raise STAT interrupt request only on rising edge.
    // It seems that STAT interrupt is triggered with a delay of 1 T-Cycle.
    pending_stat_irq = last_stat_irq < irq;
    last_stat_irq = irq;
}

inline void Ppu::update_stat_irq_for_oam_mode() {
    ASSERT(enable_lyc_eq_ly_irq);
    ASSERT(last_ly == ly);
    ASSERT(last_lyc == lyc);

    // OAM mode interrupt is not checked every dot: it is checked only here during mode transition.
    // Furthermore, it seems that having a pending LYC_EQ_LY signal high prevents the STAT IRQ to go low.
    // [daid/ppu_scanline_bgp]
    bool lyc_eq_ly_irq = stat.lyc_eq_ly_int && (last_lyc == last_ly);
    bool lyc_eq_next_ly_irq = stat.lyc_eq_ly_int && (last_lyc == last_ly + 1);
    bool irq = stat.oam_int || lyc_eq_ly_irq;

    pending_stat_irq = last_stat_irq < irq;

    // Actually, from the tests it seems that the only case in which the STAT IRQ is reset
    // (and consequently there's an opportunity to trigger an interrupt on raising edge)
    // is when LY_EQ_LYC is true for the next LY.
    // TODO: This is bizarre by the way, find a more reasonable way to represent this oddity.
    if (lyc_eq_next_ly_irq) {
        last_stat_irq = irq;
    }
}

inline void Ppu::update_stat_irq_for_oam_mode_do_not_clear_last_stat_irq() {
    ASSERT(enable_lyc_eq_ly_irq);
    ASSERT(last_ly == ly);
    ASSERT(last_lyc == lyc);

    bool lyc_eq_ly_irq = stat.lyc_eq_ly_int && (last_lyc == last_ly);
    bool irq = stat.oam_int || lyc_eq_ly_irq;
    pending_stat_irq = last_stat_irq < irq;
}

inline void Ppu::begin_increase_ly() {
    ASSERT(ly < 153);
#ifdef ENABLE_CGB
    // LY is affected by a bug: there's a window of 1 T-Cycle during which it might take
    // a different value (which follows a periodic and very specific pattern).
    // Contrarily to some documentation, this happens both in single speed mode and in
    // double speed mode (depending on the T-Cycle CPU/PPU are aligned each other).
    // It never happens in DMG.
    static constexpr std::array<uint8_t, 153> LY_UPDATE_TABLE = generate_ly_update_table();
    next_ly = ly + 1;
    ly = LY_UPDATE_TABLE[ly];
#endif
}

inline void Ppu::end_increase_ly() {
#ifdef ENABLE_CGB
    ly = next_ly;
#else
    ++ly;
#endif
}

inline void Ppu::write_stat_real(uint8_t value) {
    stat.lyc_eq_ly_int = test_bit<Specs::Bits::Video::STAT::LYC_EQ_LY_INTERRUPT>(value);
    stat.oam_int = test_bit<Specs::Bits::Video::STAT::OAM_INTERRUPT>(value);
    stat.vblank_int = test_bit<Specs::Bits::Video::STAT::VBLANK_INTERRUPT>(value);
    stat.hblank_int = test_bit<Specs::Bits::Video::STAT::HBLANK_INTERRUPT>(value);
}

inline void Ppu::tick_window() {
    // Window is considered active for the rest of the frame
    // if at some point in it happens that window is enabled while LY matches WY.
    // Note that this does not mean that window will always be rendered:
    // the WX == LX condition will be checked again during pixel transfer
    // (on the other hand WY is not checked again).
    w.active_for_frame |= lcdc.win_enable && ly == wy;
}

// ------- PPU states ------

void Ppu::oam_scan_even() {
    ASSERT(dots % 2 == 0);
    ASSERT(dots <= 76);
    ASSERT(oam.is_acquired_by_this() || dots == 76 /* oam bus seems released this cycle */);

    // Flush the read request for the next OAM entry.
    oam_scan_flush_read_request();

    if (++dots == 77) {
        tick_selector = &Ppu::oam_scan_77;
    } else {
        tick_selector = &Ppu::oam_scan_odd;
    }
}

void Ppu::oam_scan_odd() {
    ASSERT(dots % 2 == 1);
    ASSERT(dots <= 76);
    ASSERT(oam.is_acquired_by_this());

    // Submit a read request to read two bytes from OAM (Y and X) for the next OAM entry.
    oam_scan_read_request();

    if (dots == 75) {
        // OAM bus seems to be released (i.e. writes to OAM works normally) just for this cycle
        // [mooneye/lcdon_write_timing]
        oam.release();
    }

    ++dots;

    tick_selector = &Ppu::oam_scan_even;
}

void Ppu::oam_scan_77() {
    ASSERT(dots == 77);

    // OAM bus is re-acquired this cycle
    // [mooneye/lcdon_write_timing]
    oam.acquire();

    // VRAM bus is acquired one cycle before STAT is updated
    // [mooneye/lcdon_timing]
    vram.acquire();

    dots = 78;

    tick_selector = &Ppu::oam_scan_done;
}

void Ppu::oam_scan_done() {
    ASSERT(dots >= 78);

    if (++dots == 79) {
        // STAT Mode is updated to Pixel Transfer one dot earlier it actually comes in.
        update_mode<PIXEL_TRANSFER>();
    } else {
        ASSERT(dots == 80);
        enter_pixel_transfer();
    }
}

void Ppu::oam_scan_after_turn_on() {
    ASSERT(!oam.is_acquired_by_this());

    // First OAM Scan after PPU turn on does nothing.
    if (++dots == 79) {
        // Mode is updated to Pixel Transfer one dot earlier it actually comes in.
        ASSERT(mode == OAM_SCAN);
        update_mode<PIXEL_TRANSFER>();
    } else if (dots == 80) {
        enter_pixel_transfer();
    }
}

void Ppu::pixel_transfer_dummy_lx0() {
    ASSERT(lx == 0);
    ASSERT(!w.active);
    ASSERT(oam.is_acquired_by_this());
    ASSERT(vram.is_acquired_by_this());

    if (++dots == 83) {
        // The first tile fetch is used only for make the initial SCX % 8
        // alignment possible, but this data will be thrown away in any case.
        // So filling this with junk is not a problem (even for window with WX=0).
        bg_fifo.fill();

        // Initial SCX is not read after the beginning of Pixel Transfer.
        // It is read some T-cycles after: around here, after the first BG fetch.
        // (TODO: verify precise T-Cycle timing).
        // [mealybug/m3_scx_low_3_bits]
        pixel_transfer.initial_scx.to_discard = mod<TILE_WIDTH>(scx);

        if (pixel_transfer.initial_scx.to_discard) {
            pixel_transfer.initial_scx.discarded = 0;

            // When SCX % 8 is > 0, window can be activated immediately before BG.
            check_window_activation();

            // When WX=0 and SCX=7, the pixel transfer timing seems to be
            // expected one with SCX=7, but the initial shifting applied
            // to the window is 6, not 7.
            tick_selector = w.active && pixel_transfer.initial_scx.to_discard == 7
                                ? &Ppu::pixel_transfer_discard_lx0_wx0_scx7
                                : &Ppu::pixel_transfer_discard_lx0;
        } else {
            tick_selector = &Ppu::pixel_transfer_lx0;
        }
    }
}

void Ppu::pixel_transfer_discard_lx0() {
    ASSERT(lx == 0);
    ASSERT(pixel_transfer.initial_scx.to_discard < 8);
    ASSERT(pixel_transfer.initial_scx.discarded < pixel_transfer.initial_scx.to_discard);
    ASSERT(oam.is_acquired_by_this());
    ASSERT(vram.is_acquired_by_this());

    // The first SCX % 8 of a background/window tile are simply discarded.
    // Note that LX is not incremented in this case: this let the OBJ align with the BG.
    if (is_bg_fifo_ready_to_be_popped()) {
        bg_fifo.pop_front();

        if (++pixel_transfer.initial_scx.discarded == pixel_transfer.initial_scx.to_discard) {
            // All the SCX % 8 pixels have been discard
            tick_selector = &Ppu::pixel_transfer_lx0;
        }
    }

    tick_fetcher();

    ++dots;
}

void Ppu::pixel_transfer_discard_lx0_wx0_scx7() {
    ASSERT(pixel_transfer.initial_scx.to_discard == 7);
    ASSERT(w.active);

    pixel_transfer_discard_lx0();

    if (pixel_transfer.initial_scx.discarded == 1) {
        // When WX=0 and SCX=7, the pixel transfer timing seems to be
        // expected one with SCX=7, but the initial shifting applied
        // to the window is 6, not 7, therefore we just push a dummy pixel once
        // to fix this and proceed with the standard pixel_transfer_discard_lx0 state.
        bg_fifo.push_back();
        tick_selector = &Ppu::pixel_transfer_discard_lx0;
    }
}

void Ppu::pixel_transfer_lx0() {
    ASSERT(lx < 8);
    ASSERT(pixel_transfer.initial_scx.to_discard == 0 ||
           pixel_transfer.initial_scx.discarded == pixel_transfer.initial_scx.to_discard);
    ASSERT(oam.is_acquired_by_this());
    ASSERT(vram.is_acquired_by_this());

    bool inc_lx = false;

    // For LX € [0, 8) just pop the pixels but do not push them to LCD
    if (is_bg_fifo_ready_to_be_popped()) {
        bg_fifo.pop_front();

        if (obj_fifo.is_not_empty()) {
            obj_fifo.pop_front();
        }

        inc_lx = true;

        if (lx + 1 == 8) {
            tick_selector = &Ppu::pixel_transfer_lx8;
        }

        check_window_activation();
    }

    tick_fetcher();

    if (inc_lx) {
        increase_lx();
    }

    ++dots;
}

void Ppu::pixel_transfer_lx8() {
    ASSERT(lx >= 8);
    ASSERT(oam.is_acquired_by_this());
    ASSERT(vram.is_acquired_by_this());

    bool inc_lx = false;

    // Push a new pixel to the LCD if the BG fifo contains
    // some pixels and it's not blocked by a sprite fetch
    if (is_bg_fifo_ready_to_be_popped()) {
#ifdef ENABLE_CGB
        static constexpr uint16_t NO_COLOR = UINT16_MAX;
        uint16_t color {NO_COLOR};
#else
        static constexpr uint8_t NO_COLOR = 4;
        uint8_t color {NO_COLOR};
#endif

        // Pop out a pixel from BG fifo
        const BgPixel bg_pixel = bg_fifo.pop_front();

        // It seems that there is a 1 T-cycle delay between the real LCDC
        // and the LCDC the PPU sees, both for LCDC.OBJ_ENABLE and LCDC.BG_WIN_ENABLE.
        // LX == 8 seems to be an expection to this rule.
        // [mealybug/m3_lcdc_bg_en_change, mealybug/m3_lcdc_obj_en_change]
        Lcdc lcdc_ = lx == 8 ? lcdc : last_lcdc;

        if (obj_fifo.is_not_empty()) {
            const ObjPixel obj_pixel = obj_fifo.pop_front();

            // Take OBJ pixel instead of the BG pixel only if all are satisfied:
            // - OBJ are still enabled
            // - OBJ pixel is opaque
            // - either one of these is satisfied:
            //   > the BG color is 0
            //   > [LCDC.BG_WIN_ENABLE is disabled, in CGB mode]
            //   > BG_OVER_OBJ is disabled [and BG priority is disabled, in CGB mod
#ifdef ENABLE_CGB
            const bool show_obj = lcdc_.obj_enable && is_obj_opaque(obj_pixel.color_index) &&
                                  (!lcdc_.bg_win_enable || bg_pixel.color_index == 0 ||
                                   (!test_bit<Specs::Bits::OAM::Attributes::BG_OVER_OBJ>(obj_pixel.attributes) &&
                                    !test_bit<Specs::Bits::Background::Attributes::PRIORITY>(bg_pixel.attributes)));
#else
            const bool show_obj = lcdc_.obj_enable && is_obj_opaque(obj_pixel.color_index) &&
                                  (bg_pixel.color_index == 0 ||
                                   !test_bit<Specs::Bits::OAM::Attributes::BG_OVER_OBJ>(obj_pixel.attributes));
#endif

            if (show_obj) {
#ifdef ENABLE_CGB
                const uint8_t obj_palette_index =
                    get_bits_range<Specs::Bits::OAM::Attributes::CGB_PALETTE>(obj_pixel.attributes);
                ASSERT(obj_palette_index < 8);
                const uint8_t* obj_palette = &obj_palettes[8 * obj_palette_index];
                color = resolve_color(obj_pixel.color_index, obj_palette);
#else
                color = resolve_color(obj_pixel.color_index,
                                      test_bit<Specs::Bits::OAM::Attributes::DMG_PALETTE>(obj_pixel.attributes) ? obp1
                                                                                                                : obp0);
#endif
            }
        }

        if (color == NO_COLOR) {
            // There is 1 T-cycle delay between BGP and the BGP value the PPU sees.
            // During this T-Cycle, the PPU sees the last BGP ORed with the new BGP.
            // [mealybug/m3_bgp_change]
#ifdef ENABLE_CGB
            const uint8_t bg_palette_index =
                get_bits_range<Specs::Bits::Background::Attributes::PALETTE>(bg_pixel.attributes);
            ASSERT(bg_palette_index < 8);
            const uint8_t* bg_palette = &bg_palettes[8 * bg_palette_index];
            color = resolve_color(bg_pixel.color_index, bg_palette);
#else
            const uint8_t bgp_ = (uint8_t)bgp | last_bgp;
            color = lcdc_.bg_win_enable ? resolve_color(bg_pixel.color_index, bgp_) : 0;
#endif
        }

        ASSERT(color < NUMBER_OF_COLORS);

        lcd.push_pixel(color);

        inc_lx = true;

        if (lx + 1 == 168) {
            increase_lx();
            ++dots;
            enter_hblank();
            return;
        }

        check_window_activation();
    }

    tick_fetcher();

    if (inc_lx) {
        increase_lx();
    }

    ++dots;
}

void Ppu::hblank() {
    ASSERT(lx == 168);
    ASSERT(ly < 143);

    if (++dots == 453) {
        // Eventually raise OAM interrupt
        update_stat_irq_for_oam_mode();

        begin_increase_ly();

        tick_selector = &Ppu::hblank_453;
    }
}

void Ppu::hblank_453() {
    ASSERT(dots == 453);

    // OAM bus is acquired here.
    oam.acquire();

    reset_oam_scan_entries();

    // It seems that the first OAM entry is read here, at dot 453, before the beginning of OAM Scan.
    oam_scan_read_request();

    // LYC_EQ_LY IRQ is disabled for dot 453.
    ASSERT(enable_lyc_eq_ly_irq);
    enable_lyc_eq_ly_irq = false;

    end_increase_ly();

    dots = 454;

    tick_selector = &Ppu::hblank_454;
}

void Ppu::hblank_454() {
    ASSERT(dots == 454);

    // Flush the read request for the first OAM entry.
    oam_scan_flush_read_request();

    // Enable LYC_EQ_LY IRQ again.
    ASSERT(!enable_lyc_eq_ly_irq);
    enable_lyc_eq_ly_irq = true;

    dots = 455;

    tick_selector = &Ppu::hblank_455;

    // STAT Mode is updated to OAM one dot earlier it actually comes in.
    update_mode<OAM_SCAN>();
}

void Ppu::hblank_455() {
    ASSERT(dots == 455);

    oam_scan_read_request();

    dots = 0;

    enter_oam_scan();
}

void Ppu::hblank_last_line() {
    ASSERT(lx == 168);
    ASSERT(ly == 143);

    if (++dots == 453) {
        // Eventually raise OAM interrupt

        // It seems that last_stat_irq is not reset by this attempt to raise an interrupt.
        // (e.g. even if LY=LYC, LYC_EQ_LY STAT interrupt does not happen for the next line).
        // TODO: investigate further: seems strange.
        update_stat_irq_for_oam_mode_do_not_clear_last_stat_irq();

        begin_increase_ly();

        tick_selector = &Ppu::hblank_last_line_453;
    }
}

void Ppu::hblank_last_line_453() {
    ASSERT(dots == 453);

    // LYC_EQ_LY IRQ is disabled for dot 453.
    ASSERT(enable_lyc_eq_ly_irq);
    enable_lyc_eq_ly_irq = false;

    end_increase_ly();

    dots = 454;

    tick_selector = &Ppu::hblank_last_line_454;
}

void Ppu::hblank_last_line_454() {
    ASSERT(dots == 454);

    // Enable LYC_EQ_LY IRQ again.
    ASSERT(!enable_lyc_eq_ly_irq);
    enable_lyc_eq_ly_irq = true;

    dots = 455;

#ifndef ENABLE_CGB
    update_mode<VBLANK>();
#endif

    tick_selector = &Ppu::hblank_last_line_455;
}

void Ppu::hblank_last_line_455() {
    ASSERT(dots == 455);

    dots = 0;

    enter_vblank();
}

void Ppu::hblank_first_line_after_turn_on() {
    ASSERT(is_glitched_line_0);
    ASSERT(mode == PIXEL_TRANSFER);
    ASSERT(ly == 0);

    ++dots;

    if (++glitched_line_0_hblank_delay == 2) {
        // STAT is updated to HBlank mode 2 dots later the first line PPU is turned on.
        update_mode<HBLANK>();

        // TODO: is HDMA resume done here?

        tick_selector = &Ppu::hblank;

        // Not necessary for anything else: reset this.
        is_glitched_line_0 = false;
    }
}

void Ppu::vblank() {
    ASSERT(ly >= 144 && ly < 154);

    if (++dots == 453) {
        begin_increase_ly();

        tick_selector = &Ppu::vblank_453;
    }
}

void Ppu::vblank_453() {
    ASSERT(ly >= 144 && ly < 154);
    ASSERT(dots == 453);

    dots = 454;

    end_increase_ly();

    // LYC_EQ_LY IRQ is disabled for dot 454.
    ASSERT(enable_lyc_eq_ly_irq);
    enable_lyc_eq_ly_irq = false;

    tick_selector = &Ppu::vblank_454;
}

void Ppu::vblank_454() {
    ASSERT(ly >= 144 && ly < 154);
    ASSERT(dots == 454);

    dots = 455;

    // Enable LYC_EQ_LY IRQ again.
    ASSERT(!enable_lyc_eq_ly_irq);
    enable_lyc_eq_ly_irq = true;

    tick_selector = &Ppu::vblank_455;
}

void Ppu::vblank_455() {
    ASSERT(ly >= 144 && ly < 154);
    ASSERT(dots == 455);

    dots = 0;

    tick_selector = ly == 153 ? &Ppu::vblank_last_line : &Ppu::vblank;
}

void Ppu::vblank_last_line() {
    ASSERT(ly == 153);

    if (++dots == 2) {
        // LY is reset to 0
        ly = 0;

        tick_selector = &Ppu::vblank_last_line_2;
    }
}

void Ppu::vblank_last_line_2() {
    ASSERT(ly == 0);
    ASSERT(dots == 2);

    ++dots;

    // LYC_EQ_LY IRQ is disabled for dot 2 through 6.
    // TODO: why is LYC_EQ_LY disabled for 4 dots for LY==153,
    //       while just for 1 dot for the other scanlines?
    ASSERT(enable_lyc_eq_ly_irq);
    enable_lyc_eq_ly_irq = false;

    tick_selector = &Ppu::vblank_last_line_3;
}

void Ppu::vblank_last_line_3() {
    ASSERT(ly == 0);
    ASSERT(dots >= 3);

    if (++dots == 7) {
        // Enable LYC_EQ_LY IRQ again.
        ASSERT(!enable_lyc_eq_ly_irq);
        enable_lyc_eq_ly_irq = true;

        tick_selector = &Ppu::vblank_last_line_7;
    }
}

void Ppu::vblank_last_line_7() {
    ASSERT(ly == 0);
    ASSERT(dots >= 7);

    if (++dots == 453) {
        tick_selector = &Ppu::vblank_last_line_453;
    }
}

void Ppu::vblank_last_line_453() {
    ASSERT(ly == 0);
    ASSERT(dots == 453);

    // OAM bus is acquired here.
    oam.acquire();

    reset_oam_scan_entries();

    // It seems that the first OAM entry is read here, at dot 453, before the beginning of OAM Scan.
    oam_scan_read_request();

#ifndef ENABLE_CGB
    // On DMG, reading STAT the last few cycles of VBlank yields mode HBlank.
    // Nevertheless, this mode change does not cause a STAT interrupt for HBlank mode.
    vblank_last_line_stat_mode_hblank_glitch = true;

    // I guess "internal" mode is changed to OAM Scan
    update_mode<OAM_SCAN>();
#endif

    dots = 454;

    tick_selector = &Ppu::vblank_last_line_454;
}

void Ppu::vblank_last_line_454() {
    ASSERT(ly == 0);
    ASSERT(dots == 454);

    // Flush the read request for the first OAM entry.
    oam_scan_flush_read_request();

    dots = 455;

    tick_selector = &Ppu::vblank_last_line_455;
}

void Ppu::vblank_last_line_455() {
    ASSERT(ly == 0);
    ASSERT(dots == 455);

    oam_scan_read_request();

    dots = 0;

    enter_new_frame();
}

// ------- PPU states helpers ---------

void Ppu::enter_oam_scan() {
    ASSERT(mode == OAM_SCAN);

    tick_selector = &Ppu::oam_scan_even;

    ASSERT(!vram.is_acquired_by_this());
    ASSERT(oam.is_acquired_by_this());
}

void Ppu::enter_pixel_transfer() {
    ASSERT(mode == PIXEL_TRANSFER);
    ASSERT(dots == 80);
    ASSERT(ly < 144);

    ASSERT_CODE({
        ASSERT(oam_scan.count <= 10);
        ASSERT(oam_entries_count <= oam_scan.count);

        uint8_t count {};
        for (uint32_t i = 0; i < array_size(oam_entries); i++) {
            count += oam_entries[i].size();
        }
        ASSERT(count == oam_entries_count);
    });

#ifdef ENABLE_DEBUGGER
    timings.oam_scan = dots;
#endif

    reset_fetcher();

    tick_selector = &Ppu::pixel_transfer_dummy_lx0;

    vram.acquire();
    oam.acquire();
}

inline void Ppu::enter_hblank() {
    ASSERT(lx == 168);
    ASSERT(ly < 144);

    // clang-format off
    ASSERT_CODE({
        // Pixel Transfer Timing.

        // Factors always affecting timing:
        //    REASON                                 | DOTS
        // 1) first dummy fetch                      | 3
        // 2) push first tile (that is discarded)    | 8
        // 3) push 160 pixels to LCD                 | 8 * 20

        // Factors eventually affecting timing:
        //    REASON                                 | DOTS
        // 4) initial SCX % 8 shifting               | SCX % 8
        // 5) each window triggers reset             | 6 * #(win triggers)
        //    the fetcher and takes 6 dots           |
        // 6) WX = 0 and SCX > 0 takes an extra dot  | 1 if WX=0 and SCX > 0

        // 7) from 6 to 11 for sprite fetch          | 6 to 11

        uint16_t lb = 80; // OAM Scan

        // 1) First dummy fetch
        lb += 3;

        // 2) First tile push
        lb += 8;

        // 3) 20 tiles push
        lb += 20 * 8;

        // 4) Initial SCX % 8
        lb += pixel_transfer.initial_scx.to_discard;

        // 5) Window triggers
        lb += 6 * w.line_triggers.size();

        // 6) WX = 0 and SCX > 0
        bool window_triggered_at_wx0 = false;
        for (uint8_t i = 0; i < w.line_triggers.size(); i++) {
            if (w.line_triggers[i] == 0) {
                window_triggered_at_wx0 = true;
                break;
            }
        }
        lb += window_triggered_at_wx0 && pixel_transfer.initial_scx.to_discard > 0;

        const uint8_t oam_entries_served_count = oam_entries_count - oam_entries_not_served_count;

        // 7) Sprite fetch
        lb += 6 * oam_entries_served_count /* use served count for LB */;

        uint16_t ub = lb;

        // A precise Upper Bound is difficult to predict due to sprite timing,
        // I'll let test roms verify this precisely.
        // 7) Sprite fetch
        ub += (11 - 6 /* already considered in LB */) * oam_entries_count /* use total count for UB */;

        ASSERT(dots >= lb);
        ASSERT(dots <= ub);
    });
    // clang-format on

#ifdef ENABLE_DEBUGGER
    timings.pixel_transfer = dots - timings.oam_scan;
#endif

    if (is_glitched_line_0) {
        // HBlank mode is delayed by 2 dots the first line PPU is turned on.
        // It seems that only STAT is delayed by 2 dots; but bus are released now anyhow.
        tick_selector = &Ppu::hblank_first_line_after_turn_on;

        glitched_line_0_hblank_delay = 0;
    } else {
        update_mode<HBLANK>();

        tick_selector = ly == 143 ? &Ppu::hblank_last_line : &Ppu::hblank;

#ifdef ENABLE_CGB
        // On CGB with double speed mode it seems that HBlank takes
        // 1 extra T-Cycle to be visible through STAT's read.
        // Other STAT modes seems to be visible at the right time.
        if (speed_switch_controller.is_double_speed_mode()) {
            delay_stat_mode_update = true;
        }
#endif
    }

    vram.release();
    oam.release();
}

void Ppu::enter_vblank() {
#ifdef ENABLE_DEBUGGER
    timings.hblank = 456 - timings.pixel_transfer;
#endif

    tick_selector = &Ppu::vblank;

    // It seems that the VBlank mode timing is different between DMG and CGB.
    // In CGB, VBlank mode is set 1 dot later.
#ifdef ENABLE_CGB
    update_mode<VBLANK>();
#else
    ASSERT(mode == VBLANK);
#endif

    interrupts.raise_interrupt<Interrupts::InterruptType::VBlank>();

    ASSERT(!vram.is_acquired_by_this());
    ASSERT(!oam.is_acquired_by_this());
}

void Ppu::enter_new_frame() {
    // Reset window line counter
    // (reset to 255 so that it will go to 0 at the first window trigger).
    w.wly = UINT8_MAX;

    // Reset window activation state
    w.active_for_frame = false;

#ifdef ENABLE_CGB
    update_mode<OAM_SCAN>();
#else
    // End of HBlank mode glitch
    vblank_last_line_stat_mode_hblank_glitch = false;

    // We should already be in OAM Scan mode at this point
    ASSERT(mode == OAM_SCAN);
#endif

    enter_oam_scan();

    // Eventually raise OAM interrupt
    update_stat_irq_for_oam_mode();
}

inline void Ppu::tick_fetcher() {
    (this->*(fetcher_tick_selector))();
}

void Ppu::reset_fetcher() {
    lx = 0;
    bg_fifo.clear();
    obj_fifo.clear();
    is_fetching_sprite = false;
    w.active = false;
    w.just_activated = false;
#if defined(ENABLE_DEBUGGER) || defined(ENABLE_ASSERTS)
    w.line_triggers.clear();
#endif
    bwf.lx = 0;
    bwf.interrupted_fetch.has_data = false;
    wf.tilemap_x = 0;
    fetcher_tick_selector = &Ppu::bgwin_prefetcher_get_tile_0;
}

template <uint8_t Mode>
inline void Ppu::update_mode() {
    ASSERT(Mode <= 0b11);
    mode = Mode;
}

#ifdef ENABLE_CGB
inline uint16_t Ppu::resolve_color(const uint8_t color_index, const uint8_t* palette) {
    if (!color_resolver_enabled) {
        // Push black pixels instead.
        return 0x0000;
    }

    ASSERT(color_index < NUMBER_OF_COLOR_INDEXES);
    const uint8_t* palette_color_word = &palette[2 * color_index];
    return keep_bits<15>(concat(palette_color_word[1], palette_color_word[0]));
}
#else
inline uint8_t Ppu::resolve_color(const uint8_t color_index, const uint8_t palette) {
    ASSERT(color_index < NUMBER_OF_COLOR_INDEXES);
    return keep_bits<BITS_PER_PIXEL>(palette >> (BITS_PER_PIXEL * color_index));
}
#endif

inline void Ppu::increase_lx() {
    ASSERT(oam_entries_count >= oam_entries[lx].size());
#ifdef ENABLE_ASSERTS
    oam_entries_not_served_count += oam_entries[lx].size(); // update with the real served oam entries count.
#endif
    ASSERT(oam_entries_not_served_count <= oam_entries_count);

    // Clear oam entries of current LX (it might contain OBJs not served).
    oam_entries[lx].clear();

    // Increase LX.
    ++lx;
}

inline bool Ppu::is_bg_fifo_ready_to_be_popped() const {
    // The condition for which the bg fifo can be popped are:
    // - the bg fifo is not empty
    // - the fetcher is not fetching a sprite
    // - there are no more oam entries for this LX to be fetched or obj are disabled in LCDC
    return bg_fifo.is_not_empty() && !is_fetching_sprite && (oam_entries[lx].is_empty() || !lcdc.obj_enable);
}

inline bool Ppu::is_obj_ready_to_be_fetched() const {
    // The condition for which a obj fetch can be served are:
    // - there is at least a pending OAM entry for the current LX
    // - obj are enabled in LCDC
    return oam_entries[lx].is_not_empty() && lcdc.obj_enable;
}

inline void Ppu::check_window_activation() {
    // The condition for which the window can be triggered are:
    // - at some point in the frame LY was equal to WY
    // - window should is enabled LCDC
    // - pixel transfer LX matches WX
    //     furthermore, it seems that if LCDC.WIN_ENABLE was off and
    //     is turned on while LX == WX + 1, window is activated
    //     (even if it would be late by 1-cycle to be activated).
    //     [mealybug/m3_lcdc_win_en_change_multiple_wx]
    if (w.active_for_frame && !w.active && lcdc.win_enable &&
        (lx == last_wx /* standard case */ ||
         (lx == last_wx + 1 && !last_lcdc.win_enable /* window just enabled case */))) {
        setup_fetcher_for_window();
    }
}

inline void Ppu::setup_fetcher_for_window() {
    ASSERT(!w.active);

    // Activate window.
    w.active = true;
    w.just_activated = true;

    // Increase the window line counter.
    ++w.wly;

#if defined(ENABLE_DEBUGGER) || defined(ENABLE_ASSERTS)
    w.line_triggers.push_back(last_wx);
#endif

    // Reset the window tile counter.
    wf.tilemap_x = 0;

    // Throw away the pixels in the BG fifo.
    bg_fifo.clear();

    // Setup the fetcher to fetch a window tile.
    fetcher_tick_selector = &Ppu::win_prefetcher_activating;
}

void Ppu::reset_oam_scan_entries() {
    ASSERT_CODE({
        for (uint32_t i = 0; i < array_size(oam_entries); i++) {
            ASSERT(oam_entries[i].is_empty());
        }
    });

#ifdef ENABLE_ASSERTS
    oam_entries_count = 0;
    oam_entries_not_served_count = 0;
#endif

#ifdef ENABLE_DEBUGGER
    scanline_oam_entries.clear();
    timings.hblank = 456 - (timings.oam_scan + timings.pixel_transfer);
#endif

    oam_scan.count = 0;
    oam_scan.index = 0;
}

inline void Ppu::oam_scan_read_request() {
    ASSERT(oam_scan.index < 40);
    oam.read_word_request(Specs::MemoryLayout::OAM::START + OAM_ENTRY_BYTES * oam_scan.index);
}

inline void Ppu::oam_scan_flush_read_request() {
    // Note that PPU cannot access OAM while DMA transfer is in progress; in that case
    // it does not read at all and the oam registers will hold their old values.
    if (!oam.is_acquired_by<Device::Dma>()) {
        registers.oam = oam.flush_read_word_request();
    }

    // Handle the new OAM entry
    handle_oam_scan_entry();
}

inline void Ppu::handle_oam_scan_entry() {
    // Do nothing if 10 OAM entries have already been found
    if (oam_scan.count < 10) {
        // Read OAM entry height
        const uint8_t obj_height = TILE_WIDTH << lcdc.obj_size;

        // Check if the sprite is upon this scanline
        const uint8_t oam_entry_y = registers.oam.a;
        const int32_t obj_y = oam_entry_y - TILE_DOUBLE_HEIGHT;

        if (obj_y <= ly && ly < obj_y + obj_height) {
            const uint8_t oam_entry_x = registers.oam.b;

            // Push OAM entry
            if (oam_entry_x < 168) {
                oam_entries[oam_entry_x].emplace_back(oam_scan.index, oam_entry_y
#if defined(ENABLE_DEBUGGER) || defined(ENABLE_ASSERTS)
                                                      ,
                                                      oam_entry_x
#endif
                );

#ifdef ENABLE_DEBUGGER
                scanline_oam_entries.emplace_back(oam_scan.index, oam_entry_y
#ifdef ENABLE_ASSERTS
                                                  ,
                                                  oam_entry_x
#endif
                );
#endif

#ifdef ENABLE_ASSERTS
                ++oam_entries_count;
#endif
            }

            ++oam_scan.count;
        }
    }

    ++oam_scan.index;
}

// -------- Fetcher states ---------

// ----------- bg / win ------------

void Ppu::bgwin_prefetcher_get_tile_0() {
    ASSERT(!is_fetching_sprite);

    // Check whether fetch the tile for the window or for the BG
    if (w.active) {
        if (lcdc.win_enable /* TODO: other conditions checked? WY?*/) {
            // Proceed to fetch window
            win_prefetcher_get_tile_0();
            return;
        }

        // Abort window fetch and proceed with BG fetch
        w.active = false;
    }

    bg_prefetcher_get_tile_0();
}

void Ppu::bg_prefetcher_get_tile_0() {
    ASSERT(!is_fetching_sprite);

    // The prefetcher computes at this phase only the tile base address.
    // based on the tilemap X and Y coordinate and the tile map to use.
    // [mealybug/m3_lcdc_bg_map_change].
    setup_bg_pixel_slice_fetcher_tilemap_tile_address();

    setup_bg_pixel_slice_fetcher_tile_data_address();

    fetcher_tick_selector = &Ppu::bg_prefetcher_get_tile_1;
}

void Ppu::bg_prefetcher_get_tile_1() {
    ASSERT(!is_fetching_sprite);
    fetcher_tick_selector = &Ppu::bg_pixel_slice_fetcher_get_tile_data_low_0;
}

void Ppu::bg_pixel_slice_fetcher_get_tile_data_low_0() {
    ASSERT(!is_fetching_sprite);

    // Note that fetcher determinate the tile data address to read both in
    // GetTileDataLow and GetTileDataHigh just before access it from VRAM.
    // Therefore, changes to SCY or BG_WIN_TILE_DATA during these phases may
    // have bitplane desync effects
    // (e.g. read the low byte from a tile and the high byte from a different one).
    // [mealybug/m3_scy_change, mealybug/m3_lcdc_tile_sel_change]
    setup_bg_pixel_slice_fetcher_tile_data_address();

#ifdef ENABLE_CGB
    if (test_bit<Specs::Bits::Background::Attributes::BANK>(bwf.attributes)) {
        psf.tile_data_low = vram.read<1>(psf.tile_data_vram_address);
    } else {
        psf.tile_data_low = vram.read<0>(psf.tile_data_vram_address);
    }
#else
    psf.tile_data_low = vram.read<0>(psf.tile_data_vram_address);
#endif

    fetcher_tick_selector = &Ppu::bg_pixel_slice_fetcher_get_tile_data_low_1;
}

void Ppu::bg_pixel_slice_fetcher_get_tile_data_low_1() {
    ASSERT(!is_fetching_sprite);

    fetcher_tick_selector = &Ppu::bg_pixel_slice_fetcher_get_tile_data_high_0;
}

void Ppu::bg_pixel_slice_fetcher_get_tile_data_high_0() {
    ASSERT(!is_fetching_sprite);

    // Note that fetcher determinate the tile data address to read both in
    // GetTileDataLow and GetTileDataHigh just before access it from VRAM.
    // Therefore, changes to SCY or BG_WIN_TILE_DATA during these phases may
    // have bitplane desync effects
    // (e.g. read the low byte from a tile and the high byte from a different one).
    // [mealybug/m3_scy_change, mealybug/m3_lcdc_tile_sel_change]
    setup_bg_pixel_slice_fetcher_tile_data_address();

#ifdef ENABLE_CGB
    if (test_bit<Specs::Bits::Background::Attributes::BANK>(bwf.attributes)) {
        psf.tile_data_high = vram.read<1>(psf.tile_data_vram_address + 1);
    } else {
        psf.tile_data_high = vram.read<0>(psf.tile_data_vram_address + 1);
    }
#else
    psf.tile_data_high = vram.read<0>(psf.tile_data_vram_address + 1);
#endif

    fetcher_tick_selector = &Ppu::bgwin_pixel_slice_fetcher_get_tile_data_high_1;
}

// Some random notes about window //

/*
 *  Pixel Transfer Window Timing.
 *
 *   WX | SCX |   LX=1   |   LX=8      | Actual SCX discard
 *   -------------------------------------------------------
 *          (no window case)
 *  inf |  0  |   X      |  Y = X + 7  | 0
 *
 *       (window WX=0 window case)
 *    0 |  0  |   X      |  Y + 6      | 0
 *    0 |  1  |   X + 8  |  Y + 8      | 1
 *    0 |  2  |   X + 9  |  Y + 9      | 2
 *    0 |  3  |   X + 10 |  Y + 10     | 3
 *    0 |  4  |   X + 11 |  Y + 11     | 4
 *    0 |  5  |   X + 12 |  Y + 12     | 5
 *    0 |  6  |   X + 13 |  Y + 13     | 6
 *    0 |  7  |   X + 14 |  Y + 14     | 6 (not 7)
 *
 *    Probably X = 84, Y = 91.
 */

/*
 *  Window Abort BG Reprise.
 *
 *  BG Phase           |  BG tilemap X
 *  ----------------------------------
 *  GetTile 0          | +0
 *  GetTile 1          | +0
 *  GetTileDataLow 0   | +0
 *  GetTileDataLow 1   | +0
 *  GetTileDataHigh 0  | +0
 *  GetTileDataHigh 1  | +0
 *  Push               | +1
 */

/*
 *  Window 00 Pixel Glitch.
 *
 *  Given WX1, WX2:
 *
 *  Glitch Same Line   = 1 if (WX2 = WX1 + 8n)        ∃n € N
 *  Glitch Next Lines  = 1 if (WX2 = 7 - SCX + 8n)    ∃n € N
 */

inline void Ppu::win_prefetcher_activating() {
    ASSERT(!is_fetching_sprite);
    ASSERT(w.active_for_frame);
    ASSERT(w.active);
    ASSERT(w.just_activated);
    ASSERT(lcdc.win_enable);

    // It seems that the t-cycle the window is activated is just wasted and the
    // fetcher does not advance (this is deduced by the fact that the rom's expected
    // behavior of the t-cycles of this first window fetch matches the normal
    // window states only if such nop t-cycle is inserted after the window activation).
    // Note that the timing added by the window fetch is still 6 dots: therefore
    // is likely that the first window tile is pushed with the GetTile1 fetcher
    // phase and does not waste an additional t-cycle in Push.
    // [mealybug/m3_lcdc_win_map_change, mealybug/m3_lcdc_win_en_change_multiple_wx],
    fetcher_tick_selector = &Ppu::win_prefetcher_get_tile_0;
}

void Ppu::win_prefetcher_get_tile_0() {
    ASSERT(!is_fetching_sprite);
    ASSERT(w.active_for_frame);
    ASSERT(w.active);

    // If the window is turned off, the fetcher switches back to the BG fetcher.
    if (!lcdc.win_enable) {
        bg_prefetcher_get_tile_0();
    } else {
        // The prefetcher computes at this phase only the tile base address.
        // based on the tilemap X and Y coordinate and the tile map to use.
        // [mealybug/m3_lcdc_win_map_change].
        setup_win_pixel_slice_fetcher_tilemap_tile_address();

        setup_win_pixel_slice_fetcher_tile_data_address();

        fetcher_tick_selector = &Ppu::win_prefetcher_get_tile_1;
    }

    if (w.just_activated) {
        // The window activation shifts back the BG prefetcher by one tile.
        // Note that this happens here (not when the window is triggered),
        // therefore there is a 1 t-cycle window opportunity to show
        // the same tile twice if window is disabled just after it is activated.
        bwf.lx -= TILE_WIDTH;
    }
}

void Ppu::win_prefetcher_get_tile_1() {
    ASSERT(!is_fetching_sprite);
    ASSERT(w.active_for_frame);
    ASSERT(w.active);

    // If the window is turned off, the fetcher switches back to the BG fetcher.
    if (!lcdc.win_enable) {
        bg_prefetcher_get_tile_1();
        return;
    }

    fetcher_tick_selector = &Ppu::win_pixel_slice_fetcher_get_tile_data_low_0;
}

void Ppu::win_pixel_slice_fetcher_get_tile_data_low_0() {
    ASSERT(!is_fetching_sprite);

    // If the window is turned off, the fetcher switches back to the BG fetcher.
    if (!lcdc.win_enable) {
        bg_pixel_slice_fetcher_get_tile_data_low_0();
        return;
    }

    // Note that fetcher determinate the tile data address to read both in
    // GetTileDataLow and GetTileDataHigh just before access it from VRAM.
    // Therefore, changes to (WY or?) BG_WIN_TILE_DATA during these phases may
    // have bitplane desync effects
    // (e.g. read the low byte from a tile and the high byte from a different one).
    // [mealybug/m3_lcdc_tile_sel_win_change]
    setup_win_pixel_slice_fetcher_tile_data_address();

#ifdef ENABLE_CGB
    if (test_bit<Specs::Bits::Background::Attributes::BANK>(bwf.attributes)) {
        psf.tile_data_low = vram.read<1>(psf.tile_data_vram_address);
    } else {
        psf.tile_data_low = vram.read<0>(psf.tile_data_vram_address);
    }
#else
    psf.tile_data_low = vram.read<0>(psf.tile_data_vram_address);
#endif

    fetcher_tick_selector = &Ppu::win_pixel_slice_fetcher_get_tile_data_low_1;
}

void Ppu::win_pixel_slice_fetcher_get_tile_data_low_1() {
    ASSERT(!is_fetching_sprite);

    // If the window is turned off, the fetcher switches back to the BG fetcher.
    if (!lcdc.win_enable) {
        bg_pixel_slice_fetcher_get_tile_data_low_1();
        return;
    }

    fetcher_tick_selector = &Ppu::win_pixel_slice_fetcher_get_tile_data_high_0;
}

void Ppu::win_pixel_slice_fetcher_get_tile_data_high_0() {
    ASSERT(!is_fetching_sprite);

    // If the window is turned off, the fetcher switches back to the BG fetcher.
    if (!lcdc.win_enable) {
        bg_pixel_slice_fetcher_get_tile_data_high_0();
        return;
    }

    // Note that fetcher determinate the tile data address to read both in
    // GetTileDataLow and GetTileDataHigh just before access it from VRAM.
    // Therefore, changes to (WY or?) BG_WIN_TILE_DATA during these phases may
    // have bitplane desync effects
    // (e.g. read the low byte from a tile and the high byte from a different one).
    // [mealybug/m3_lcdc_tile_sel_win_change]
    setup_win_pixel_slice_fetcher_tile_data_address();

#ifdef ENABLE_CGB
    if (test_bit<Specs::Bits::Background::Attributes::BANK>(bwf.attributes)) {
        psf.tile_data_high = vram.read<1>(psf.tile_data_vram_address + 1);
    } else {
        psf.tile_data_high = vram.read<0>(psf.tile_data_vram_address + 1);
    }
#else
    psf.tile_data_high = vram.read<0>(psf.tile_data_vram_address + 1);
#endif

    fetcher_tick_selector = &Ppu::bgwin_pixel_slice_fetcher_get_tile_data_high_1;
}

void Ppu::bgwin_pixel_slice_fetcher_get_tile_data_high_1() {
    ASSERT(!is_fetching_sprite);

    // The bg/win fetcher tile is increased only at this step, not before or after.
    // Therefore, if the window aborts a bg fetch before this step,
    // the bg tile does not increment: it increments only if the
    // fetcher reaches the push stage (or a sprite aborts it)
    bwf.lx += TILE_WIDTH; // automatically handle mod 8 by overflowing

    // If there is a pending obj hit (and the bg fifo is not empty),
    // discard the fetched bg pixels and restart the obj prefetcher with the obj hit
    // note: the first obj prefetcher tick should overlap this tick, not the push one
    if (is_obj_ready_to_be_fetched() && bg_fifo.is_not_empty()) {
        // The bg/win tile fetched is not thrown away: instead it is
        // cached so that the fetcher can start with it after the sprite
        // has been merged into obj fifo
        cache_bg_win_fetch();

        is_fetching_sprite = true;
        of.entry = oam_entries[lx].pull_back();
        obj_prefetcher_get_tile_0();
        return;
    }

    if (w.just_activated) {
        // The fetcher immediately pushes the window tile if the window
        // has just been activated (i.e. this is the first window fetch).
        ASSERT(bg_fifo.is_empty());
        bgwin_pixel_slice_fetcher_push();
        w.just_activated = false;
    } else {
        // Standard case.
        fetcher_tick_selector = &Ppu::bgwin_pixel_slice_fetcher_push;
    }
}

void Ppu::bgwin_pixel_slice_fetcher_push() {
    ASSERT(!is_fetching_sprite);

    /*
     * As far as I'm understanding, there are 4 possible cases:
     *
     * [canPushToBgFifo] | [isObjReadyToBeFetched]
     * ---------------------------------------------------------
     * bgFifo.IsEmpty()  | oamEntries[LX].isNotEmpty() &&
     *                   | test_bit<OBJ_ENABLE>(video.LCDC)
     * ---------------------------------------------------------
     *        0         |           0              | wait (nop)
     *        0         |           1              | cache bg fetch and start obj prefetcher (tick now)
     *        1         |           0              | push to bg fifo and prepare for bg/win prefetcher (tick next dot)
     *        1         |           1              | push to bg fifo and start obj prefetcher (tick now)
     */

    // The pixels can be pushed only if the bg fifo is empty,
    // otherwise wait in push state until bg fifo is emptied
    const bool can_push_to_bg_fifo = bg_fifo.is_empty();
    if (can_push_to_bg_fifo) {
#ifndef ENABLE_CGB
        // If window activation conditions were met in the frame, then a
        // glitch can occur: if this push stage happens when LX == WX,
        // than a 00 pixel is pushed into the bg fifo instead of the fetched tile.
        // Therefore, the push of the tile that was about to happen is postponed by 1 dot
        // TODO: verify this for CGB.
        if (w.active_for_frame && lx == last_wx && lx > 8 /* TODO: not sure about this */) {
            bg_fifo.push_back(BgPixel {0});
            return;
        }
#endif

        // Push pixels into bg fifo
#ifdef ENABLE_CGB
        ASSERT(bg_fifo.is_empty());

        PixelColorIndex pixel_data[8];
        const uint8_t (*pixels_map_ptr)[8] = test_bit<Specs::Bits::Background::Attributes::X_FLIP>(bwf.attributes)
                                                 ? TILE_ROW_DATA_TO_ROW_PIXELS_FLIPPED
                                                 : TILE_ROW_DATA_TO_ROW_PIXELS;
        memcpy(pixel_data, &pixels_map_ptr[concat(psf.tile_data_high, psf.tile_data_low)], 8);

        for (int8_t i = 7; i >= 0; i--) {
            bg_fifo.emplace_back(pixel_data[i], bwf.attributes);
        }
#else
        bg_fifo.fill(&TILE_ROW_DATA_TO_ROW_PIXELS[concat(psf.tile_data_high, psf.tile_data_low)]);
#endif

        fetcher_tick_selector = &Ppu::bgwin_prefetcher_get_tile_0;

        // Sprite fetches are ignored just after a window tile push.
        if (w.active) {
            return;
        }
    }

    // If there is a pending obj hit, discard the fetched bg pixels
    // and restart the obj prefetcher with the obj hit.
    // Note: the first obj prefetcher tick overlap this tick.
    if (is_obj_ready_to_be_fetched()) {
        if (!can_push_to_bg_fifo) {
            // The bg/win tile fetched is not thrown away: instead it is
            // cached so that the fetcher can start with it after the sprite
            // has been merged into obj fifo.
            cache_bg_win_fetch();
        }

        is_fetching_sprite = true;
        of.entry = oam_entries[lx].pull_back();
        obj_prefetcher_get_tile_0();
    }
}

// ------------- obj ---------------

void Ppu::obj_prefetcher_get_tile_0() {
    ASSERT(is_fetching_sprite);

    // Read two bytes from OAM (Tile Number and Attributes).
    // Note that if DMA transfer is in progress conflicts can occur and
    // we might end up reading from the OAM address the DMA is writing to
    // (but only if DMA writing request is pending, i.e. it is in T0 or T1).
    // [hacktix/strikethrough]

    // TODO: figure out exact timing of read request and read flush for obj prefetcher.
    oam.read_word_request(Specs::MemoryLayout::OAM::START + OAM_ENTRY_BYTES * of.entry.number +
                          Specs::Bytes::OAM::TILE_NUMBER);

    fetcher_tick_selector = &Ppu::obj_prefetcher_get_tile_1;
}

void Ppu::obj_prefetcher_get_tile_1() {
    ASSERT(is_fetching_sprite);

    // TODO: figure out exact timing of read request and read flush for obj prefetcher.
    registers.oam = oam.flush_read_word_request();

    of.tile_number = registers.oam.a;
    of.attributes = registers.oam.b;

    fetcher_tick_selector = &Ppu::obj_pixel_slice_fetcher_get_tile_data_low_0;
}

void Ppu::obj_pixel_slice_fetcher_get_tile_data_low_0() {
    ASSERT(is_fetching_sprite);
    ASSERT(bg_fifo.is_not_empty());

    fetcher_tick_selector = &Ppu::obj_pixel_slice_fetcher_get_tile_data_low_1;
}

void Ppu::obj_pixel_slice_fetcher_get_tile_data_low_1() {
    ASSERT(is_fetching_sprite);
    ASSERT(bg_fifo.is_not_empty());

    // Note that fetcher determinate the tile data address to read both in
    // GetTileDataLow and GetTileDataHigh just before access it from VRAM.
    // Therefore, changes to OBJ_SIZE during these phases may have bitplane
    // desync effects.
    // (e.g. read the low byte from a tile and the high byte from a different one).
    // [mealybug/m3_lcdc_obj_size_change, mealybug/m3_lcdc_obj_size_change_scx]
    setup_obj_pixel_slice_fetcher_tile_data_address();

#ifdef ENABLE_CGB
    if (test_bit<Specs::Bits::OAM::Attributes::BANK>(of.attributes)) {
        psf.tile_data_low = vram.read<1>(psf.tile_data_vram_address);
    } else {
        psf.tile_data_low = vram.read<0>(psf.tile_data_vram_address);
    }
#else
    psf.tile_data_low = vram.read<0>(psf.tile_data_vram_address);
#endif

    fetcher_tick_selector = &Ppu::obj_pixel_slice_fetcher_get_tile_data_high_0;
}

void Ppu::obj_pixel_slice_fetcher_get_tile_data_high_0() {
    ASSERT(is_fetching_sprite);
    ASSERT(bg_fifo.is_not_empty());

    fetcher_tick_selector = &Ppu::obj_pixel_slice_fetcher_get_tile_data_high_1_and_merge_with_obj_fifo;
}

void Ppu::obj_pixel_slice_fetcher_get_tile_data_high_1_and_merge_with_obj_fifo() {
    ASSERT(is_fetching_sprite);
    ASSERT(of.entry.x == lx);
    ASSERT(bg_fifo.is_not_empty());

    // Note that fetcher determinate the tile data address to read both in
    // GetTileDataLow and GetTileDataHigh just before access it from VRAM.
    // Therefore, changes to OBJ_SIZE during these phases may have bitplane
    // desync effects.
    // (e.g. read the low byte from a tile and the high byte from a different one).
    // [mealybug/m3_lcdc_obj_size_change, mealybug/m3_lcdc_obj_size_change_scx]
    setup_obj_pixel_slice_fetcher_tile_data_address();

#ifdef ENABLE_CGB
    if (test_bit<Specs::Bits::OAM::Attributes::BANK>(of.attributes)) {
        psf.tile_data_high = vram.read<1>(psf.tile_data_vram_address + 1);
    } else {
        psf.tile_data_high = vram.read<0>(psf.tile_data_vram_address + 1);
    }
#else
    psf.tile_data_high = vram.read<0>(psf.tile_data_vram_address + 1);
#endif

    PixelColorIndex obj_pixels_colors[8];
    const uint8_t (*pixels_map_ptr)[8] = test_bit<Specs::Bits::OAM::Attributes::X_FLIP>(of.attributes)
                                             ? TILE_ROW_DATA_TO_ROW_PIXELS_FLIPPED
                                             : TILE_ROW_DATA_TO_ROW_PIXELS;
    memcpy(obj_pixels_colors, &pixels_map_ptr[concat(psf.tile_data_high, psf.tile_data_low)], 8);

    ObjPixel obj_pixels[8];
    for (uint8_t i = 0; i < 8; i++) {
        obj_pixels[i] = {obj_pixels_colors[i], of.attributes, of.entry.number, lx};
    }

    // Handle sprite-to-sprite conflicts by merging the new obj pixels with
    // the pixels already in the obj fifo
    // "The smaller the X coordinate, the higher the priority."
    // "When X coordinates are identical, the object located first in OAM has higher priority."

    const uint8_t obj_fifoSize = obj_fifo.size();
    uint8_t i = 0;

    // Handle conflict between the new and the old obj pixels
    while (i < obj_fifoSize) {
        const ObjPixel& fifo_obj_pixel = obj_fifo[i];
        const ObjPixel& obj_pixel = obj_pixels[i];

        ASSERT(obj_pixel.x >= fifo_obj_pixel.x);

        // Overwrite pixel in fifo with pixel in sprite if:
        // 1. Pixel in sprite is opaque and pixel in fifo is transparent
        // 2. Both pixels in sprite and fifo are opaque but pixel in sprite has:
        //    > [lower x, or if x is equal, in CGB mode]
        //    > lowest oam number.
#ifdef ENABLE_CGB
        const bool overwrite_obj_pixel =
            (is_obj_opaque(obj_pixel.color_index) && !is_obj_opaque(fifo_obj_pixel.color_index)) ||
            ((is_obj_opaque(obj_pixel.color_index) && is_obj_opaque(fifo_obj_pixel.color_index)) &&
             (obj_pixel.number < fifo_obj_pixel.number));
#else
        const bool overwrite_obj_pixel =
            (is_obj_opaque(obj_pixel.color_index) && !is_obj_opaque(fifo_obj_pixel.color_index)) ||
            ((is_obj_opaque(obj_pixel.color_index) && is_obj_opaque(fifo_obj_pixel.color_index)) &&
             (obj_pixel.x == fifo_obj_pixel.x && obj_pixel.number < fifo_obj_pixel.number));
#endif

        if (overwrite_obj_pixel) {
            obj_fifo[i] = obj_pixel;
        }

        ++i;
    }

    // Push the remaining obj pixels
    while (i < 8) {
        obj_fifo.push_back(obj_pixels[i++]);
    }

    ASSERT(obj_fifo.is_full());

    if (is_obj_ready_to_be_fetched()) {
        // Still oam entries hit to be served for this x: setup the fetcher
        // for another obj fetch
        of.entry = oam_entries[lx].pull_back();
        fetcher_tick_selector = &Ppu::obj_prefetcher_get_tile_0;
    } else {
        // No more oam entries to serve for this x: setup to fetcher with
        // the cached tile data that has been interrupted by this obj fetch
        is_fetching_sprite = false;

        if (bwf.interrupted_fetch.has_data) {
            restore_bg_win_fetch();
            fetcher_tick_selector = &Ppu::bgwin_pixel_slice_fetcher_push;
        } else {
            fetcher_tick_selector = &Ppu::bgwin_prefetcher_get_tile_0;
        }
    }
}

// ------- Fetcher states helpers ---------

inline void Ppu::setup_obj_pixel_slice_fetcher_tile_data_address() {
    const bool is_double_height = lcdc.obj_size;
    const uint8_t height_mask = is_double_height ? 0xF : 0x7;

    const uint8_t obj_y = of.entry.y - TILE_DOUBLE_HEIGHT;

    // The OBJ tileY is always mapped within the range [0:objHeight).
    uint8_t tile_y = (ly - obj_y) & height_mask;

    if (test_bit<Specs::Bits::OAM::Attributes::Y_FLIP>(of.attributes)) {
        // Take the opposite row within objHeight.
        tile_y ^= height_mask;
    }

    // Last bit is ignored for 8x16 OBJs.
    const uint8_t tile_number = (is_double_height ? (discard_bits<1>(of.tile_number)) : of.tile_number);

    const uint16_t vram_tile_addr = TILE_BYTES * tile_number;

    psf.tile_data_vram_address = vram_tile_addr + TILE_ROW_BYTES * tile_y;
}

inline void Ppu::setup_bg_pixel_slice_fetcher_tilemap_tile_address() {
    const uint8_t tilemap_x = mod<TILEMAP_WIDTH>((bwf.lx + scx) / TILE_WIDTH);
    const uint8_t tilemap_y = mod<TILEMAP_HEIGHT>((ly + scy) / TILE_HEIGHT);

    const uint16_t tilemap_vram_addr_base = lcdc.bg_tile_map ? 0x1C00 : 0x1800; // 0x9800 or 0x9C00 (global)
    const uint16_t tilemap_vram_addr_slack = (TILEMAP_WIDTH * TILEMAP_CELL_BYTES * tilemap_y) + tilemap_x;
    bwf.tilemap_tile_vram_addr = tilemap_vram_addr_base + tilemap_vram_addr_slack;

#ifdef ENABLE_DEBUGGER
    bwf.tilemap_x = tilemap_x;
    bwf.tilemap_y = tilemap_y;
    bwf.tilemap_vram_addr = tilemap_vram_addr_base;
#endif
}

inline void Ppu::setup_bg_pixel_slice_fetcher_tile_data_address() {
#ifdef ENABLE_CGB
    bwf.attributes = vram.read<1>(bwf.tilemap_tile_vram_addr);
#endif

    const uint8_t tile_number = vram.read<0>(bwf.tilemap_tile_vram_addr);

    const uint16_t vram_tile_addr = lcdc.bg_win_tile_data
                                        ?
                                        // unsigned addressing mode with 0x8000 as (global) base address
                                        0x0000 + TILE_BYTES * tile_number
                                        :
                                        // signed addressing mode with 0x9000 as (global) base address
                                        0x1000 + TILE_BYTES * static_cast<int8_t>(tile_number);
    uint8_t tile_y = mod<TILE_HEIGHT>(ly + scy);

#ifdef ENABLE_CGB
    if (test_bit<Specs::Bits::Background::Attributes::Y_FLIP>(bwf.attributes)) {
        // Take the opposite row within tile height.
        tile_y ^= 0x7;
    }
#endif

    ASSERT(tile_y < 8);

    psf.tile_data_vram_address = vram_tile_addr + TILE_ROW_BYTES * tile_y;
}

inline void Ppu::setup_win_pixel_slice_fetcher_tilemap_tile_address() {
    // The window prefetcher has its own internal counter to determine the tile to fetch
    const uint8_t tilemap_x = wf.tilemap_x++;
    const uint8_t tilemap_y = w.wly / TILE_HEIGHT;

    const uint16_t tilemap_vram_addr_base = lcdc.win_tile_map ? 0x1C00 : 0x1800; // 0x9800 or 0x9C00 (global)
    const uint16_t tilemap_vram_addr_slack = (TILEMAP_WIDTH * TILEMAP_CELL_BYTES * tilemap_y) + tilemap_x;
    bwf.tilemap_tile_vram_addr = tilemap_vram_addr_base + tilemap_vram_addr_slack;

#ifdef ENABLE_DEBUGGER
    bwf.tilemap_x = tilemap_x;
    bwf.tilemap_y = tilemap_y;
    bwf.tilemap_vram_addr = tilemap_vram_addr_base;
#endif
}

inline void Ppu::setup_win_pixel_slice_fetcher_tile_data_address() {
#ifdef ENABLE_CGB
    bwf.attributes = vram.read<1>(bwf.tilemap_tile_vram_addr);
#endif

    const uint8_t tile_number = vram.read<0>(bwf.tilemap_tile_vram_addr);

    const uint16_t vram_tile_addr = lcdc.bg_win_tile_data
                                        ?
                                        // unsigned addressing mode with 0x8000 as (global) base address
                                        0x0000 + TILE_BYTES * tile_number
                                        :
                                        // signed addressing mode with 0x9000 as (global) base address
                                        0x1000 + TILE_BYTES * static_cast<int8_t>(tile_number);

    uint8_t tile_y = mod<TILE_HEIGHT>(w.wly);

#ifdef ENABLE_CGB
    if (test_bit<Specs::Bits::Background::Attributes::Y_FLIP>(bwf.attributes)) {
        // Take the opposite row within objHeight.
        tile_y ^= 0x7;
    }
#endif

    ASSERT(tile_y < 8);

    psf.tile_data_vram_address = vram_tile_addr + TILE_ROW_BYTES * tile_y;
}

inline void Ppu::cache_bg_win_fetch() {
    bwf.interrupted_fetch.tile_data_low = psf.tile_data_low;
    bwf.interrupted_fetch.tile_data_high = psf.tile_data_high;
    ASSERT(!bwf.interrupted_fetch.has_data);
    bwf.interrupted_fetch.has_data = true;
}

inline void Ppu::restore_bg_win_fetch() {
    psf.tile_data_low = bwf.interrupted_fetch.tile_data_low;
    psf.tile_data_high = bwf.interrupted_fetch.tile_data_high;
    ASSERT(bwf.interrupted_fetch.has_data);
    bwf.interrupted_fetch.has_data = false;
}

void Ppu::save_state(Parcel& parcel) const {
    parcel.write_bool(lcdc.enable);
    parcel.write_bool(lcdc.win_tile_map);
    parcel.write_bool(lcdc.win_enable);
    parcel.write_bool(lcdc.bg_win_tile_data);
    parcel.write_bool(lcdc.bg_tile_map);
    parcel.write_bool(lcdc.obj_size);
    parcel.write_bool(lcdc.obj_enable);
    parcel.write_bool(lcdc.bg_win_enable);

    parcel.write_bool(stat.lyc_eq_ly_int);
    parcel.write_bool(stat.oam_int);
    parcel.write_bool(stat.vblank_int);
    parcel.write_bool(stat.hblank_int);
    parcel.write_bool(stat.lyc_eq_ly);
    parcel.write_uint8(stat.mode);

    parcel.write_uint8(scy);
    parcel.write_uint8(scx);
    parcel.write_uint8(ly);
    parcel.write_uint8(lyc);
    parcel.write_uint8(dma);
    parcel.write_uint8(bgp);
    parcel.write_uint8(obp0);
    parcel.write_uint8(obp1);
    parcel.write_uint8(wy);
    parcel.write_uint8(wx);

#ifdef ENABLE_CGB
    parcel.write_bool(bcps.auto_increment);
    parcel.write_uint8(bcps.address);

    parcel.write_bool(ocps.auto_increment);
    parcel.write_uint8(ocps.address);

    parcel.write_bool(opri.priority_mode);
#endif

    {
        uint8_t i = 0;

        while (i < (uint8_t)array_size(TICK_SELECTORS) && tick_selector != TICK_SELECTORS[i]) {
            ++i;
        }
        ASSERT(i < array_size(TICK_SELECTORS));
        parcel.write_uint8(i);

        i = 0;
        while (i < (uint8_t)array_size(FETCHER_TICK_SELECTORS) && fetcher_tick_selector != FETCHER_TICK_SELECTORS[i]) {
            ++i;
        }
        ASSERT(i < array_size(FETCHER_TICK_SELECTORS));
        parcel.write_uint8(i);
    }

    parcel.write_bool(last_stat_irq);
    parcel.write_bool(enable_lyc_eq_ly_irq);
    parcel.write_bool(pending_stat_irq);
#ifdef ENABLE_CGB
    parcel.write_uint8(stat_mode);
    parcel.write_bool(delay_stat_mode_update);
#endif

#ifndef ENABLE_CGB
    parcel.write_bool(stat_write.pending);
    parcel.write_uint8(stat_write.value);
#endif
    parcel.write_uint16(dots);
    parcel.write_uint8(lx);
    parcel.write_uint8(mode);
    parcel.write_uint8(last_ly);
    parcel.write_uint8(last_lyc);
    parcel.write_uint8(last_bgp);
    parcel.write_uint8(last_wx);

    parcel.write_bool(last_lcdc.enable);
    parcel.write_bool(last_lcdc.win_tile_map);
    parcel.write_bool(last_lcdc.win_enable);
    parcel.write_bool(last_lcdc.bg_win_tile_data);
    parcel.write_bool(last_lcdc.bg_tile_map);
    parcel.write_bool(last_lcdc.obj_size);
    parcel.write_bool(last_lcdc.obj_enable);
    parcel.write_bool(last_lcdc.bg_win_enable);

    parcel.write_bytes(bg_fifo.data, sizeof(bg_fifo.data));
    parcel.write_uint8(bg_fifo.cursor);

    parcel.write_bytes(obj_fifo.data, sizeof(obj_fifo.data));
    parcel.write_uint8(obj_fifo.start);
    parcel.write_uint8(obj_fifo.end);
    parcel.write_uint8(obj_fifo.count);

    parcel.write_bytes(oam_entries, sizeof(oam_entries));
#ifdef ENABLE_ASSERTS
    parcel.write_uint8(oam_entries_count);
    parcel.write_uint8(oam_entries_not_served_count);
#endif

    parcel.write_bool(is_fetching_sprite);

    parcel.write_bool(is_glitched_line_0);
    parcel.write_uint8(glitched_line_0_hblank_delay);
#ifndef ENABLE_CGB
    parcel.write_bool(vblank_last_line_stat_mode_hblank_glitch);
#endif
    parcel.write_uint8(next_ly);

    parcel.write_uint8(registers.oam.a);
    parcel.write_uint8(registers.oam.b);

    parcel.write_uint8(oam_scan.count);
    parcel.write_uint8(oam_scan.index);

    parcel.write_uint8(pixel_transfer.initial_scx.to_discard);
    parcel.write_uint8(pixel_transfer.initial_scx.discarded);

    parcel.write_uint8(w.wly);
    parcel.write_bool(w.active);
    parcel.write_bool(w.just_activated);

#if defined(ENABLE_DEBUGGER) || defined(ENABLE_ASSERTS)
    for (uint8_t i = 0; i < decltype(w.line_triggers)::Size; i++) {
        parcel.write_uint8(i < w.line_triggers.size() ? w.line_triggers[i] : UINT8_MAX);
    }
#endif

    parcel.write_uint8(bwf.lx);
    parcel.write_uint8(bwf.tilemap_tile_vram_addr);
#ifdef ENABLE_CGB
    parcel.write_uint8(bwf.attributes);
#endif
#ifdef ENABLE_DEBUGGER
    parcel.write_uint8(bwf.tilemap_x);
    parcel.write_uint8(bwf.tilemap_y);
    parcel.write_uint8(bwf.tilemap_vram_addr);
#endif
    parcel.write_bool(bwf.interrupted_fetch.has_data);
    parcel.write_uint8(bwf.interrupted_fetch.tile_data_low);
    parcel.write_uint8(bwf.interrupted_fetch.tile_data_high);

    parcel.write_uint8(wf.tilemap_x);

    parcel.write_uint8(of.entry.number);
    parcel.write_uint8(of.entry.y);
#ifdef ENABLE_ASSERTS
    parcel.write_uint8(of.entry.x);
#endif
    parcel.write_uint8(of.tile_number);
    parcel.write_uint8(of.attributes);

    parcel.write_uint16(psf.tile_data_vram_address);
    parcel.write_uint8(psf.tile_data_low);
    parcel.write_uint8(psf.tile_data_high);

#ifdef ENABLE_CGB
    parcel.write_bytes(bg_palettes, sizeof(bg_palettes));
    parcel.write_bytes(obj_palettes, sizeof(obj_palettes));

    parcel.write_bool(color_resolver_enabled);
#endif

#ifdef ENABLE_DEBUGGER
    parcel.write_uint64(cycles);
#endif
}

void Ppu::load_state(Parcel& parcel) {
    lcdc.enable = parcel.read_bool();
    lcdc.win_tile_map = parcel.read_bool();
    lcdc.win_enable = parcel.read_bool();
    lcdc.bg_win_tile_data = parcel.read_bool();
    lcdc.bg_tile_map = parcel.read_bool();
    lcdc.obj_size = parcel.read_bool();
    lcdc.obj_enable = parcel.read_bool();
    lcdc.bg_win_enable = parcel.read_bool();

    stat.lyc_eq_ly_int = parcel.read_bool();
    stat.oam_int = parcel.read_bool();
    stat.vblank_int = parcel.read_bool();
    stat.hblank_int = parcel.read_bool();
    stat.lyc_eq_ly = parcel.read_bool();
    stat.mode = parcel.read_uint8();

    scy = parcel.read_uint8();
    scx = parcel.read_uint8();
    ly = parcel.read_uint8();
    lyc = parcel.read_uint8();
    dma = parcel.read_uint8();
    bgp = parcel.read_uint8();
    obp0 = parcel.read_uint8();
    obp1 = parcel.read_uint8();
    wy = parcel.read_uint8();
    wx = parcel.read_uint8();

#ifdef ENABLE_CGB
    bcps.auto_increment = parcel.read_bool();
    bcps.address = parcel.read_uint8();

    ocps.auto_increment = parcel.read_bool();
    ocps.address = parcel.read_uint8();

    opri.priority_mode = parcel.read_bool();
#endif

    const uint8_t tick_selector_number = parcel.read_uint8();
    ASSERT(tick_selector_number < array_size(TICK_SELECTORS));
    tick_selector = TICK_SELECTORS[tick_selector_number];

    const uint8_t fetcher_tick_selector_number = parcel.read_uint8();
    ASSERT(fetcher_tick_selector_number < array_size(FETCHER_TICK_SELECTORS));
    fetcher_tick_selector = FETCHER_TICK_SELECTORS[fetcher_tick_selector_number];

    last_stat_irq = parcel.read_bool();
    enable_lyc_eq_ly_irq = parcel.read_bool();
    pending_stat_irq = parcel.read_bool();
#ifdef ENABLE_CGB
    stat_mode = parcel.read_uint8();
    delay_stat_mode_update = parcel.read_bool();
#endif
#ifndef ENABLE_CGB
    stat_write.pending = parcel.read_bool();
    stat_write.value = parcel.read_uint8();
#endif
    dots = parcel.read_uint16();
    lx = parcel.read_uint8();
    mode = parcel.read_uint8();
    last_ly = parcel.read_uint8();
    last_lyc = parcel.read_uint8();
    last_bgp = parcel.read_uint8();
    last_wx = parcel.read_uint8();

    last_lcdc.enable = parcel.read_bool();
    last_lcdc.win_tile_map = parcel.read_bool();
    last_lcdc.win_enable = parcel.read_bool();
    last_lcdc.bg_win_tile_data = parcel.read_bool();
    last_lcdc.bg_tile_map = parcel.read_bool();
    last_lcdc.obj_size = parcel.read_bool();
    last_lcdc.obj_enable = parcel.read_bool();
    last_lcdc.bg_win_enable = parcel.read_bool();

    parcel.read_bytes(bg_fifo.data, sizeof(bg_fifo.data));
    bg_fifo.cursor = parcel.read_uint8();

    parcel.read_bytes(obj_fifo.data, sizeof(obj_fifo.data));
    obj_fifo.start = parcel.read_uint8();
    obj_fifo.end = parcel.read_uint8();
    obj_fifo.count = parcel.read_uint8();

    parcel.read_bytes(oam_entries, sizeof(oam_entries));
#ifdef ENABLE_ASSERTS
    oam_entries_count = parcel.read_uint8();
    oam_entries_not_served_count = parcel.read_uint8();
#endif

    is_fetching_sprite = parcel.read_bool();

    is_glitched_line_0 = parcel.read_bool();
    glitched_line_0_hblank_delay = parcel.read_uint8();
#ifndef ENABLE_CGB
    vblank_last_line_stat_mode_hblank_glitch = parcel.read_bool();
#endif
    next_ly = parcel.read_uint8();

    registers.oam.a = parcel.read_uint8();
    registers.oam.b = parcel.read_uint8();

    oam_scan.count = parcel.read_uint8();
    oam_scan.index = parcel.read_uint8();

    pixel_transfer.initial_scx.to_discard = parcel.read_uint8();
    pixel_transfer.initial_scx.discarded = parcel.read_uint8();

    w.wly = parcel.read_uint8();
    w.active = parcel.read_bool();
    w.just_activated = parcel.read_bool();

#if defined(ENABLE_DEBUGGER) || defined(ENABLE_ASSERTS)
    w.line_triggers.clear();
    for (uint8_t i = 0; i < decltype(w.line_triggers)::Size; i++) {
        if (uint8_t line_trigger = parcel.read_uint8(); line_trigger != UINT8_MAX) {
            w.line_triggers.push_back(line_trigger);
        }
    }
#endif

    bwf.lx = parcel.read_uint8();
    bwf.tilemap_tile_vram_addr = parcel.read_uint8();
#ifdef ENABLE_CGB
    bwf.attributes = parcel.read_uint8();
#endif
#ifdef ENABLE_DEBUGGER
    bwf.tilemap_x = parcel.read_uint8();
    bwf.tilemap_y = parcel.read_uint8();
    bwf.tilemap_vram_addr = parcel.read_uint8();
#endif
    bwf.interrupted_fetch.has_data = parcel.read_bool();
    bwf.interrupted_fetch.tile_data_low = parcel.read_uint8();
    bwf.interrupted_fetch.tile_data_high = parcel.read_uint8();

    wf.tilemap_x = parcel.read_uint8();

    of.entry.number = parcel.read_uint8();
    of.entry.y = parcel.read_uint8();
#ifdef ENABLE_ASSERTS
    of.entry.x = parcel.read_uint8();
#endif
    of.tile_number = parcel.read_uint8();
    of.attributes = parcel.read_uint8();

    psf.tile_data_vram_address = parcel.read_uint16();
    psf.tile_data_low = parcel.read_uint8();
    psf.tile_data_high = parcel.read_uint8();

#ifdef ENABLE_CGB
    parcel.read_bytes(bg_palettes, sizeof(bg_palettes));
    parcel.read_bytes(obj_palettes, sizeof(obj_palettes));

    color_resolver_enabled = parcel.read_bool();
#endif

#ifdef ENABLE_DEBUGGER
    cycles = parcel.read_uint64();
#endif
}

void Ppu::reset() {
    lcdc.enable = if_bootrom_else(false, true);
    lcdc.win_tile_map = false;
    lcdc.win_enable = false;
    lcdc.bg_win_tile_data = if_bootrom_else(false, true);
    lcdc.bg_tile_map = false;
    lcdc.obj_size = false;
    lcdc.obj_enable = false;
    lcdc.bg_win_enable = if_bootrom_else(false, true);

    stat.lyc_eq_ly_int = false;
    stat.oam_int = false;
    stat.vblank_int = false;
    stat.hblank_int = false;
#ifdef ENABLE_CGB
    // TODO: with bootrom
    stat.lyc_eq_ly = false;
    stat.mode = VBLANK;
#else
    stat.lyc_eq_ly = if_bootrom_else(false, true);
    stat.mode = if_bootrom_else(HBLANK, VBLANK);
#endif

    scy = 0;
    scx = 0;
#ifdef ENABLE_CGB
    // TODO: with bootrom
    ly = 0x90;
#else
    ly = 0;
#endif
    lyc = 0;
#ifdef ENABLE_CGB
    dma = 0;
#else
    dma = 0xFF;
#endif
    bgp = if_bootrom_else(0, 0xFC);
    obp0 = 0;
    obp1 = 0;
    wy = 0;
    wx = 0;

#ifdef ENABLE_CGB
    bcps.auto_increment = true;
    bcps.address = 0;

    ocps.auto_increment = true;
    ocps.address = 1;

    opri.priority_mode = false;
#endif

#ifdef ENABLE_CGB
    // TODO: with bootrom
    tick_selector = &Ppu::vblank;
#else
    tick_selector = if_bootrom_else(&Ppu::oam_scan_even, &Ppu::vblank_last_line_7);
#endif
    fetcher_tick_selector = &Ppu::bg_prefetcher_get_tile_0;

    last_stat_irq = false;
    enable_lyc_eq_ly_irq = true;
    pending_stat_irq = false;

#ifdef ENABLE_CGB
    stat_mode = stat.mode;
    delay_stat_mode_update = false;
#endif

#ifdef ENABLE_CGB
    // TODO: with bootrom
    dots = if_bootrom_else(0, 163);
#else
    dots = if_bootrom_else(0, 395);
#endif
    lx = 0;

    mode = stat.mode;

    last_ly = ly;
    last_lyc = lyc;
    last_bgp = if_bootrom_else(0, 0xFC);
    last_wx = 0;

    bg_fifo.clear();
    obj_fifo.clear();

    for (uint32_t i = 0; i < array_size(oam_entries); i++) {
        oam_entries[i].clear();
    }

#ifdef ENABLE_ASSERTS
    oam_entries_count = 0;
    oam_entries_not_served_count = 0;
#endif

#ifdef ENABLE_DEBUGGER
    scanline_oam_entries.clear();
#endif

    is_fetching_sprite = false;

    is_glitched_line_0 = false;
    glitched_line_0_hblank_delay = 0;
#ifndef ENABLE_CGB
    vblank_last_line_stat_mode_hblank_glitch = false;
#endif

    next_ly = 0;

#ifdef ENABLE_DEBUGGER
    timings.oam_scan = 0;
    timings.pixel_transfer = 0;
    timings.hblank = 0;
#endif

    registers.oam.a = 0;
    registers.oam.a = 0;

    oam_scan.count = 0;
    oam_scan.index = 0;

    pixel_transfer.initial_scx.to_discard = 0;
    pixel_transfer.initial_scx.discarded = 0;

    w.active_for_frame = false;
    w.wly = UINT8_MAX;
    w.active = false;
    w.just_activated = false;
#if defined(ENABLE_DEBUGGER) || defined(ENABLE_ASSERTS)
    w.line_triggers.clear();
#endif

    bwf.lx = 0;
    bwf.tilemap_tile_vram_addr = 0;

#ifdef ENABLE_DEBUGGER
    bwf.tilemap_x = 0;
    bwf.tilemap_y = 0;
    bwf.tilemap_vram_addr = 0;
#endif

    bwf.interrupted_fetch.has_data = false;
    bwf.interrupted_fetch.tile_data_low = 0;
    bwf.interrupted_fetch.tile_data_high = 0;
    wf.tilemap_x = 0;

    of.entry.number = 0;
    of.entry.y = 0;
#if defined(ENABLE_DEBUGGER) || defined(ENABLE_ASSERTS)
    of.entry.x = 0;
#endif
    of.tile_number = 0;
    of.attributes = 0;

    psf.tile_data_vram_address = 0;
    psf.tile_data_low = 0;
    psf.tile_data_high = 0;

#ifdef ENABLE_CGB
#ifndef ENABLE_BOOTROM
    // BG palettes are all initialized as white.
    for (uint32_t i = 0; i < 32; i++) {
        bg_palettes[2 * i] = 0xFF;
        bg_palettes[2 * i + 1] = 0x7F;
    }
    // OBJ palettes are random.
    memcpy(obj_palettes, RANDOM_DATA, sizeof(obj_palettes));
#else
    for (uint32_t i = 0; i < 64; i++) {
        obj_palettes[i] = 0;
    }
    for (uint32_t i = 0; i < 64; i++) {
        obj_palettes[i] = 0;
    }
#endif

    color_resolver_enabled = true;
#endif

#ifdef ENABLE_DEBUGGER
    cycles = 0;
#endif

#ifndef ENABLE_BOOTROM
    oam.acquire();
#endif
}

uint8_t Ppu::read_lcdc() const {
    return lcdc.enable << Specs::Bits::Video::LCDC::LCD_ENABLE |
           lcdc.win_tile_map << Specs::Bits::Video::LCDC::WIN_TILE_MAP |
           lcdc.win_enable << Specs::Bits::Video::LCDC::WIN_ENABLE |
           lcdc.bg_win_tile_data << Specs::Bits::Video::LCDC::BG_WIN_TILE_DATA |
           lcdc.bg_tile_map << Specs::Bits::Video::LCDC::BG_TILE_MAP |
           lcdc.obj_size << Specs::Bits::Video::LCDC::OBJ_SIZE |
           lcdc.obj_enable << Specs::Bits::Video::LCDC::OBJ_ENABLE |
           lcdc.bg_win_enable << Specs::Bits::Video::LCDC::BG_WIN_ENABLE;
}

void Ppu::write_lcdc(uint8_t value) {
    const bool en = test_bit<Specs::Bits::Video::LCDC::LCD_ENABLE>(value);
    if (en != lcdc.enable) {
        en ? turn_on() : turn_off();
        lcdc.enable = en;
    }
    lcdc.win_tile_map = test_bit<Specs::Bits::Video::LCDC::WIN_TILE_MAP>(value);
    lcdc.win_enable = test_bit<Specs::Bits::Video::LCDC::WIN_ENABLE>(value);
    lcdc.bg_win_tile_data = test_bit<Specs::Bits::Video::LCDC::BG_WIN_TILE_DATA>(value);
    lcdc.bg_tile_map = test_bit<Specs::Bits::Video::LCDC::BG_TILE_MAP>(value);
    lcdc.obj_size = test_bit<Specs::Bits::Video::LCDC::OBJ_SIZE>(value);
    lcdc.obj_enable = test_bit<Specs::Bits::Video::LCDC::OBJ_ENABLE>(value);
    lcdc.bg_win_enable = test_bit<Specs::Bits::Video::LCDC::BG_WIN_ENABLE>(value);
}

uint8_t Ppu::read_stat() const {
#ifndef ENABLE_CGB
    ASSERT(!stat_write.pending);
#endif

    return 0b10000000 | stat.lyc_eq_ly_int << Specs::Bits::Video::STAT::LYC_EQ_LY_INTERRUPT |
           stat.oam_int << Specs::Bits::Video::STAT::OAM_INTERRUPT |
           stat.vblank_int << Specs::Bits::Video::STAT::VBLANK_INTERRUPT |
           stat.hblank_int << Specs::Bits::Video::STAT::HBLANK_INTERRUPT |
           stat.lyc_eq_ly << Specs::Bits::Video::STAT::LYC_EQ_LY |
#ifdef ENABLE_CGB
           stat_mode
#else
           stat.mode;
#endif
        ;
}

void Ppu::write_stat(uint8_t value) {
#ifdef ENABLE_CGB
    write_stat_real(value);
#else
    // STAT write takes effect 1 T-Cycle later on DMG, and during this T-Cycle it behaves as if FF
    // would have been written, i.e. all the STAT interrupts are enabled.
    stat_write.pending = true;
    stat_write.value = value;

    stat.lyc_eq_ly_int = true;
    stat.oam_int = true;
    stat.vblank_int = true;
    stat.hblank_int = true;
#endif
}

void Ppu::write_dma(uint8_t value) {
    dma = value;
    dma_controller.start_transfer(dma);
}

#ifdef ENABLE_CGB
uint8_t Ppu::read_bcps() const {
    return 0b01000000 | bcps.auto_increment << Specs::Bits::Video::BCPS::AUTO_INCREMENT | bcps.address;
}

void Ppu::write_bcps(uint8_t value) {
    bcps.auto_increment = test_bit<Specs::Bits::Video::BCPS::AUTO_INCREMENT>(value);
    bcps.address = get_bits_range<Specs::Bits::Video::BCPS::ADDRESS>(value);
}

uint8_t Ppu::read_bcpd() const {
    ASSERT(bcps.address < array_size(bg_palettes));
    return bg_palettes[bcps.address];
}

void Ppu::write_bcpd(uint8_t value) {
    ASSERT(bcps.address < array_size(bg_palettes));
    bg_palettes[bcps.address] = value;
    if (bcps.auto_increment) {
        bcps.address = mod<64>(bcps.address + 1);
    }
}

uint8_t Ppu::read_ocps() const {
    return 0b01000000 | ocps.auto_increment << Specs::Bits::Video::OCPS::AUTO_INCREMENT | ocps.address;
}

void Ppu::write_ocps(uint8_t value) {
    ocps.auto_increment = test_bit<Specs::Bits::Video::OCPS::AUTO_INCREMENT>(value);
    ocps.address = get_bits_range<Specs::Bits::Video::OCPS::ADDRESS>(value);
}

uint8_t Ppu::read_ocpd() const {
    ASSERT(ocps.address < array_size(obj_palettes));
    return obj_palettes[ocps.address];
}

void Ppu::write_ocpd(uint8_t value) {
    ASSERT(ocps.address < array_size(obj_palettes));
    obj_palettes[ocps.address] = value;
    if (ocps.auto_increment) {
        ocps.address = mod<64>(ocps.address + 1);
    }
}

uint8_t Ppu::read_opri() const {
    return 0xFE | opri.priority_mode;
}

void Ppu::write_opri(uint8_t value) {
    opri.priority_mode = test_bit<Specs::Bits::Video::OPRI::PRIORITY_MODE>(value);
}

#endif

#ifdef ENABLE_DEBUGGER
inline Ppu::Lcdc::Lcdc(bool watch) {
    if (!watch) {
        enable.unwatch();
        win_tile_map.unwatch();
        win_enable.unwatch();
        bg_win_tile_data.unwatch();
        bg_tile_map.unwatch();
        obj_size.unwatch();
        obj_enable.unwatch();
        bg_win_enable.unwatch();
    }
}

inline Ppu::Lcdc::Lcdc(const Ppu::Lcdc& other) :
    Lcdc(false) {
    assign(other);
}

inline Ppu::Lcdc& Ppu::Lcdc::operator=(const Ppu::Lcdc& other) {
    assign(other);
    return *this;
}

inline void Ppu::Lcdc::assign(const Ppu::Lcdc& other) {
    enable = (bool)other.enable;
    win_tile_map = (bool)other.win_tile_map;
    win_enable = (bool)other.win_enable;
    bg_win_tile_data = (bool)other.bg_win_tile_data;
    bg_tile_map = (bool)other.bg_tile_map;
    obj_size = (bool)other.obj_size;
    obj_enable = (bool)other.obj_enable;
    bg_win_enable = (bool)other.bg_win_enable;
}
#endif
