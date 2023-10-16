#include "core.h"

#ifdef ENABLE_DEBUGGER
#include "docboy/debugger/backend.h"
#define TICK_DEBUGGER(n) if (debugger) debugger->onTick(n)
#define RETURN_IF_DEBUGGER_IS_ASKING_TO_SHUTDOWN() if (debugger && debugger->isAskingToShutdown()) return
#else
#define TICK_DEBUGGER(n) (void)(0)
#define RETURN_IF_DEBUGGER_IS_ASKING_TO_SHUTDOWN() (void)(0)
#endif


static constexpr uint8_t CPU_PERIOD = Specs::Frequencies::CLOCK / Specs::Frequencies::CPU;
static constexpr uint16_t DIV_PERIOD = Specs::Frequencies::CLOCK / Specs::Frequencies::DIV;
static constexpr uint16_t SERIAL_PERIOD = Specs::Frequencies::CLOCK / Specs::Frequencies::SERIAL;
static constexpr uint16_t TIMA_PERIODS[] = {
    Specs::Frequencies::CLOCK / Specs::Frequencies::TAC[0],
    Specs::Frequencies::CLOCK / Specs::Frequencies::TAC[1],
    Specs::Frequencies::CLOCK / Specs::Frequencies::TAC[2],
    Specs::Frequencies::CLOCK / Specs::Frequencies::TAC[3],
};

inline
void Core::tick() {
    TICK_DEBUGGER(ticks);
    RETURN_IF_DEBUGGER_IS_ASKING_TO_SHUTDOWN();

    if (ticks % CPU_PERIOD == 0) {
        // DIV
        if (ticks % DIV_PERIOD == 0)
            gb.timers.tickDIV();

        // TIMA
        if (test_bit<Specs::Bits::Timers::TAC::ENABLE>(gb.timers.TAC)) {
            if (ticks % TIMA_PERIODS[keep_bits<2>(gb.timers.TAC)] == 0)
                gb.timers.tickTIMA();
        }

        // CPU
        gb.cpu.tick();

        // DMA
        gb.dma.tick();

        // Serial
        if (ticks % SERIAL_PERIOD == 0)
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