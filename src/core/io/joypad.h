#ifndef JOYPADIO_H
#define JOYPADIO_H

#include <cstddef>
#include <cstdint>

class IJoypadIO {
public:
    virtual ~IJoypadIO() = default;

    [[nodiscard]] virtual uint8_t readP1() const = 0;
    virtual void writeP1(uint8_t value) = 0;
};


#endif // JOYPADIO_H