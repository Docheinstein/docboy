#include "port.h"
#include "core/definitions.h"
#include "link.h"
#include "serial.h"
#include "utils/binutils.h"

SerialPort::SerialPort(IInterruptsIO& interrupts) :
    interrupts(interrupts),
    link(),
    SB(),
    SC() {
}

uint8_t SerialPort::serialRead() {
    // TODO: interrupt ?
    return readSB();
}

void SerialPort::serialWrite(uint8_t data) {
    writeSB(data);
    writeSC(reset_bit<7>(readSC()));
    interrupts.setIF<Bits::Interrupts::SERIAL>();
}

void SerialPort::tick() {
    if (link)
        link->tick();
    else {
        // disconnected cable
        serialWrite(0xFF);
    }
}

void SerialPort::attachSerialLink(SerialLink::Plug& plug) {
    link = &plug.attach(this);
}

void SerialPort::detachSerialLink() {
    link = nullptr;
}

uint8_t SerialPort::readSB() const {
    return SB;
}

void SerialPort::writeSB(uint8_t value) {
    SB = value;
}

uint8_t SerialPort::readSC() const {
    return SC;
}

void SerialPort::writeSC(uint8_t value) {
    SC = value;
}
