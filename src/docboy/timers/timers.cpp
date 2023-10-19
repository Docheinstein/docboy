#include "timers.h"
#include "docboy/interrupts/interrupts.h"

static constexpr uint8_t TAC_DIV_BITS_SELECTOR[] {9, 3, 5, 7};

TimersIO::TimersIO(InterruptsIO& interrupts) :
    interrupts(interrupts) {
}

void TimersIO::saveState(Parcel& parcel) const {
    parcel.writeUInt16(DIV);
    parcel.writeUInt8(TIMA);
    parcel.writeUInt8(TMA);
    parcel.writeUInt8(TAC);
}

void TimersIO::loadState(Parcel& parcel) {
    DIV = parcel.readUInt16();
    TIMA = parcel.readUInt8();
    TMA = parcel.readUInt8();
    TAC = parcel.readUInt8();
}

void TimersIO::writeDIV(uint8_t value) {
    IF_DEBUGGER(uint8_t oldValue = readDIV());
    setDIV(0);
    IF_DEBUGGER(DebuggerMemorySniffer::notifyMemoryWrite(Specs::Registers::Timers::DIV, oldValue, value));
}
#if 0
void TimersIO::writeTAC(uint8_t value) {
    // TODO: this should fit a unique falling edge detector
    const bool tacDivBit = test_bit(DIV, TAC_DIV_BITS_SELECTOR[keep_bits<2>(TAC)]);
    const bool timaWasEnabled = test_bit<Specs::Bits::Timers::TAC::ENABLE>(TAC);
    TAC = value;
    const bool timaIsEnabled = test_bit<Specs::Bits::Timers::TAC::ENABLE>(TAC);
    if ((timaWasEnabled && tacDivBit && !timaIsEnabled)) {
        incTIMA();
    }
}

inline void TimersIO::setDIV(uint16_t value) {
    const uint8_t b = TAC_DIV_BITS_SELECTOR[keep_bits<2>(TAC)];

    const bool tacDivBitBefore = test_bit(DIV, b);
    DIV = value;
    const bool tacDivBitAfter = test_bit(DIV, b);

    // Increment TIMA if:
    // 1) Has falling edge on DIV bit selected by TAC
    // 2) TAC enable is set
    if ((tacDivBitBefore && !tacDivBitAfter) && test_bit<Specs::Bits::Timers::TAC::ENABLE>(TAC)) {
        incTIMA();
    }
}
#endif

void TimersIO::writeTAC(uint8_t value) {
    TAC = value;
    onFallingEdgeIncTIMA();
}

inline void TimersIO::setDIV(uint16_t value) {
    DIV = value;
    onFallingEdgeIncTIMA();
}

inline void TimersIO::incTIMA() {
    if (TIMA == 0xFF) {
        // TIMA would overflow: reset it to TMA and raise interrupt
        TIMA = (uint8_t)TMA;
        interrupts.raiseInterrupt<InterruptsIO::InterruptType::Timer>();
    } else {
        // Increment TIMA
        ++TIMA;
    }
}

void TimersIO::onFallingEdgeIncTIMA() {
    // TIMA is incremented if (DIV bit selected by TAC && TAC enable)
    // was true and now it's false
    const bool tacDivBit = test_bit(DIV, TAC_DIV_BITS_SELECTOR[keep_bits<2>(TAC)]);
    const bool tacEnable = test_bit<Specs::Bits::Timers::TAC::ENABLE>(TAC);
    const bool divBitAndTacEnable = tacDivBit && tacEnable;
    if (lastDivBitAndTacEnable && !divBitAndTacEnable)
        incTIMA();
    lastDivBitAndTacEnable = divBitAndTacEnable;
}

Timers::Timers(InterruptsIO& interrupts) :
    TimersIO(interrupts) {
}

void Timers::tick() {
    setDIV(DIV + 1);
}
