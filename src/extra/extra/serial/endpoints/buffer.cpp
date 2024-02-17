#include "buffer.h"

void SerialBuffer::serialWrite(uint8_t data) {
    buffer.push_back(data);
}

uint8_t SerialBuffer::serialRead() {
    return 0xFF;
}