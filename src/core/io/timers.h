#ifndef TIMERSIO_H
#define TIMERSIO_H

#include <cstdint>

class ITimersIO {
public:
    virtual ~ITimersIO() = default;

    [[nodiscard]] virtual uint8_t readDIV() const = 0;
    virtual void writeDIV(uint8_t value) = 0;

    [[nodiscard]] virtual uint8_t readTIMA() const = 0;
    virtual void writeTIMA(uint8_t value) = 0;

    [[nodiscard]] virtual uint8_t readTMA() const = 0;
    virtual void writeTMA(uint8_t value) = 0;

    [[nodiscard]] virtual uint8_t readTAC() const = 0;
    virtual void writeTAC(uint8_t value) = 0;
};

#endif // TIMERSIO_H