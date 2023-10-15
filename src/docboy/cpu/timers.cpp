#include "timers.h"
#include "interrupts.h"

Timers::Timers(InterruptsIO& interrupts) :
    interrupts(interrupts) {
}

void Timers::tickDIV() {
    ++DIV;
}

void Timers::tickTIMA() {
    if (TIMA == 0xFF) {
        TIMA = (uint8_t)TMA;
        interrupts.raiseInterrupt<InterruptsIO::InterruptType::Timer>();
    } else {
        ++TIMA;
    }
}
