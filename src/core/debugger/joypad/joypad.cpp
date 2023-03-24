#include "joypad.h"
#include "core/definitions.h"


DebuggableJoypad::DebuggableJoypad(IInterruptsIO &interrupts) :
    Joypad(interrupts), observer() {

}

uint8_t DebuggableJoypad::readP1() const {
    uint8_t value = Joypad::readP1();
    if (observer)
        observer->onReadP1(value);
    return value;
}

void DebuggableJoypad::writeP1(uint8_t value) {
    uint8_t oldValue = Joypad::readP1();
    Joypad::writeP1(value);
    if (observer)
        observer->onWriteP1(oldValue, value);
}

void DebuggableJoypad::setObserver(Observer *o) {
    observer = o;
}

