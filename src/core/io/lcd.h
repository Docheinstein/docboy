#ifndef LCDIO_H
#define LCDIO_H

#include <cstdint>

class ILCDIO {
public:
    virtual ~ILCDIO() = default;

    [[nodiscard]] virtual uint8_t readLCDC() const = 0;
    virtual void writeLCDC(uint8_t value) = 0;

    [[nodiscard]] virtual uint8_t readSTAT() const = 0;
    virtual void writeSTAT(uint8_t value) = 0;

    [[nodiscard]] virtual uint8_t readSCY() const = 0;
    virtual void writeSCY(uint8_t value) = 0;

    [[nodiscard]] virtual uint8_t readSCX() const = 0;
    virtual void writeSCX(uint8_t value) = 0;

    [[nodiscard]] virtual uint8_t readLY() const = 0;
    virtual void writeLY(uint8_t value) = 0;

    [[nodiscard]] virtual uint8_t readLYC() const = 0;
    virtual void writeLYC(uint8_t value) = 0;

    [[nodiscard]] virtual uint8_t readDMA() const = 0;
    virtual void writeDMA(uint8_t value) = 0;

    [[nodiscard]] virtual uint8_t readBGP() const = 0;
    virtual void writeBGP(uint8_t value) = 0;

    [[nodiscard]] virtual uint8_t readOBP0() const = 0;
    virtual void writeOBP0(uint8_t value) = 0;

    [[nodiscard]] virtual uint8_t readOBP1() const = 0;
    virtual void writeOBP1(uint8_t value) = 0;

    [[nodiscard]] virtual uint8_t readWY() const = 0;
    virtual void writeWY(uint8_t value) = 0;

    [[nodiscard]] virtual uint8_t readWX() const = 0;
    virtual void writeWX(uint8_t value) = 0;
};

#endif // LCDIO_H