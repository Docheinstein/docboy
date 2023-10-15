#include "port.h"
#include "docboy/cpu/interrupts.h"
#include "link.h"

SerialPort::SerialPort(InterruptsIO& interrupts) :
    interrupts(interrupts) {
}

void SerialPort::tick() {
    if (!test_bit<Specs::Bits::Serial::SC::TRANSFER_START>(SC) || !test_bit<Specs::Bits::Serial::SC::CLOCK>(SC))
        return;

#ifdef ENABLE_SERIAL
    if (link) {
        link->tick();
        return;
    }
#endif

    // disconnected cable
    serialWrite(0xFF);
}

void SerialPort::attach(SerialLink::Plug& plug) {
    link = &plug.attach(this);
}

void SerialPort::detach() {
    link = nullptr;
}

uint8_t SerialPort::serialRead() {
    return SB;
}

void SerialPort::serialWrite(uint8_t data) {
    SB = data;
    reset_bit<Specs::Bits::Serial::SC::TRANSFER_START>(SC);
    interrupts.raiseInterrupt<InterruptsIO::InterruptType::Serial>();
}