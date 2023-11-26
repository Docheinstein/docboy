#include "timers.h"
#include "docboy/interrupts/interrupts.h"

/* TIMA overflow timing example.
 *   t                  CPU                              Timer
 * CPU   |
 * Timer |                                      TIMA overflows -> Pending
 *
 * CPU   | [writing TIMA aborts PendingReload]
 *       | [writing TMA is considered]
 * Timer |                                      Pending -> Reload (TIMA = TMA, raise interrupt)
 *
 * CPU   | [writing TIMA is ignored]
 *       | [writing TMA writes TIMA too]
 * DMA   |                                      Reload -> None
 */

TimersIO::TimersIO(InterruptsIO& interrupts) :
    interrupts(interrupts) {
}

void TimersIO::saveState(Parcel& parcel) const {
    parcel.writeUInt16(DIV);
    parcel.writeUInt8(TIMA);
    parcel.writeUInt8(TMA);
    parcel.writeUInt8(TAC);
    parcel.writeUInt8(timaState);
    parcel.writeBool(lastDivBitAndTacEnable);
}

void TimersIO::loadState(Parcel& parcel) {
    DIV = parcel.readUInt16();
    TIMA = parcel.readUInt8();
    TMA = parcel.readUInt8();
    TAC = parcel.readUInt8();
    timaState = parcel.readUInt8();
    lastDivBitAndTacEnable = parcel.readBool();
}

void TimersIO::writeDIV(uint8_t value) {
    IF_DEBUGGER(uint8_t oldValue = readDIV());
    setDIV(0);
    IF_DEBUGGER(DebuggerMemorySniffer::notifyMemoryWrite(Specs::Registers::Timers::DIV, oldValue, value));
}

void TimersIO::writeTIMA(uint8_t value) {
    // Writing to TIMA in the same cycle it has been reloaded is ignored
    if (timaState != TimaReloadState::Reload) {
        TIMA = value;
        timaState = TimaReloadState::None;
    }
}

void TimersIO::writeTMA(uint8_t value) {
    // Writing TMA in the same cycle TIMA has been reloaded writes TIMA too
    TMA = value;
    if (timaState == TimaReloadState::Reload)
        TIMA = value;
}

void TimersIO::writeTAC(uint8_t value) {
    TAC = 0b11111000 | value;
    onFallingEdgeIncTima();
}

inline void TimersIO::setDIV(uint16_t value) {
    DIV = value;
    onFallingEdgeIncTima();
}

inline void TimersIO::incTIMA() {
    // When TIMA overflows, TMA is reloaded in TIMA: but it is delayed by 1 m-cycle
    if (++TIMA == 0) {
        timaState = TimaReloadState::Pending;
    }
}

inline void TimersIO::onFallingEdgeIncTima() {
    // TIMA is incremented if (DIV bit selected by TAC && TAC enable)
    // was true and now it's false
    const bool tacDivBit = test_bit(DIV, Specs::Timers::TAC_DIV_BITS_SELECTOR[keep_bits<2>(TAC)]);
    const bool tacEnable = test_bit<Specs::Bits::Timers::TAC::ENABLE>(TAC);
    const bool divBitAndTacEnable = tacDivBit && tacEnable;
    if (lastDivBitAndTacEnable && !divBitAndTacEnable)
        incTIMA();
    lastDivBitAndTacEnable = divBitAndTacEnable;
}

inline void TimersIO::handlePendingTimaReload() {
    if (timaState != TimaReloadState::None) {
        if (--timaState == TimaReloadState::Reload) {
            TIMA = (uint8_t)TMA;
            interrupts.raiseInterrupt<InterruptsIO::InterruptType::Timer>();
        }
    }
}

Timers::Timers(InterruptsIO& interrupts) :
    TimersIO(interrupts) {
}

void Timers::tick() {
    handlePendingTimaReload();
    setDIV(DIV + 4);
}
