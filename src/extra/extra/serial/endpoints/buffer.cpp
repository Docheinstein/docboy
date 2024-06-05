#include "extra/serial/endpoints/buffer.h"

void SerialBuffer::serial_write(uint8_t data) {
    buffer.push_back(data);
}

uint8_t SerialBuffer::serial_read() {
    return 0xFF;
}