#ifndef BOOTIO_H
#define BOOTIO_H

#include <cstdint>

class IBootIO {
public:
    virtual ~IBootIO() = default;

    [[nodiscard]] virtual uint8_t readBOOT() const = 0;
    virtual void writeBOOT(uint8_t value) = 0;
};

#endif // BOOTIO_H