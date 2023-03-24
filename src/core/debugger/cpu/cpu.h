#ifndef DEBUGGERCPU_H
#define DEBUGGERCPU_H

#include "core/cpu/cpu.h"

class ICPUDebug {
public:
    struct State {
        struct Registers {
            uint16_t AF;
            uint16_t BC;
            uint16_t DE;
            uint16_t HL;
            uint16_t PC;
            uint16_t SP;
        } registers;
        struct Instruction {
            bool ISR;
            // TODO: union if ISR?
            uint16_t address;
            uint8_t microop;
        } instruction;
        bool IME;
        bool halted;
        uint64_t cycles;
    };

    virtual ~ICPUDebug() = default;

    virtual State getState() = 0;
};

class DebuggableCPU : public ICPUDebug, public CPU {
public:
    DebuggableCPU(
            IBus &bus,
            IClockable &timers,
            IClockable &serial,
            bool bootRom = false);
    ~DebuggableCPU() override = default;

    State getState() override;
};


#endif // DEBUGGERCPU_H