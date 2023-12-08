#include "core.h"
#include "docboy/cartridge/factory.h"
#include "utils/math.h"
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

static constexpr uint16_t SERIAL_PERIOD = 8 /* bits */ * Specs::Frequencies::CLOCK / Specs::Frequencies::SERIAL;

// TODO: deduce SERIAL_TICKS_OFFSET also for bootrom version when sure about bootrom timing
static constexpr uint64_t SERIAL_PHASE_OFFSET = (SERIAL_PERIOD - 48); // deduced from boot_sclk_align-dmgABCmgb.gb

static constexpr uint32_t STATE_SAVE_SIZE_UNKNOWN = UINT32_MAX;
static uint32_t STATE_SAVE_SIZE = STATE_SAVE_SIZE_UNKNOWN;

Core::Core(GameBoy& gb) :
    gb(gb) {
}

inline void Core::tick_t0() {
    gb.cpu.tick();
    gb.ppu.tick();
    gb.cpu.checkInterrupt_t0();
    gb.mmu.tick_t0();
}

inline void Core::tick_t1() {
    gb.ppu.tick();
    gb.cpu.checkInterrupt_t1();
    gb.dma.tickWrite();
    gb.mmu.tick_t1();
}

inline void Core::tick_t2() {
    gb.ppu.tick();
    gb.cpu.checkInterrupt_t2();
    gb.mmu.tick_t2();
}

inline void Core::tick_t3() {
    if (mod<SERIAL_PERIOD>(ticks + SERIAL_PHASE_OFFSET) == 3)
        gb.serialPort.tick();
    gb.timers.tick();
    gb.dma.tickRead();
    gb.ppu.tick();
    gb.cpu.checkInterrupt_t3();
    gb.mmu.tick_t3();
}

void Core::cycle() {
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

void Core::frame() {
    byte& LY = gb.video.LY;
    if (LY >= 144) {
        while (LY != 0) {
            cycle();
            RETURN_IF_DEBUGGER_IS_ASKING_TO_SHUTDOWN();
        }
    }
    while (LY < 144) {
        cycle();
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

uint32_t Core::getStateSaveSize() const {
    if (STATE_SAVE_SIZE == STATE_SAVE_SIZE_UNKNOWN)
        parcelizeState();
    check(STATE_SAVE_SIZE != STATE_SAVE_SIZE_UNKNOWN);
    return STATE_SAVE_SIZE;
}

void Core::saveState(void* data) {
    memcpy(data, parcelizeState().getData(), getStateSaveSize());
}

void Core::loadState(const void* data) {
    unparcelizeState(Parcel {data, getStateSaveSize()});
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

    STATE_SAVE_SIZE = p.getSize();

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