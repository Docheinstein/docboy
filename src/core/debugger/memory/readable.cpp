#include "readable.h"

DebuggableReadable::DebuggableReadable(size_t size) : Readable(size), observer() {

}

DebuggableReadable::DebuggableReadable(const std::vector<uint8_t> &data)
    : Readable(data), observer() {

}

DebuggableReadable::DebuggableReadable(std::vector<uint8_t> &&data)
    : Readable(data), observer() {

}

void DebuggableReadable::setObserver(IDebuggableReadable::Observer *o) {
    observer = o;
}

uint8_t DebuggableReadable::read(uint16_t index) const {
    uint8_t value = Readable::read(index);
    if (observer)
        observer->onRead(index, value);
    return value;
}

uint8_t DebuggableReadable::readRaw(uint16_t index) const {
    return Readable::read(index);
}
