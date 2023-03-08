#include "port.h"
#include "link.h"
#include "core/bus/bus.h"
#include "core/definitions.h"
#include "utils/binutils.h"

SerialPort::SerialPort(IBus &bus) : SerialEndpoint(), bus(bus), link() {

}


uint8_t SerialPort::serialRead() {
    return bus.read(Registers::Serial::SB);
}

void SerialPort::serialWrite(uint8_t data) {
    bus.write(Registers::Serial::SB, data);
    uint8_t SC = bus.read(Registers::Serial::SC);
    set_bit<7>(SC, false);
    bus.write(Registers::Serial::SC, SC);
    // TODO: interrupt
}

void SerialPort::tick() {
    if (link)
        link->tick();
}

void SerialPort::attachSerialLink(SerialLink::Plug &plug) {
    link = &plug.attach(this);
}

void SerialPort::detachSerialLink() {
    link = nullptr;
}