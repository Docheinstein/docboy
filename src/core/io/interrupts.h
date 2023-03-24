#ifndef INTERRUPTSIO_H
#define INTERRUPTSIO_H

#include <cstdint>
#include "utils/binutils.h"

class IInterruptsIO {
public:
    virtual ~IInterruptsIO() = default;

    [[nodiscard]] virtual uint8_t readIF() const = 0;
    virtual void writeIF(uint8_t value) = 0;

    [[nodiscard]] virtual uint8_t readIE() const = 0;
    virtual void writeIE(uint8_t value) = 0;

    // helper
    template<uint8_t b>
    void setIF() {
        writeIF(set_bit<b>(readIF()));
    }
};

#endif // INTERRUPTSIO_H