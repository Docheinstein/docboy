#ifndef TIMERS_H
#define TIMERS_H

#include "docboy/bootrom/macros.h"
#include "docboy/debugger/macros.h"
#include "docboy/memory/byte.hpp"
#include "docboy/shared/specs.h"
#include "utils/parcel.h"

#ifdef ENABLE_DEBUGGER
#include "docboy/debugger/memsniffer.h"
#endif

class InterruptsIO;

class TimersIO {
public:
    void saveState(Parcel& parcel) const {
        parcel.writeUInt8(DIV);
        parcel.writeUInt8(TIMA);
        parcel.writeUInt8(TMA);
        parcel.writeUInt8(TAC);
    }

    void loadState(Parcel& parcel) {
        DIV = parcel.readUInt8();
        TIMA = parcel.readUInt8();
        TMA = parcel.readUInt8();
        TAC = parcel.readUInt8();
    }

    [[nodiscard]] uint8_t readDIV() const {
        const uint8_t DIVh = DIV >> 8;
        IF_DEBUGGER(DebuggerMemorySniffer::notifyMemoryRead(Specs::Registers::Timers::DIV, DIVh));
        return DIVh;
    }

    void writeDIV(uint8_t value) {
        IF_DEBUGGER(uint8_t oldValue = readDIV());
        DIV = 0;
        IF_DEBUGGER(DebuggerMemorySniffer::notifyMemoryWrite(Specs::Registers::Timers::DIV, oldValue, value));
    }

    uint16_t DIV {IF_NOT_BOOTROM(0xABCC)};
    BYTE(TIMA, Specs::Registers::Timers::TIMA);
    BYTE(TMA, Specs::Registers::Timers::TMA);
    BYTE(TAC, Specs::Registers::Timers::TAC);
};

class Timers : public TimersIO {
public:
    explicit Timers(InterruptsIO& interrupts);

    void tick();

private:
    InterruptsIO& interrupts;
};
#endif // TIMERS_H