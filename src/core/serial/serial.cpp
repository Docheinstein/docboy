#include "serial.h"

SerialIO::SerialIO(IInterruptsIO& interrupts) :
    interrupts(interrupts),
    SB(),
    SC() {
}

uint8_t SerialIO::readSB() const {
    return SB;
}

void SerialIO::writeSB(uint8_t value) {
    SB = value;
}

uint8_t SerialIO::readSC() const {
    return SC;
}

void SerialIO::writeSC(uint8_t value) {
    SC = value;
}
