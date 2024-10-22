#include <iostream>

#include "docboy/core/core.h"

#include "docboy/cartridge/factory.h"
#include "docboy/common/randomdata.h"
#include "docboy/memory/vram0.h"

#ifdef ENABLE_DEBUGGER
#include "docboy/debugger/backend.h"

#define TICK_DEBUGGER(n)                                                                                               \
    if (debugger) {                                                                                                    \
        debugger->notify_tick(n);                                                                                      \
    }
#define RETURN_ON_QUIT_REQUEST()                                                                                       \
    if (debugger && debugger->is_asking_to_quit()) {                                                                   \
        return;                                                                                                        \
    }
#else
#define TICK_DEBUGGER(n) (void)(0)
#define RETURN_ON_QUIT_REQUEST() (void)(0)
#endif

namespace {
constexpr uint16_t LCD_VBLANK_CYCLES = 16416;
constexpr uint16_t SERIAL_PERIOD = 8 /* bits */ * Specs::Frequencies::CLOCK / Specs::Frequencies::SERIAL;
constexpr uint64_t SERIAL_PHASE_OFFSET = if_bootrom_else(1036, 4048); // [mooneye/boot_sclk_align-dmgABCmgb.gb]
constexpr uint32_t STATE_SAVE_SIZE_UNKNOWN = UINT32_MAX;

uint32_t rom_state_size = STATE_SAVE_SIZE_UNKNOWN;
} // namespace

Core::Core(GameBoy& gb) :
    gb {gb} {
}

inline void Core::tick_t0() const {
#ifdef ENABLE_CGB
    if (!gb.hdma.is_active()) {
        gb.cpu.tick_t0();
    }
#else
    gb.cpu.tick_t0();
#endif
    gb.ppu.tick();
#ifdef ENABLE_CGB
    gb.hdma.tick_t0();
#endif
    gb.apu.tick_t0();
}

inline void Core::tick_t1() const {
#ifdef ENABLE_CGB
    if (!gb.hdma.is_active()) {
        gb.cpu.tick_t1();
    }
#else
    gb.cpu.tick_t1();
#endif

    gb.ppu.tick();
    gb.dma.tick_t1();
#ifdef ENABLE_CGB
    gb.hdma.tick_t1();
#endif
    gb.apu.tick_t1();
}

inline void Core::tick_t2() const {
#ifdef ENABLE_CGB
    if (!gb.hdma.is_active()) {
        gb.cpu.tick_t2();
    }
#else
    gb.cpu.tick_t2();
#endif

    gb.ppu.tick();
#ifdef ENABLE_CGB
    gb.hdma.tick_t2();
#endif
    gb.apu.tick_t2();
}

inline void Core::tick_t3() const {
#ifdef ENABLE_CGB
    if (!gb.hdma.is_active()) {
        gb.cpu.tick_t3();
    }
#else
    gb.cpu.tick_t3();
#endif

    if (mod<SERIAL_PERIOD>(ticks + SERIAL_PHASE_OFFSET) == 3) {
        gb.serial_port.tick();
    }
    gb.timers.tick_t3();
    gb.ppu.tick();
#ifdef ENABLE_CGB
    gb.hdma.tick_t3();
#endif
    gb.dma.tick_t3();
    gb.stop_controller.tick();
    gb.cartridge_slot.tick();
    gb.apu.tick_t3();
}

void Core::_cycle() {
    ASSERT(mod<4>(ticks) == 0);

    TICK_DEBUGGER(ticks);
    RETURN_ON_QUIT_REQUEST();
    tick_t0();
    ++ticks;

    TICK_DEBUGGER(ticks);
    RETURN_ON_QUIT_REQUEST();
    tick_t1();
    ++ticks;

    TICK_DEBUGGER(ticks);
    RETURN_ON_QUIT_REQUEST();
    tick_t2();
    ++ticks;

    TICK_DEBUGGER(ticks);
    RETURN_ON_QUIT_REQUEST();
    tick_t3();
    ++ticks;
}

void Core::cycle() {
    if (gb.stopped) {
        // Just handle joypad input in STOP mode: all the components are idle.
        TICK_DEBUGGER(ticks);
        gb.stop_controller.handle_stop_mode();
        return;
    }

    _cycle();
}

void Core::frame() {
    if (gb.stopped) {
        // Just handle joypad input in STOP mode: all the components are idle.
        TICK_DEBUGGER(ticks);
        gb.stop_controller.handle_stop_mode();
        return;
    }

    // If LCD is disabled we risk to never reach VBlank (which would stall the UI and prevent inputs).
    // Therefore, proceed for the cycles needed to render an entire frame and quit
    // if the LCD is still disabled even after that.
    if (!gb.ppu.lcdc.enable) {
        for (uint16_t c = 0; c < LCD_VBLANK_CYCLES; c++) {
            _cycle();
            ASSERT(gb.ppu.stat.mode != Specs::Ppu::Modes::VBLANK);

            if (gb.stopped) {
                return;
            }
            RETURN_ON_QUIT_REQUEST();
        }

        // LCD still disabled: quit.
        if (!gb.ppu.lcdc.enable) {
            return;
        }
    }

    ASSERT(gb.ppu.lcdc.enable);

    // Eventually go out of current VBlank.
    while (gb.ppu.stat.mode == Specs::Ppu::Modes::VBLANK) {
        _cycle();
        if (!gb.ppu.lcdc.enable || gb.stopped) {
            return;
        }
        RETURN_ON_QUIT_REQUEST();
    }

    ASSERT(gb.ppu.stat.mode != Specs::Ppu::Modes::VBLANK);

    // Proceed until next VBlank.
    while (gb.ppu.stat.mode != Specs::Ppu::Modes::VBLANK) {
        _cycle();
        if (!gb.ppu.lcdc.enable || gb.stopped)
            return;
        RETURN_ON_QUIT_REQUEST();
    }
}

void Core::load_rom(const std::string& filename) {
    gb.cartridge_slot.attach(CartridgeFactory::create(filename));
    reset();
}

void Core::attach_serial_link(SerialLink::Plug& plug) const {
    gb.serial_port.attach(plug);
}

bool Core::can_save_ram() const {
    return get_ram_save_size() > 0;
}

uint32_t Core::get_ram_save_size() const {
    return gb.cartridge_slot.cartridge->get_ram_save_size();
}

void Core::save_ram(void* data) const {
    memcpy(data, gb.cartridge_slot.cartridge->get_ram_save_data(), get_ram_save_size());
}

void Core::load_ram(const void* data) const {
    memcpy(gb.cartridge_slot.cartridge->get_ram_save_data(), data, get_ram_save_size());
}

uint32_t Core::get_state_size() const {
    if (rom_state_size == STATE_SAVE_SIZE_UNKNOWN) {
        const Parcel p = parcelize_state();
        rom_state_size = p.get_size();
    }

    ASSERT(rom_state_size != STATE_SAVE_SIZE_UNKNOWN);
    return rom_state_size;
}

void Core::save_state(void* data) const {
    memcpy(data, parcelize_state().get_data(), get_state_size());
}

void Core::load_state(const void* data) {
    unparcelize_state(Parcel {data, get_state_size()});
}

Parcel Core::parcelize_state() const {
    Parcel p;

    p.write_uint64(ticks);

    gb.cpu.save_state(p);
    gb.ppu.save_state(p);
    gb.apu.save_state(p);
    gb.cartridge_slot.cartridge->save_state(p);
    for (std::size_t i = 0; i < array_size(gb.vram); i++) {
        gb.vram[i].save_state(p);
    }
    gb.wram1.save_state(p);
    for (std::size_t i = 0; i < array_size(gb.wram2); i++) {
        gb.wram2[i].save_state(p);
    }
    gb.oam.save_state(p);
    gb.hram.save_state(p);
    gb.ext_bus.save_state(p);
    gb.cpu_bus.save_state(p);
    gb.vram_bus.save_state(p);
    gb.oam_bus.save_state(p);
    gb.boot.save_state(p);
    gb.serial_port.save_state(p);
    gb.timers.save_state(p);
    gb.interrupts.save_state(p);
    gb.lcd.save_state(p);
    gb.dma.save_state(p);
    gb.mmu.save_state(p);
    gb.stop_controller.save_state(p);

#ifdef ENABLE_CGB
    gb.vram_bank_controller.save_state(p);
    gb.wram_bank_controller.save_state(p);

    gb.infrared.save_state(p);
    gb.undocumented_registers.save_state(p);
#endif

    return p;
}

void Core::unparcelize_state(Parcel&& parcel) {
    ticks = parcel.read_uint64();

    gb.cpu.load_state(parcel);
    gb.ppu.load_state(parcel);
    gb.apu.load_state(parcel);
    gb.cartridge_slot.cartridge->load_state(parcel);
    for (std::size_t i = 0; i < array_size(gb.vram); i++) {
        gb.vram[i].load_state(parcel);
    }
    gb.wram1.load_state(parcel);
    for (std::size_t i = 0; i < array_size(gb.wram2); i++) {
        gb.wram2[i].load_state(parcel);
    }
    gb.oam.load_state(parcel);
    gb.hram.load_state(parcel);
    gb.ext_bus.load_state(parcel);
    gb.cpu_bus.load_state(parcel);
    gb.vram_bus.load_state(parcel);
    gb.oam_bus.load_state(parcel);
    gb.boot.load_state(parcel);
    gb.serial_port.load_state(parcel);
    gb.timers.load_state(parcel);
    gb.interrupts.load_state(parcel);
    gb.lcd.load_state(parcel);
    gb.dma.load_state(parcel);
    gb.mmu.load_state(parcel);
    gb.stop_controller.load_state(parcel);

#ifdef ENABLE_CGB
    gb.vram_bank_controller.load_state(parcel);
    gb.wram_bank_controller.load_state(parcel);

    gb.infrared.load_state(parcel);
    gb.undocumented_registers.load_state(parcel);
#endif

    ASSERT(parcel.get_remaining_size() == 0);
}

void Core::reset() {
    rom_state_size = STATE_SAVE_SIZE_UNKNOWN;
    ticks = 0;

    gb.cpu.reset();
    gb.ppu.reset();
    gb.apu.reset();
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
    gb.hram.reset(RANDOM_DATA);
    gb.ext_bus.reset();
    gb.cpu_bus.reset();
    gb.vram_bus.reset();
    gb.oam_bus.reset();
    gb.boot.reset();
    gb.serial_port.reset();
    gb.timers.reset();
    gb.interrupts.reset();
    gb.lcd.reset();
    gb.dma.reset();
    gb.mmu.reset();
    gb.stop_controller.reset();

#ifdef ENABLE_CGB
    gb.vram_bank_controller.reset();
    gb.wram_bank_controller.reset();

    gb.infrared.reset();
    gb.undocumented_registers.reset();
#endif
}

#ifdef ENABLE_AUDIO
void Core::set_audio_sample_callback(std::function<void(const Apu::AudioSample)>&& audio_callback) const {
    gb.apu.set_audio_sample_callback(std::move(audio_callback));
}
#endif

#ifdef ENABLE_DEBUGGER
void Core::attach_debugger(DebuggerBackend& dbg) {
    debugger = &dbg;
}

void Core::detach_debugger() {
    debugger = nullptr;
}

bool Core::is_asking_to_quit() const {
    return debugger && debugger->is_asking_to_quit();
}
#endif