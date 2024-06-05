#ifndef BUFFER_H
#define BUFFER_H

#include <vector>

#include "docboy/serial/endpoint.h"

class SerialBuffer : public ISerialEndpoint {
public:
    uint8_t serial_read() override;
    void serial_write(uint8_t) override;

    std::vector<uint8_t> buffer;
};

#endif // BUFFER_H