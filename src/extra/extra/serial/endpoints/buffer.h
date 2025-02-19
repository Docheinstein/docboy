#ifndef SERIALBUFFER_H
#define SERIALBUFFER_H

#include <vector>

#include "docboy/serial/endpoint.h"

class SerialBuffer : public ISerialEndpoint {
public:
    bool serial_read_bit() const override;
    void serial_write_bit(bool bit) override;

    std::vector<uint8_t> buffer;

protected:
    bool serial_write_bit_(bool bit);

private:
    struct {
        uint8_t count {};
        uint8_t value {};
    } data;
};

#endif // SERIALBUFFER_H