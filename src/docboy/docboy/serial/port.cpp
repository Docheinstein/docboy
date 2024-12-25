#include "docboy/serial/port.h"

#include "docboy/interrupts/interrupts.h"

SerialPort::SerialPort(Interrupts& interrupts) :
    interrupts {interrupts} {
}

void SerialPort::tick() {
    if (!test_bit<Specs::Bits::Serial::SC::TRANSFER_START>(sc) || !test_bit<Specs::Bits::Serial::SC::CLOCK>(sc)) {
        return;
    }

#ifdef ENABLE_SERIAL
    if (link) {
        link->tick();
        return;
    }
#endif

    // disconnected cable
    serial_write(0xFF);
}

void SerialPort::attach(SerialLink::Plug& plug) {
    link = &plug.attach(*this);
}

void SerialPort::detach() {
    link = nullptr;
}

uint8_t SerialPort::serial_read() {
    return sb;
}

void SerialPort::serial_write(uint8_t data) {
    sb = data;
    reset_bit<Specs::Bits::Serial::SC::TRANSFER_START>(sc);
    interrupts.raise_interrupt<Interrupts::InterruptType::Serial>();
}