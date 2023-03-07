#include "debuggablememory.h"

DebuggableMemory::DebuggableMemory(size_t size)
    : Memory(size), observer() {

}

DebuggableMemory::DebuggableMemory(const std::vector<uint8_t> &data)
    : Memory(data), observer() {

}

DebuggableMemory::DebuggableMemory(std::vector<uint8_t> &&data)
    : Memory(data), observer() {

}

uint8_t DebuggableMemory::read(uint16_t index) const {
    uint8_t value = Memory::read(index);
    if (observer)
        observer->onRead(index, value);
    return value;
}

void DebuggableMemory::write(uint16_t index, uint8_t value) {
    uint8_t oldValue = Memory::read(index);
    Memory::write(index, value);
    if (observer)
        observer->onWrite(index, oldValue, value);
}

uint8_t DebuggableMemory::readRaw(uint16_t index) const {
    return Memory::read(index);
}

void DebuggableMemory::writeRaw(uint16_t index, uint8_t value) {
    Memory::write(index, value);
}

void DebuggableMemory::setObserver(IDebuggableMemory::Observer *o) {
    observer = o;
}


DebuggableReadable::DebuggableReadable(size_t size) : Readable(size) {

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
