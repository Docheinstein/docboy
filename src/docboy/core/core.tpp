#include "core.h"
#include "utils/math.h"

#ifdef ENABLE_DEBUGGER
#include "docboy/debugger/backend.h"

#define TICK_DEBUGGER(n) if (debugger) debugger->onTick(n)
#define RETURN_IF_DEBUGGER_IS_ASKING_TO_SHUTDOWN() if (debugger && debugger->isAskingToShutdown()) return
#else
#define TICK_DEBUGGER(n) (void)(0)
#define RETURN_IF_DEBUGGER_IS_ASKING_TO_SHUTDOWN() (void)(0)
#endif


static constexpr uint8_t CPU_PERIOD = Specs::Frequencies::CLOCK / Specs::Frequencies::CPU;
static constexpr uint16_t SERIAL_PERIOD = 8 /* bits */ * Specs::Frequencies::CLOCK / Specs::Frequencies::SERIAL;

// TODO: deduce SERIAL_TICKS_OFFSET also for bootrom version when sure about bootrom timing
static constexpr uint64_t SERIAL_TICKS_OFFSET = (SERIAL_PERIOD - 48); // deduced from boot_sclk_align-dmgABCmgb.gb

inline
void Core::tick() {
    TICK_DEBUGGER(ticks);
    RETURN_IF_DEBUGGER_IS_ASKING_TO_SHUTDOWN();

    if (mod<CPU_PERIOD>(ticks) == 0) {
        // CPU
        gb.cpu.tick();

        // Timers
        gb.timers.tick();

        // DMA
        gb.dma.tick();

        // Serial
        if (mod<SERIAL_PERIOD>(ticks + SERIAL_TICKS_OFFSET) == 0)
            gb.serialPort.tick();
    }

    // PPU
     gb.ppu.tick();

    ++ticks;
}

inline
void Core::frame() {
    byte& LY = gb.video.LY;
    if (LY >= 144) {
        while (LY != 0) {
            tick();
            RETURN_IF_DEBUGGER_IS_ASKING_TO_SHUTDOWN();
        }
    }
    while (LY < 144) {
        tick();
        RETURN_IF_DEBUGGER_IS_ASKING_TO_SHUTDOWN();
    }
}

inline
void Core::setKey(Joypad::Key key, Joypad::KeyState state) const {
    gb.joypad.setKeyState(key, state);
}