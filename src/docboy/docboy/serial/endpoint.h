#ifndef SERIALENDPOINT_H
#define SERIALENDPOINT_H

#include <cstdint>

class ISerialEndpoint {
public:
    virtual ~ISerialEndpoint() = default;

    virtual bool serial_read_bit() const = 0;
    virtual void serial_write_bit(bool) = 0;
};

#endif // SERIALENDPOINT_H