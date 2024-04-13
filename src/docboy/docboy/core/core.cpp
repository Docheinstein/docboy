#include "core.h"
#include "docboy/cartridge/factory.h"
#include "utils/mathematics.h"
#include <iostream>

#ifdef ENABLE_DEBUGGER
#include "docboy/debugger/backend.h"

#define TICK_DEBUGGER(n)                                                                                               \
    if (debugger) {                                                                                                    \
        debugger->onTick(n);                                                                                           \
    }
#define RETURN_IF_DEBUGGER_IS_ASKING_TO_SHUTDOWN()                                                                     \
    if (debugger && debugger->isAskingToShutdown()) {                                                                  \
        return;                                                                                                        \
    }
#else
#define TICK_DEBUGGER(n) (void)(0)
#define RETURN_IF_DEBUGGER_IS_ASKING_TO_SHUTDOWN() (void)(0)
#endif

namespace {
constexpr uint16_t LCD_VBLANK_CYCLES = 16416;

constexpr uint16_t SERIAL_PERIOD = 8 /* bits */ * Specs::Frequencies::CLOCK / Specs::Frequencies::SERIAL;

// TODO: deduce SERIAL_TICKS_OFFSET also for bootrom version when sure about bootrom timing
constexpr uint64_t SERIAL_PHASE_OFFSET = SERIAL_PERIOD - 48; // deduced from mooneye/boot_sclk_align-dmgABCmgb.gb

constexpr uint32_t STATE_SAVE_SIZE_UNKNOWN = UINT32_MAX;

uint32_t stateSize = STATE_SAVE_SIZE_UNKNOWN;
} // namespace

Core::Core(GameBoy& gb) :
    gb(gb) {
}

inline void Core::tick_t0() const {
    gb.cpu.tick_t0();
    gb.ppu.tick();
}

inline void Core::tick_t1() const {
    gb.cpu.tick_t1();
    gb.ppu.tick();
    gb.dma.tick_t1();
}

inline void Core::tick_t2() const {
    gb.cpu.tick_t2();
    gb.ppu.tick();
}

inline void Core::tick_t3() const {
    gb.cpu.tick_t3();
    if (mod<SERIAL_PERIOD>(ticks + SERIAL_PHASE_OFFSET) == 3)
        gb.serialPort.tick();
    gb.timers.tick_t3();
    gb.ppu.tick();
    gb.dma.tick_t3();
    gb.stopController.tick();
    gb.cartridgeSlot.tick();
}

void Core::cycle_() {
    check(mod<4>(ticks) == 0);

    TICK_DEBUGGER(ticks);
    RETURN_IF_DEBUGGER_IS_ASKING_TO_SHUTDOWN();
    tick_t0();
    ++ticks;

    TICK_DEBUGGER(ticks);
    RETURN_IF_DEBUGGER_IS_ASKING_TO_SHUTDOWN();
    tick_t1();
    ++ticks;

    TICK_DEBUGGER(ticks);
    RETURN_IF_DEBUGGER_IS_ASKING_TO_SHUTDOWN();
    tick_t2();
    ++ticks;

    TICK_DEBUGGER(ticks);
    RETURN_IF_DEBUGGER_IS_ASKING_TO_SHUTDOWN();
    tick_t3();
    ++ticks;
}

void Core::cycle() {
    if (gb.stopped) {
        // Just handle joypad input in STOP mode: all the components are idle.
        TICK_DEBUGGER(ticks);
        gb.stopController.handleStopMode();
        return;
    }

    cycle_();
}

void Core::frame() {
    if (gb.stopped) {
        // Just handle joypad input in STOP mode: all the components are idle.
        TICK_DEBUGGER(ticks);
        gb.stopController.handleStopMode();
        return;
    }

    // If LCD is disabled we risk to never reach VBlank (which would stall the UI and prevent inputs).
    // Therefore, proceed for the cycles needed to render an entire frame and quit
    // if the LCD is still disabled even after that.
    if (!test_bit<Specs::Bits::Video::LCDC::LCD_ENABLE>(gb.video.LCDC)) {
        for (uint16_t c = 0; c < LCD_VBLANK_CYCLES; c++) {
            cycle_();
            check(keep_bits<2>(gb.video.STAT) != Specs::Ppu::Modes::VBLANK);

            if (gb.stopped)
                return;
            RETURN_IF_DEBUGGER_IS_ASKING_TO_SHUTDOWN();
        }

        // LCD still disabled: quit.
        if (!test_bit<Specs::Bits::Video::LCDC::LCD_ENABLE>(gb.video.LCDC))
            return;
    }

    check(test_bit<Specs::Bits::Video::LCDC::LCD_ENABLE>(gb.video.LCDC));

    // Eventually go out of current VBlank.
    while (keep_bits<2>(gb.video.STAT) == Specs::Ppu::Modes::VBLANK) {
        cycle_();
        if (!test_bit<Specs::Bits::Video::LCDC::LCD_ENABLE>(gb.video.LCDC) || gb.stopped)
            return;
        RETURN_IF_DEBUGGER_IS_ASKING_TO_SHUTDOWN();
    }

    check(keep_bits<2>(gb.video.STAT) != Specs::Ppu::Modes::VBLANK);

    // Proceed until next VBlank.
    while (keep_bits<2>(gb.video.STAT) != Specs::Ppu::Modes::VBLANK) {
        cycle_();
        if (!test_bit<Specs::Bits::Video::LCDC::LCD_ENABLE>(gb.video.LCDC) || gb.stopped)
            return;
        RETURN_IF_DEBUGGER_IS_ASKING_TO_SHUTDOWN();
    }
}

void Core::loadRom(const std::string& filename) const {
    loadRom(CartridgeFactory().create(filename));
}

void Core::loadRom(std::unique_ptr<ICartridge>&& cartridge) const {
    gb.cartridgeSlot.attach(std::move(cartridge));
}

void Core::attachSerialLink(SerialLink::Plug& plug) const {
    gb.serialPort.attach(plug);
}

bool Core::canSaveRam() const {
    return getRamSaveSize() > 0;
}

uint32_t Core::getRamSaveSize() const {
    return gb.cartridgeSlot.cartridge->getRamSaveSize();
}

void Core::saveRam(void* data) const {
    memcpy(data, gb.cartridgeSlot.cartridge->getRamSaveData(), getRamSaveSize());
}

void Core::loadRam(const void* data) const {
    memcpy(gb.cartridgeSlot.cartridge->getRamSaveData(), data, getRamSaveSize());
}

uint32_t Core::getStateSize() const {
    if (stateSize == STATE_SAVE_SIZE_UNKNOWN) {
        const Parcel p = parcelizeState();
        stateSize = p.getSize();
    }

    check(stateSize != STATE_SAVE_SIZE_UNKNOWN);
    return stateSize;
}

void Core::saveState(void* data) const {
    memcpy(data, parcelizeState().getData(), getStateSize());
}

void Core::loadState(const void* data) {
    unparcelizeState(Parcel {data, getStateSize()});
}

Parcel Core::parcelizeState() const {
    Parcel p;

    p.writeUInt64(ticks);

    gb.cpu.saveState(p);
    gb.ppu.saveState(p);
    gb.cartridgeSlot.cartridge->saveState(p);
    gb.vram.saveState(p);
    gb.wram1.saveState(p);
    gb.wram2.saveState(p);
    gb.oam.saveState(p);
    gb.hram.saveState(p);
    gb.vramBus.saveState(p);
    gb.oamBus.saveState(p);
    gb.boot.saveState(p);
    gb.serialPort.saveState(p);
    gb.timers.saveState(p);
    gb.interrupts.saveState(p);
    gb.sound.saveState(p);
    gb.video.saveState(p);
    gb.lcd.saveState(p);
    gb.dma.saveState(p);
    gb.mmu.saveState(p);
    gb.stopController.saveState(p);

    return p;
}

void Core::unparcelizeState(Parcel&& p) {
    ticks = p.readUInt64();

    gb.cpu.loadState(p);
    gb.ppu.loadState(p);
    gb.cartridgeSlot.cartridge->loadState(p);
    gb.vram.loadState(p);
    gb.wram1.loadState(p);
    gb.wram2.loadState(p);
    gb.oam.loadState(p);
    gb.hram.loadState(p);
    gb.vramBus.loadState(p);
    gb.oamBus.loadState(p);
    gb.boot.loadState(p);
    gb.serialPort.loadState(p);
    gb.timers.loadState(p);
    gb.interrupts.loadState(p);
    gb.sound.loadState(p);
    gb.video.loadState(p);
    gb.lcd.loadState(p);
    gb.dma.loadState(p);
    gb.mmu.loadState(p);
    gb.stopController.loadState(p);

    check(p.getRemainingSize() == 0);
}

void Core::saveState(Parcel& parcel) const {
    parcel.writeUInt64(ticks);
}

void Core::loadState(Parcel& parcel) {
    ticks = parcel.readUInt64();
}

#ifdef ENABLE_DEBUGGER
void Core::attachDebugger(DebuggerBackend& dbg) {
    debugger = &dbg;
}

void Core::detachDebugger() {
    debugger = nullptr;
}

bool Core::isDebuggerAskingToShutdown() const {
    return debugger != nullptr && debugger->isAskingToShutdown();
}
#endif