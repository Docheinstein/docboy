#ifndef WRITABLESTATE_H
#define WRITABLESTATE_H

#include <cstdint>
#include <vector>

class IWritableState {
public:
    virtual ~IWritableState() = default;

    virtual void writeBool(bool value) = 0;

    virtual void writeUInt8(uint8_t value) = 0;
    virtual void writeUInt16(uint16_t value) = 0;
    virtual void writeUInt32(uint32_t value) = 0;
    virtual void writeUInt64(uint64_t value) = 0;

    virtual void writeInt8(int8_t value) = 0;
    virtual void writeInt16(int16_t value) = 0;
    virtual void writeInt32(int32_t value) = 0;
    virtual void writeInt64(int64_t value) = 0;

    virtual void writeBytes(const std::vector<uint8_t>& bytes) = 0;
    virtual void writeBytes(void* bytes, uint32_t count) = 0;
};

#endif // WRITABLESTATE_H