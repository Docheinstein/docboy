#ifndef ENDPOINT_H
#define ENDPOINT_H

#include <cstdint>

class ISerialEndpoint {
public:
    virtual ~ISerialEndpoint() = default;
    virtual uint8_t serial_read() = 0;
    virtual void serial_write(uint8_t) = 0;
};

#endif // ENDPOINT_H