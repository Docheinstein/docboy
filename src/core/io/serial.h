#ifndef SERIALIO_H
#define SERIALIO_H

#include <cstdint>

class ISerialIO {
public:
    virtual ~ISerialIO() = default;

    [[nodiscard]] virtual uint8_t readSB() const = 0;
    virtual void writeSB(uint8_t value) = 0;

    [[nodiscard]] virtual uint8_t readSC() const = 0;
    virtual void writeSC(uint8_t value) = 0;
};

#endif // SERIALIO_H