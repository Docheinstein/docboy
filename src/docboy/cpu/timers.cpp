#include "timers.h"
#include "interrupts.h"

static constexpr uint8_t TAC_DIV_BITS_SELECTOR[] {9, 3, 5, 7};

Timers::Timers(InterruptsIO& interrupts) :
    interrupts(interrupts) {
}

void Timers::tick() {
    const uint8_t tacDivBitSelector = TAC_DIV_BITS_SELECTOR[keep_bits<2>(TAC)];
    const bool tacDivBitBefore = test_bit(DIV, tacDivBitSelector);

    ++DIV;

    const bool tacDivBitAfter = test_bit(DIV, tacDivBitSelector);

    if ( // Has falling edge on DIV bit selected by TAC
        (tacDivBitBefore && !tacDivBitAfter) &&
        // TAC enable is set
        test_bit<Specs::Bits::Timers::TAC::ENABLE>(TAC)) {

        // Increment TIMA
        if (TIMA == 0xFF) {
            // TIMA would overflow: reset it to TMA and raise interrupt
            TIMA = (uint8_t)TMA;
            interrupts.raiseInterrupt<InterruptsIO::InterruptType::Timer>();
        } else {
            ++TIMA;
        }
    }
}