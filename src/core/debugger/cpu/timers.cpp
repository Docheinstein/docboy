#include "timers.h"
#include "core/definitions.h"

DebuggableTimers::DebuggableTimers(IInterruptsIO& interrupts) :
    Timers(interrupts),
    observer() {
}

uint8_t DebuggableTimers::readDIV() const {
    uint8_t value = Timers::readDIV();
    if (observer)
        observer->onReadDIV(value);
    return value;
}

void DebuggableTimers::writeDIV(uint8_t value) {
    uint8_t oldValue = Timers::readDIV();
    Timers::writeDIV(value);
    if (observer)
        observer->onWriteDIV(oldValue, value);
}

uint8_t DebuggableTimers::readTIMA() const {
    uint8_t value = Timers::readTIMA();
    if (observer)
        observer->onReadTIMA(value);
    return value;
}

void DebuggableTimers::writeTIMA(uint8_t value) {
    uint8_t oldValue = Timers::readTIMA();
    Timers::writeTIMA(value);
    if (observer)
        observer->onWriteTIMA(oldValue, value);
}

uint8_t DebuggableTimers::readTMA() const {
    uint8_t value = Timers::readTMA();
    if (observer)
        observer->onReadTMA(value);
    return value;
}

void DebuggableTimers::writeTMA(uint8_t value) {
    uint8_t oldValue = Timers::readTMA();
    Timers::writeTMA(value);
    if (observer)
        observer->onWriteTMA(oldValue, value);
}

uint8_t DebuggableTimers::readTAC() const {
    uint8_t value = Timers::readTAC();
    if (observer)
        observer->onReadTAC(value);
    return value;
}

void DebuggableTimers::writeTAC(uint8_t value) {
    uint8_t oldValue = Timers::readTAC();
    Timers::writeTAC(value);
    if (observer)
        observer->onWriteTAC(oldValue, value);
}

void DebuggableTimers::setObserver(Observer* o) {
    observer = o;
}