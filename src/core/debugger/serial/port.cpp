#include "port.h"
#include "core/definitions.h"

DebuggableSerialPort::DebuggableSerialPort(IInterruptsIO &interrupts) :
    SerialPort(interrupts), observer() {

}


uint8_t DebuggableSerialPort::readSB() const {
    uint8_t value = SerialPort::readSB();
    if (observer)
        observer->onReadSB(value);
    return value;
}

void DebuggableSerialPort::writeSB(uint8_t value) {
    uint8_t oldValue = SerialPort::readSB();
    SerialPort::writeSB(value);
    if (observer)
        observer->onWriteSB(oldValue, value);
}

uint8_t DebuggableSerialPort::readSC() const {
    uint8_t value = SerialPort::readSC();
    if (observer)
        observer->onReadSC(value);
    return value;
}

void DebuggableSerialPort::writeSC(uint8_t value) {
    uint8_t oldValue = SerialPort::readSC();
    SerialPort::writeSC(value);
    if (observer)
        observer->onWriteSC(oldValue, value);
}

void DebuggableSerialPort::setObserver(Observer *o) {
    observer = o;
}