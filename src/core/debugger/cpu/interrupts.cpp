#include "interrupts.h"
#include "core/definitions.h"

uint8_t DebuggableInterrupts::readIF() const {
    uint8_t value = Interrupts::readIF();
    if (observer)
        observer->onReadIF(value);
    return value;
}

void DebuggableInterrupts::writeIF(uint8_t value) {
    uint8_t oldValue = Interrupts::readIF();
    Interrupts::writeIF(value);
    if (observer)
        observer->onWriteIF(oldValue, value);
}

uint8_t DebuggableInterrupts::readIE() const {
    uint8_t value = Interrupts::readIE();
    if (observer)
        observer->onReadIE(value);
    return value;
}

void DebuggableInterrupts::writeIE(uint8_t value) {
    uint8_t oldValue = Interrupts::readIE();
    Interrupts::writeIE(value);
    if (observer)
        observer->onWriteIE(oldValue, value);
}

void DebuggableInterrupts::setObserver(Observer* o) {
    observer = o;
}
