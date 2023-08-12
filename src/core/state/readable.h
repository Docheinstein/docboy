#ifndef READABLESTATE_H
#define READABLESTATE_H

#include <cstdint>
#include <vector>

class IReadableState {
public:
    virtual ~IReadableState() = default;

    virtual bool readBool() = 0;

    virtual uint8_t readUInt8() = 0;
    virtual uint16_t readUInt16() = 0;
    virtual uint32_t readUInt32() = 0;
    virtual uint64_t readUInt64() = 0;

    virtual int8_t readInt8() = 0;
    virtual int16_t readInt16() = 0;
    virtual int32_t readInt32() = 0;
    virtual int64_t readInt64() = 0;

    virtual std::vector<uint8_t> readBytes() = 0;
    virtual void readBytes(void* result) = 0;
};

#endif // READABLESTATE_H