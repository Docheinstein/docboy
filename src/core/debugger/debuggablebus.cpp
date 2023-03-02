#include "debuggablebus.h"

DebuggableBus::DebuggableBus(IMemory &wram1, IMemory &wram2, IMemory &io, IMemory &hram, IMemory &ie) :
    Bus(wram1, wram2, io, hram, ie),
    observer() {

}

void DebuggableBus::setObserver(DebuggableBus::Observer *obs) {
    observer = obs;
}

void DebuggableBus::unsetObserver() {
    observer = nullptr;
}

uint8_t DebuggableBus::read(uint16_t addr, bool notify) const {
    uint8_t value = Bus::read(addr);
    if (notify && observer)
        observer->onBusRead(addr, value);
    return value;
}

void DebuggableBus::write(uint16_t addr, uint8_t value, bool notify) {
    uint8_t oldValue = Bus::read(addr);
    Bus::write(addr, value);
    if (notify && observer)
        observer->onBusWrite(addr, oldValue, value);
}

uint8_t DebuggableBus::read(uint16_t addr) const {
    return read(addr, true);
}

void DebuggableBus::write(uint16_t addr, uint8_t value) {
    write(addr, value, true);
}