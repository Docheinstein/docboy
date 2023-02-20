#include "serialbuffer.h"

SerialBufferEndpoint::SerialBufferEndpoint() = default;
SerialBufferEndpoint::~SerialBufferEndpoint() = default;

void SerialBufferEndpoint::serialWrite(uint8_t data) {
    buffer.push_back(data);
}

uint8_t SerialBufferEndpoint::serialRead() {
    return 0xFF;
}

const std::vector<uint8_t> & SerialBufferEndpoint::getData() const {
    return buffer;
}

void SerialBufferEndpoint::clearData() {
    buffer.clear();
}

