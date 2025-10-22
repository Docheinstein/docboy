#include <iostream>

#include "docboy/core/core.h"

#include "docboy/cartridge/factory.h"
#include "docboy/common/randomdata.h"
#include "docboy/memory/vram0.h"

#ifdef ENABLE_BOOTROM
#include "docboy/bootrom/factory.h"
#endif

#ifdef ENABLE_DEBUGGER
#include "docboy/debugger/backend.h"

#define TICK_DEBUGGER(n)                                                                                               \
    if (debugger) {                                                                                                    \
        debugger->notify_tick(n);                                                                                      \
    }
#define RETURN_FROM_CYCLE_ON_QUIT_OR_RESET_REQUEST()                                                                   \
    if (is_asking_to_quit() || resetting) {                                                                            \
        return;                                                                                                        \
    }
#define RETURN_ON_QUIT_OR_RESET_REQUEST(ret)                                                                           \
    if (is_asking_to_quit() || resetting) {                                                                            \
        resetting = false;                                                                                             \
        return ret;                                                                                                    \
    }

#else
#define TICK_DEBUGGER(n) (void)(0)
#define RETURN_FROM_CYCLE_ON_QUIT_OR_RESET_REQUEST() (void)(0)
#define RETURN_ON_QUIT_OR_RESET_REQUEST(ret) (void)(0)
#endif

namespace {
constexpr uint16_t ARTIFICIAL_VBLANK_CYCLES = 16416;
constexpr uint32_t STATE_SAVE_SIZE_UNKNOWN = UINT32_MAX;
constexpr uint32_t STATE_WATERMARK = 0xABCDDCBA;

uint32_t rom_state_size = STATE_SAVE_SIZE_UNKNOWN;
} // namespace

Core::Core(GameBoy& gb) :
    gb {gb} {
}

inline void Core::tick_t0() const {
    if (gb.stop_controller.stopped) {
        gb.stop_controller.stopped_tick();
        return;
    }

#ifdef ENABLE_CGB
    if (!gb.hdma.is_active()) {
        gb.cpu.tick_t0();
    }
#else
    gb.cpu.tick_t0();
#endif

#ifdef ENABLE_CGB
    if (!gb.speed_switch_controller.is_blocking_ppu()) {
        gb.ppu.tick();
    }
#else
    gb.ppu.tick();
#endif

#ifdef ENABLE_CGB
    gb.hdma.tick_t0();
#endif
#ifdef ENABLE_CGB
    if (gb.speed_switch_controller.is_double_speed_mode()) {
        if (!gb.speed_switch_controller.is_blocking_dma()) {
            gb.dma.tick();
        }
    }
#endif
    gb.apu.tick_t0();
    gb.ext_bus.tick();
    gb.cpu_bus.tick();
    gb.oam_bus.tick();
    gb.vram_bus.tick();
}

inline void Core::tick_t1() const {
    if (gb.stop_controller.stopped) {
        gb.stop_controller.stopped_tick();
        return;
    }

#ifdef ENABLE_CGB
    if (!gb.hdma.is_active()) {
        gb.cpu.tick_t1();
    }
#else
    gb.cpu.tick_t1();
#endif

#ifdef ENABLE_CGB
    gb.speed_switch_controller.tick();

    if (gb.speed_switch_controller.is_double_speed_mode()) {
        if (!gb.speed_switch_controller.is_blocking_timers() /* TODO: ok? */) {
            gb.timers.tick();
        }
        gb.serial.tick();
    }
#endif

#ifdef ENABLE_CGB
    if (!gb.speed_switch_controller.is_blocking_ppu()) {
        gb.ppu.tick();
    }
#else
    gb.ppu.tick();
#endif

#ifdef ENABLE_CGB
    gb.hdma.tick_t1();
#endif
#ifdef ENABLE_CGB
    if (!gb.speed_switch_controller.is_blocking_dma()) {
        gb.dma.tick();
    }
#else
    gb.dma.tick();
#endif
    gb.apu.tick_t1();
    gb.ext_bus.tick();
    gb.cpu_bus.tick();
    gb.oam_bus.tick();
    gb.vram_bus.tick();

#ifdef ENABLE_CGB
    if (gb.speed_switch_controller.is_double_speed_mode()) {
        gb.stop_controller.tick();
    }
#endif
}

inline void Core::tick_t2() const {
    if (gb.stop_controller.stopped) {
        gb.stop_controller.stopped_tick();
        return;
    }

#ifdef ENABLE_CGB
    if (!gb.hdma.is_active()) {
        gb.cpu.tick_t2();
    }
#else
    gb.cpu.tick_t2();
#endif

#ifdef ENABLE_CGB
    if (!gb.speed_switch_controller.is_blocking_ppu()) {
        gb.ppu.tick();
    }
#else
    gb.ppu.tick();
#endif

#ifdef ENABLE_CGB
    gb.hdma.tick_t2();
#endif
#ifdef ENABLE_CGB
    if (gb.speed_switch_controller.is_double_speed_mode()) {
        if (!gb.speed_switch_controller.is_blocking_dma()) {
            gb.dma.tick();
        }
    }
#endif
    gb.apu.tick_t2();
    gb.ext_bus.tick();
    gb.cpu_bus.tick();
    gb.oam_bus.tick_t2();
    gb.vram_bus.tick();
}

inline void Core::tick_t3() const {
    if (gb.stop_controller.stopped) {
        gb.stop_controller.stopped_tock();
        return;
    }

#ifdef ENABLE_CGB
    if (!gb.hdma.is_active()) {
        gb.cpu.tick_t3();
    }
#else
    gb.cpu.tick_t3();
#endif

#ifdef ENABLE_CGB
    gb.speed_switch_controller.tick();
#endif

#ifdef ENABLE_CGB
    if (!gb.speed_switch_controller.is_blocking_timers()) {
        gb.timers.tick();
    }
#else
    gb.timers.tick();
#endif

    gb.serial.tick();

#ifdef ENABLE_CGB
    if (!gb.speed_switch_controller.is_blocking_ppu()) {
        gb.ppu.tick();
    }
#else
    gb.ppu.tick();
#endif

    // TODO: why HDMA/DMA reversed here?
#ifdef ENABLE_CGB
    gb.hdma.tick_t3();
#endif
#ifdef ENABLE_CGB
    if (!gb.speed_switch_controller.is_blocking_dma()) {
        gb.dma.tick();
    }
#else
    gb.dma.tick();
#endif
    gb.interrupts.tick_t3();
    gb.cartridge_slot.tick();
    gb.apu.tick_t3();
    gb.ext_bus.tick();
    gb.cpu_bus.tick();
    gb.oam_bus.tick();
    gb.vram_bus.tick();
    gb.stop_controller.tick();
}

void Core::cycle() {
    ASSERT(mod<4>(ticks) == 0);

    TICK_DEBUGGER(ticks);
    RETURN_FROM_CYCLE_ON_QUIT_OR_RESET_REQUEST();
    tick_t0();
    ++ticks;

    TICK_DEBUGGER(ticks);
    RETURN_FROM_CYCLE_ON_QUIT_OR_RESET_REQUEST();
    tick_t1();
    ++ticks;

    TICK_DEBUGGER(ticks);
    RETURN_FROM_CYCLE_ON_QUIT_OR_RESET_REQUEST();
    tick_t2();
    ++ticks;

    TICK_DEBUGGER(ticks);
    RETURN_FROM_CYCLE_ON_QUIT_OR_RESET_REQUEST();
    tick_t3();
    ++ticks;
}

void Core::frame() {
    if (gb.stop_controller.stopped) {
        // Artificially run for a reasonable amount of cycles while stopped
        for (uint16_t c = 0; c < ARTIFICIAL_VBLANK_CYCLES; c++) {
            cycle();

            if (!gb.stop_controller.stopped) {
                return;
            }

            RETURN_ON_QUIT_OR_RESET_REQUEST();
        }

        // Still stopped: quit.
        if (gb.stop_controller.stopped) {
            return;
        }
    }

    ASSERT(!gb.stop_controller.stopped);

    // If LCD is disabled we risk to never reach VBlank (which would stall the UI and prevent inputs).
    // Therefore, proceed for the cycles needed to render an entire frame.
    // Quit if the LCD is still disabled even after that.
    if (!gb.ppu.lcdc.enable) {
        for (uint16_t c = 0; c < ARTIFICIAL_VBLANK_CYCLES; c++) {
            cycle();

            if (gb.stop_controller.stopped) {
                return;
            }

            RETURN_ON_QUIT_OR_RESET_REQUEST();
            ASSERT(gb.ppu.stat.mode != Specs::Ppu::Modes::VBLANK);
        }

        // LCD still disabled: quit.
        if (!gb.ppu.lcdc.enable) {
            return;
        }
    }

    ASSERT(gb.ppu.lcdc.enable);

    // Eventually go out of current VBlank.
    while (gb.ppu.stat.mode == Specs::Ppu::Modes::VBLANK) {
        cycle();

        if (!gb.ppu.lcdc.enable || gb.stop_controller.stopped) {
            return;
        }

        RETURN_ON_QUIT_OR_RESET_REQUEST();
    }

    ASSERT(gb.ppu.stat.mode != Specs::Ppu::Modes::VBLANK);

    // Proceed until next VBlank.
    while (gb.ppu.stat.mode != Specs::Ppu::Modes::VBLANK) {
        cycle();

        if (!gb.ppu.lcdc.enable || gb.stop_controller.stopped) {
            return;
        }

        RETURN_ON_QUIT_OR_RESET_REQUEST();
    }
}

bool Core::run_for_cycles(uint32_t cycles_to_run) {
    uint8_t cycle_count {};

    if (gb.stop_controller.stopped) {
        // Artificially run for a reasonable amount of cycles while stopped
        for (uint16_t c = cycles_of_artificial_vblank; c < ARTIFICIAL_VBLANK_CYCLES; c++) {
            if (++cycle_count > cycles_to_run) {
                // No more cycles to run for this burst.
                cycles_of_artificial_vblank = c;
                return false;
            }

            cycle();

            if (!gb.stop_controller.stopped) {
                cycles_of_artificial_vblank = 0;
                return true;
            }

            RETURN_ON_QUIT_OR_RESET_REQUEST(true);
        }

        // Still stopped: quit.
        if (gb.stop_controller.stopped) {
            cycles_of_artificial_vblank = 0;
            return true;
        }
    }

    ASSERT(!gb.stop_controller.stopped);

    // If LCD is disabled we risk to never reach VBlank (which would stall the UI and prevent inputs).
    // Therefore, proceed for the cycles needed to render an entire frame and quit
    // if the LCD is still disabled even after that.
    if (!gb.ppu.lcdc.enable) {
        for (uint16_t c = cycles_of_artificial_vblank; c < ARTIFICIAL_VBLANK_CYCLES; c++) {
            if (++cycle_count > cycles_to_run) {
                // No more cycles to run for this burst.
                cycles_of_artificial_vblank = c;
                return false;
            }

            cycle();

            if (gb.stop_controller.stopped) {
                cycles_of_artificial_vblank = 0;
                return true;
            }

            RETURN_ON_QUIT_OR_RESET_REQUEST(true);
            ASSERT(gb.ppu.stat.mode != Specs::Ppu::Modes::VBLANK);
        }

        // LCD still disabled: quit.
        if (!gb.ppu.lcdc.enable) {
            cycles_of_artificial_vblank = 0;
            return true;
        }
    }

    ASSERT(gb.ppu.lcdc.enable);

    // Eventually go out of current VBlank.
    while (gb.ppu.stat.mode == Specs::Ppu::Modes::VBLANK) {
        if (++cycle_count > cycles_to_run) {
            // No more cycles to run for this burst.
            return false;
        }

        cycle();

        if (!gb.ppu.lcdc.enable || gb.stop_controller.stopped) {
            return true;
        }

        RETURN_ON_QUIT_OR_RESET_REQUEST(true);
    }

    ASSERT(gb.ppu.stat.mode != Specs::Ppu::Modes::VBLANK);

    // Proceed until next VBlank.
    while (gb.ppu.stat.mode != Specs::Ppu::Modes::VBLANK) {
        if (++cycle_count > cycles_to_run) {
            // No more cycles to run for this burst.
            return false;
        }

        cycle();

        if (!gb.ppu.lcdc.enable || gb.stop_controller.stopped) {
            return true;
        }

        RETURN_ON_QUIT_OR_RESET_REQUEST(true);
    }

    // Frame completed.
    return true;
}

#ifdef ENABLE_BOOTROM
void Core::load_boot_rom(const std::string& filename) {
    BootRomFactory::load(gb.boot_rom, filename);
}
#endif

void Core::load_rom(const std::string& filename) {
    gb.cartridge_slot.attach(CartridgeFactory::create(filename));
    reset();
}

void Core::attach_serial_link(ISerialEndpoint& endpoint) const {
    gb.serial.attach(endpoint);
}

void Core::detach_serial_link() const {
    gb.serial.detach();
}

void Core::save(void* data) const {
    memcpy(data, gb.cartridge_slot.cartridge->get_ram_save_data(), gb.cartridge_slot.cartridge->get_ram_save_size());
    memcpy(static_cast<uint8_t*>(data) + gb.cartridge_slot.cartridge->get_ram_save_size(),
           gb.cartridge_slot.cartridge->get_rtc_save_data(), gb.cartridge_slot.cartridge->get_rtc_save_size());
}

void Core::load(const void* data) const {
    memcpy(gb.cartridge_slot.cartridge->get_ram_save_data(), data, gb.cartridge_slot.cartridge->get_ram_save_size());
    memcpy(gb.cartridge_slot.cartridge->get_rtc_save_data(),
           static_cast<const uint8_t*>(data) + gb.cartridge_slot.cartridge->get_ram_save_size(),
           gb.cartridge_slot.cartridge->get_rtc_save_size());
}

bool Core::can_save() const {
    return get_save_size() > 0;
}

uint32_t Core::get_save_size() const {
    return gb.cartridge_slot.cartridge->get_ram_save_size() + gb.cartridge_slot.cartridge->get_rtc_save_size();
}

void Core::save_state(void* data) const {
    memcpy(data, parcelize_state().get_data(), get_state_size());
}

void Core::load_state(const void* data) {
    unparcelize_state(Parcel {data, get_state_size()});
}

uint32_t Core::get_state_size() const {
    if (rom_state_size == STATE_SAVE_SIZE_UNKNOWN) {
        const Parcel p = parcelize_state();
        rom_state_size = p.get_size();
    }

    ASSERT(rom_state_size != STATE_SAVE_SIZE_UNKNOWN);
    return rom_state_size;
}

Parcel Core::parcelize_state() const {
    Parcel p;

    PARCEL_WRITE_UINT32(p, STATE_WATERMARK);
    PARCEL_WRITE_UINT64(p, ticks);

    gb.cpu.save_state(p);
    gb.ppu.save_state(p);
    gb.lcd.save_state(p);
    gb.dma.save_state(p);
    gb.apu.save_state(p);
    gb.stop_controller.save_state(p);
    gb.cartridge_slot.cartridge->save_state(p);
    for (std::size_t i = 0; i < array_size(gb.vram); i++) {
        gb.vram[i].save_state(p);
    }
    gb.wram1.save_state(p);
    for (std::size_t i = 0; i < array_size(gb.wram2); i++) {
        gb.wram2[i].save_state(p);
    }
    gb.oam.save_state(p);
#ifdef ENABLE_CGB
    gb.not_usable.save_state(p);
#endif
    gb.hram.save_state(p);
    gb.boot.save_state(p);
    gb.serial.save_state(p);
    gb.timers.save_state(p);
    gb.interrupts.save_state(p);
    gb.ext_bus.save_state(p);
#ifdef ENABLE_CGB
    gb.wram_bus.save_state(p);
#endif
    gb.cpu_bus.save_state(p);
    gb.vram_bus.save_state(p);
    gb.oam_bus.save_state(p);
    gb.mmu.save_state(p);

#ifdef ENABLE_CGB
    gb.vram_bank_controller.save_state(p);
    gb.wram_bank_controller.save_state(p);
    gb.hdma.save_state(p);
    gb.speed_switch.save_state(p);
    gb.speed_switch_controller.save_state(p);
    gb.infrared.save_state(p);
    gb.undocumented_registers.save_state(p);
#endif

    return p;
}

void Core::unparcelize_state(Parcel&& p) {
    [[maybe_unused]] const uint32_t watermark = p.read_uint32();
    ASSERT(watermark == STATE_WATERMARK);

    ticks = p.read_uint64();

    gb.cpu.load_state(p);
    gb.ppu.load_state(p);
    gb.lcd.load_state(p);
    gb.dma.load_state(p);
    gb.apu.load_state(p);
    gb.stop_controller.load_state(p);
    gb.cartridge_slot.cartridge->load_state(p);
    for (std::size_t i = 0; i < array_size(gb.vram); i++) {
        gb.vram[i].load_state(p);
    }
    gb.wram1.load_state(p);
    for (std::size_t i = 0; i < array_size(gb.wram2); i++) {
        gb.wram2[i].load_state(p);
    }
    gb.oam.load_state(p);
#ifdef ENABLE_CGB
    gb.not_usable.load_state(p);
#endif
    gb.hram.load_state(p);
    gb.boot.load_state(p);
    gb.serial.load_state(p);
    gb.timers.load_state(p);
    gb.interrupts.load_state(p);
    gb.ext_bus.load_state(p);
#ifdef ENABLE_CGB
    gb.wram_bus.load_state(p);
#endif
    gb.cpu_bus.load_state(p);
    gb.vram_bus.load_state(p);
    gb.oam_bus.load_state(p);
    gb.mmu.load_state(p);

#ifdef ENABLE_CGB
    gb.vram_bank_controller.load_state(p);
    gb.wram_bank_controller.load_state(p);
    gb.hdma.load_state(p);
    gb.speed_switch.load_state(p);
    gb.speed_switch_controller.load_state(p);
    gb.infrared.load_state(p);
    gb.undocumented_registers.load_state(p);
#endif

    ASSERT(p.get_remaining_size() == 0);
}

void Core::reset() {
    rom_state_size = STATE_SAVE_SIZE_UNKNOWN;

    ticks = 0;

    gb.cpu.reset();
    gb.ppu.reset();
    gb.lcd.reset();
    gb.dma.reset();
    gb.apu.reset();
    gb.stop_controller.reset();
    gb.cartridge_slot.cartridge->reset();
    gb.vram[0].reset(VRAM0_INITIAL_DATA);
#ifdef ENABLE_CGB
    gb.vram[1].reset();
#endif
    gb.wram1.reset(RANDOM_DATA);
    for (std::size_t i = 0; i < array_size(gb.wram2); i++) {
        gb.wram2[i].reset(RANDOM_DATA);
    }
#ifdef ENABLE_CGB
    gb.oam.reset();
#else
    gb.oam.reset(RANDOM_DATA);
#endif
#ifdef ENABLE_CGB
    gb.not_usable.reset();
#endif
    gb.hram.reset(HRAM_INITIAL_DATA);
    gb.boot.reset();
    gb.timers.reset();
    gb.serial.reset();
    gb.interrupts.reset();
    gb.ext_bus.reset();
#ifdef ENABLE_CGB
    gb.wram_bus.reset();
#endif
    gb.cpu_bus.reset();
    gb.vram_bus.reset();
    gb.oam_bus.reset();
    gb.mmu.reset();

#ifdef ENABLE_CGB
    gb.vram_bank_controller.reset();
    gb.wram_bank_controller.reset();
    gb.hdma.reset();
    gb.speed_switch.reset();
    gb.speed_switch_controller.reset();
    gb.infrared.reset();
    gb.undocumented_registers.reset();
#endif
}

#ifdef ENABLE_AUDIO
void Core::set_audio_sample_callback(std::function<void(Apu::AudioSample)>&& audio_callback) const {
    gb.apu.set_audio_sample_callback(std::move(audio_callback));
}

void Core::set_audio_volume(double volume) const {
    gb.apu.set_volume(volume);
}

void Core::set_audio_high_pass_filter_enabled(bool enabled) const {
    gb.apu.set_high_pass_filter_enabled(enabled);
}
#endif

void Core::set_rumble_callback(std::function<void(bool)>&& rumble_callback) const {
    gb.cartridge_slot.cartridge->set_rumble_callback(std::move(rumble_callback));
}

#ifdef ENABLE_DEBUGGER
void Core::attach_debugger(DebuggerBackend& dbg) {
    debugger = &dbg;
    debugger->set_on_reset_callback([this] {
        resetting = true;
    });
}

void Core::detach_debugger() {
    debugger = nullptr;
}

bool Core::is_asking_to_quit() const {
    return debugger && debugger->is_asking_to_quit();
}
#endif