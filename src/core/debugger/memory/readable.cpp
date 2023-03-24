#include "readable.h"

DebuggableReadOnlyMemory::DebuggableReadOnlyMemory(size_t size)
    : ReadOnlyMemory(size), observer() {

}

DebuggableReadOnlyMemory::DebuggableReadOnlyMemory(const std::vector<uint8_t> &data)
    : ReadOnlyMemory(data), observer() {

}

DebuggableReadOnlyMemory::DebuggableReadOnlyMemory(std::vector<uint8_t> &&data)
    : ReadOnlyMemory(data), observer() {

}

void DebuggableReadOnlyMemory::setObserver(IReadableDebug::Observer *o) {
    observer = o;
}

uint8_t DebuggableReadOnlyMemory::read(uint16_t index) const {
    uint8_t value = ReadOnlyMemory::read(index);
    if (observer)
        observer->onRead(index, value);
    return value;
}