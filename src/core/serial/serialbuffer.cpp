#include "serialbuffer.h"

SerialBuffer::SerialBuffer() = default;
SerialBuffer::~SerialBuffer() = default;

void SerialBuffer::serialWrite(uint8_t data) {
    buffer.push_back(data);
}

uint8_t SerialBuffer::serialRead() {
    return 0xFF;
}

const std::vector<uint8_t> & SerialBuffer::getData() const {
    return buffer;
}

void SerialBuffer::clearData() {
    buffer.clear();
}

