#include "boot.h"


DebuggableBoot::DebuggableBoot(std::unique_ptr<IBootROM> bootRom) :
    Boot(std::move(bootRom)), romObserver(), ioObserver() {

}

uint8_t DebuggableBoot::read(uint16_t index) const {
    uint8_t value = Boot::read(index);
    if (romObserver)
        romObserver->onRead(index, value);
    return value;
}

uint8_t DebuggableBoot::readBOOT() const {
    uint8_t value = Boot::readBOOT();
    if (ioObserver)
        ioObserver->onReadBOOT(value);
    return value;
}

void DebuggableBoot::writeBOOT(uint8_t value) {
    uint8_t oldValue = Boot::readBOOT();
    Boot::writeBOOT(value);
    if (ioObserver)
        ioObserver->onWriteBOOT(oldValue, value);
}

void DebuggableBoot::setObserver(IReadableDebug::Observer *o) {
    romObserver = o;
}

void DebuggableBoot::setObserver(IBootIODebug::Observer *o) {
    ioObserver = o;
}
