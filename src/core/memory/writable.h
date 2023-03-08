#ifndef WRITABLE_H
#define WRITABLE_H

#include <cstdint>

class IWritable {
public:
    virtual ~IWritable() = default;
    virtual void write(uint16_t index, uint8_t value) = 0;
};

#endif // WRITABLE_H