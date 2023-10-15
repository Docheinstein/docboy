#ifndef TIMERS_H
#define TIMERS_H

#include "docboy/memory/byte.hpp"
#include "docboy/shared/specs.h"
#include "utils/parcel.h"

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

    BYTE(DIV, Specs::Registers::Timers::DIV);
    BYTE(TIMA, Specs::Registers::Timers::TIMA);
    BYTE(TMA, Specs::Registers::Timers::TMA);
    BYTE(TAC, Specs::Registers::Timers::TAC);
};

class Timers : public TimersIO {
public:
    explicit Timers(InterruptsIO& interrupts);

    void tickDIV();
    void tickTIMA();

private:
    InterruptsIO& interrupts;
};
#endif // TIMERS_H