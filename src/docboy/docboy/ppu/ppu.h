#ifndef PPU_H
#define PPU_H

#include <vector>

#include "docboy/bus/oambus.h"
#include "docboy/bus/vrambus.h"

#include "utils/arrays.h"
#include "utils/asserts.h"
#include "utils/fillqueue.h"
#include "utils/queue.h"
#include "utils/vector.h"

class Lcd;
class Interrupts;
class Dma;
class Hdma;
class Parcel;

class Ppu {
    DEBUGGABLE_CLASS()

public:
#ifdef ENABLE_CGB
    Ppu(Lcd& lcd, Interrupts& interrupts, Hdma& hdma, VramBus::View<Device::Ppu> vram_bus,
        OamBus::View<Device::Ppu> oam_bus, Dma& dma_controller);
#else
    Ppu(Lcd& lcd, Interrupts& interrupts, VramBus::View<Device::Ppu> vram_bus, OamBus::View<Device::Ppu> oam_bus,
        Dma& dma_controller);
#endif

    void tick();

    void save_state(Parcel& parcel) const;
    void load_state(Parcel& parcel);

    void reset();

    // PPU I/O registers
    uint8_t read_lcdc() const;
    void write_lcdc(uint8_t value);

    uint8_t read_stat() const;
    void write_stat(uint8_t value);

    void write_dma(uint8_t value);

#ifdef ENABLE_CGB
    uint8_t read_bcps() const;
    void write_bcps(uint8_t value);

    uint8_t read_bcpd() const;
    void write_bcpd(uint8_t value);

    uint8_t read_ocps() const;
    void write_ocps(uint8_t value);

    uint8_t read_ocpd() const;
    void write_ocpd(uint8_t value);

    uint8_t read_opri() const;
    void write_opri(uint8_t value);
#endif

    struct Lcdc : Composite<Specs::Registers::Video::LCDC> {
#ifdef ENABLE_DEBUGGER
        explicit Lcdc(bool watch = false);
        Lcdc(const Lcdc& lcdc);
        Lcdc& operator=(const Lcdc& lcdc);

        void assign(const Lcdc& lcdc);
#endif

        Bool enable {make_bool()};
        Bool win_tile_map {make_bool()};
        Bool win_enable {make_bool()};
        Bool bg_win_tile_data {make_bool()};
        Bool bg_tile_map {make_bool()};
        Bool obj_size {make_bool()};
        Bool obj_enable {make_bool()};
        Bool bg_win_enable {make_bool()};
    } lcdc {
#ifdef ENABLE_DEBUGGER
        true /* watch */
#endif
    };

    struct Stat : Composite<Specs::Registers::Video::STAT> {
        Bool lyc_eq_ly_int {make_bool()};
        Bool oam_int {make_bool()};
        Bool vblank_int {make_bool()};
        Bool hblank_int {make_bool()};
        Bool lyc_eq_ly {make_bool()};
        UInt8 mode {make_uint8()};
    } stat {};

    UInt8 scy {make_uint8(Specs::Registers::Video::SCY)};
    UInt8 scx {make_uint8(Specs::Registers::Video::SCX)};
    UInt8 ly {make_uint8(Specs::Registers::Video::LY)};
    UInt8 lyc {make_uint8(Specs::Registers::Video::LYC)};
    UInt8 dma {make_uint8(Specs::Registers::Video::DMA)};
    UInt8 bgp {make_uint8(Specs::Registers::Video::BGP)};
    UInt8 obp0 {make_uint8(Specs::Registers::Video::OBP0)};
    UInt8 obp1 {make_uint8(Specs::Registers::Video::OBP1)};
    UInt8 wy {make_uint8(Specs::Registers::Video::WY)};
    UInt8 wx {make_uint8(Specs::Registers::Video::WX)};

#ifdef ENABLE_CGB
    struct Bcps : Composite<Specs::Registers::Video::BCPS> {
        Bool auto_increment {make_bool()};
        UInt8 address {make_uint8()};
    } bcps {};

    struct Ocps : Composite<Specs::Registers::Video::OCPS> {
        Bool auto_increment {make_bool()};
        UInt8 address {make_uint8()};
    } ocps {};

    struct Opri : Composite<Specs::Registers::Video::OPRI> {
        Bool priority_mode {make_bool()};
    } opri {};
#endif

private:
    using TickSelector = void (Ppu::*)();
    using FetcherTickSelector = void (Ppu::*)();
    using PixelColorIndex = uint8_t;

#ifdef ENABLE_CGB
    struct BgPixel {
        PixelColorIndex color_index;
        uint8_t attributes;
    };
#else
    struct BgPixel {
        PixelColorIndex color_index;
    };
#endif

    struct ObjPixel {
        PixelColorIndex color_index;
        uint8_t attributes;
        uint8_t number; // TODO: probably can be removed since oam scan is ordered by number
        uint8_t x;
    };

    struct OamScanEntry {
        uint8_t number; // [0, 40)
        uint8_t y;
#if defined(ENABLE_DEBUGGER) || defined(ENABLE_ASSERTS)
        uint8_t x;
#endif
    };

    // PPU helpers
    void turn_on();
    void turn_off();

    bool is_lyc_eq_ly() const;

    void tick_stat();
    void update_state_irq(bool irq);

    void tick_window();

    // PPU states
    void oam_scan_even();
    void oam_scan_odd();
    void oam_scan_done();
    void oam_scan_after_turn_on();

    void pixel_transfer_dummy_lx0();
    void pixel_transfer_discard_lx0();
    void pixel_transfer_discard_lx0_wx0_scx7();
    void pixel_transfer_lx0();
    void pixel_transfer_lx8();

    void hblank();
    void hblank_453();
    void hblank_454();
    void hblank_455();
    void hblank_last_line();
    void hblank_last_line_454();
    void hblank_last_line_455();
    void hblank_first_line_after_turn_on();

    void vblank();
    void vblank_454();
    void vblank_last_line();
    void vblank_last_line_2();
    void vblank_last_line_7();
    void vblank_last_line_454();

    // PPU states helpers
    void enter_oam_scan();
    void enter_pixel_transfer();
    void enter_hblank();
    void enter_vblank();
    void enter_new_frame();

    void tick_fetcher();
    void reset_fetcher();

    template <uint8_t mode>
    void update_mode();

    void update_stat_irq_for_oam_mode();

    void increase_lx();

    bool is_bg_fifo_ready_to_be_popped() const;
    bool is_obj_ready_to_be_fetched() const;

    void check_window_activation();
    void setup_fetcher_for_window();

    void handle_oam_scan_buses_oddities();

    // Fetcher states
    void bgwin_prefetcher_get_tile_0();

    void bg_prefetcher_get_tile_0();
    void bg_prefetcher_get_tile_1();
    void bg_pixel_slice_fetcher_get_tile_data_low_0();
    void bg_pixel_slice_fetcher_get_tile_data_low_1();
    void bg_pixel_slice_fetcher_get_tile_data_high_0();

    void win_prefetcher_activating();
    void win_prefetcher_get_tile_0();
    void win_prefetcher_get_tile_1();
    void win_pixel_slice_fetcher_get_tile_data_low_0();
    void win_pixel_slice_fetcher_get_tile_data_low_1();
    void win_pixel_slice_fetcher_get_tile_data_high_0();

    void bgwin_pixel_slice_fetcher_get_tile_data_high_1();
    void bgwin_pixel_slice_fetcher_push();

    void obj_prefetcher_get_tile_0();
    void obj_prefetcher_get_tile_1();
    void obj_pixel_slice_fetcher_get_tile_data_low_0();
    void obj_pixel_slice_fetcher_get_tile_data_low_1();
    void obj_pixel_slice_fetcher_get_tile_data_high_0();
    void obj_pixel_slice_fetcher_get_tile_data_high_1_and_merge_with_obj_fifo();

    // Fetcher states helpers
    void setup_obj_pixel_slice_fetcher_tile_data_address();
    void setup_bg_pixel_slice_fetcher_tilemap_tile_address();
    void setup_bg_pixel_slice_fetcher_tile_data_address();
    void setup_win_pixel_slice_fetcher_tilemap_tile_address();
    void setup_win_pixel_slice_fetcher_tile_data_address();

    void cache_bg_win_fetch();
    void restore_bg_win_fetch();

    static const TickSelector TICK_SELECTORS[];
    static const FetcherTickSelector FETCHER_TICK_SELECTORS[];

    Lcd& lcd;
    Interrupts& interrupts;
#ifdef ENABLE_CGB
    Hdma& hdma;
#endif
    VramBus::View<Device::Ppu> vram;
    OamBus::View<Device::Ppu> oam;
    Dma& dma_controller;

    TickSelector tick_selector {};
    FetcherTickSelector fetcher_tick_selector {&Ppu::bg_prefetcher_get_tile_0};

    bool last_stat_irq {};
    bool enable_lyc_eq_ly_irq {true};

    uint16_t dots {}; // [0, 456)
    uint8_t lx {};    // LX=X+8, therefore [0, 168)

    uint8_t last_bgp {}; // BGP delayed by 1 t-cycle
    uint8_t last_wx {};  // WX delayed by 1 t-cycle
    Lcdc last_lcdc {};   // LCDC delayed by 1 t-cycle

    FillQueue<BgPixel, 8> bg_fifo {};
    Queue<ObjPixel, 8> obj_fifo {};

    Vector<OamScanEntry, 10> oam_entries[168] {};
#ifdef ENABLE_ASSERTS
    uint8_t oam_entries_count {};
    uint8_t oam_entries_not_served_count {};
#endif
#ifdef ENABLE_DEBUGGER
    Vector<OamScanEntry, 10> scanline_oam_entries {}; // not cleared after oam scan
#endif

    bool is_fetching_sprite {};

    bool is_glitched_line_0 {};
    uint8_t glitched_line_0_hblank_delay {}; // [0, 2]

#ifdef ENABLE_DEBUGGER
    struct {
        uint16_t oam_scan {};
        uint16_t pixel_transfer {};
        uint16_t hblank {};
    } timings;
#endif

    struct {
        OamBus::Word oam;
    } registers;

    // Oam Scan
    struct {
        uint8_t count {}; // [0, 10]
    } oam_scan;

    // Pixel Transfer
    struct {
        struct {
            uint8_t to_discard {}; // [0, 8)
            uint8_t discarded {};  // [0, 8)
        } initial_scx;
    } pixel_transfer;

    // Window
    struct {
        bool active_for_frame {}; // window activated for the frame
        uint8_t wly {};           // window line counter

        bool active {};         // currently rendering window
        bool just_activated {}; // first fetch of this window streak

#if defined(ENABLE_DEBUGGER) || defined(ENABLE_ASSERTS)
        Vector<uint8_t, 20> line_triggers {};
#endif
    } w;

    // Bg/Window Prefetcher
    struct {
        uint8_t lx {}; // [0, 256), advances 8 by 8
        uint16_t tilemap_tile_vram_addr {};

#ifdef ENABLE_CGB
        bool attributes_enabled {};
        uint8_t attributes {};
#endif

#ifdef ENABLE_DEBUGGER
        uint8_t tilemap_x {};
        uint8_t tilemap_y {};
        uint8_t tilemap_vram_addr {};
#endif

        struct {
            bool has_data {};
            uint8_t tile_data_low {};
            uint8_t tile_data_high {};
        } interrupted_fetch {};
    } bwf;

    // Window Prefetcher
    struct {
        uint8_t tilemap_x {};
    } wf;

    // Obj Prefetcher
    struct {
        OamScanEntry entry {};  // hit obj
        uint8_t tile_number {}; // [0, 256)
        uint8_t attributes {};
    } of;

    // Pixel Slice Fetcher
    struct {
        uint16_t tile_data_vram_address {};
        uint8_t tile_data_low {};
        uint8_t tile_data_high {};
    } psf;

#ifdef ENABLE_CGB
    // CGB palettes
    uint8_t bg_palettes[64] {};
    uint8_t obj_palettes[64] {};
#endif

#ifdef ENABLE_DEBUGGER
    uint64_t cycles {};
#endif
};

#endif // PPU_H