#include "extra/serial/endpoints/buffer.h"

bool SerialBuffer::serial_read_bit() const {
    return true;
}

void SerialBuffer::serial_write_bit(bool bit) {
    serial_write_bit_(bit);
}

bool SerialBuffer::serial_write_bit_(bool bit) {
    data.value = (data.value << 1) | bit;

    if (++data.count == 8) {
        // A full byte has been transferred: push it to the buffer
        buffer.push_back(data.value);
        data.count = 0;
        return true;
    }

    return false;
}
