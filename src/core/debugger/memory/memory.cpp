#include "memory.h"

DebuggableMemory::DebuggableMemory(size_t size)
    : Memory(size), robserver(), wobserver() {

}

DebuggableMemory::DebuggableMemory(const std::vector<uint8_t> &data)
    : Memory(data), robserver(), wobserver() {

}

DebuggableMemory::DebuggableMemory(std::vector<uint8_t> &&data)
    : Memory(data), robserver(), wobserver() {

}

uint8_t DebuggableMemory::read(uint16_t index) const {
    uint8_t value = Memory::read(index);
    if (robserver)
        robserver->onRead(index, value);
    return value;
}

void DebuggableMemory::write(uint16_t index, uint8_t value) {
    uint8_t oldValue = Memory::read(index);
    Memory::write(index, value);
    if (wobserver)
        wobserver->onWrite(index, oldValue, value);
}

void DebuggableMemory::setObserver(IReadableDebug::Observer  *o) {
    robserver = o;
}

void DebuggableMemory::setObserver(IWritableDebug::Observer  *o) {
    wobserver = o;
}

void DebuggableMemory::setObserver(IMemoryDebug::Observer *o) {
    robserver = o;
    wobserver = o;
}

