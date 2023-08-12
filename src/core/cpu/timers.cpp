#include "timers.h"
#include "core/definitions.h"

Timers::Timers(IInterruptsIO& interrupts) :
    interrupts(interrupts),
    DIV(),
    TIMA(),
    TMA(),
    TAC(),
    divTicks(),
    timaTicks() {
}

void Timers::loadState(IReadableState& state) {
    DIV = state.readUInt8();
    TIMA = state.readUInt8();
    TMA = state.readUInt8();
    TAC = state.readUInt8();
    divTicks = state.readUInt32();
    timaTicks = state.readUInt32();
}

void Timers::saveState(IWritableState& state) {
    state.writeUInt8(DIV);
    state.writeUInt8(TIMA);
    state.writeUInt8(TMA);
    state.writeUInt8(TAC);
    state.writeUInt32(divTicks);
    state.writeUInt32(timaTicks);
}

void Timers::tick() {
    constexpr uint32_t DIV_PERIOD = Specs::CPU::FREQUENCY / Specs::CPU::DIV_FREQUENCY;

    // DIV
    divTicks += 1;
    if (divTicks >= DIV_PERIOD) {
        divTicks = 0;
        writeDIV(readDIV() + 1);
    }

    // TIMA
    if (get_bit<Bits::Timers::TAC::ENABLE>(readTAC()))
        timaTicks += 1;

    const uint32_t TIMA_PERIOD = Specs::CPU::FREQUENCY / Specs::CPU::TAC_FREQUENCY[keep_bits<2>(TAC)];
    if (timaTicks >= TIMA_PERIOD) {
        timaTicks = 0;
        auto [result, overflow] = sum_carry<7>(readTIMA(), 1);
        if (overflow) {
            writeTIMA(readTMA());
            interrupts.setIF<Bits::Interrupts::TIMER>();
        } else {
            writeTIMA(result);
        }
    }
}

uint8_t Timers::readDIV() const {
    return DIV;
}

void Timers::writeDIV(uint8_t value) {
    DIV = value;
}

uint8_t Timers::readTIMA() const {
    return TIMA;
}

void Timers::writeTIMA(uint8_t value) {
    TIMA = value;
}

uint8_t Timers::readTMA() const {
    return TMA;
}

void Timers::writeTMA(uint8_t value) {
    TMA = value;
}

uint8_t Timers::readTAC() const {
    return TAC;
}

void Timers::writeTAC(uint8_t value) {
    TAC = value;
}
