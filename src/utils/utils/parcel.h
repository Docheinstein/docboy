#ifndef PARCEL_H
#define PARCEL_H

#include <cstdint>
#include <vector>

class Parcel {
public:
    Parcel() = default;

    explicit Parcel(const void* data, uint32_t size);

    bool readBool();

    uint8_t readUInt8();
    uint16_t readUInt16();
    uint32_t readUInt32();
    uint64_t readUInt64();

    int8_t readInt8();
    int16_t readInt16();
    int32_t readInt32();
    int64_t readInt64();

    void readBytes(void* data, uint32_t count);

    void writeBool(bool value);

    void writeUInt8(uint8_t value);
    void writeUInt16(uint16_t value);
    void writeUInt32(uint32_t value);
    void writeUInt64(uint64_t value);

    void writeInt8(int8_t value);
    void writeInt16(int16_t value);
    void writeInt32(int32_t value);
    void writeInt64(int64_t value);

    void writeBytes(const void* data, uint32_t count);

    [[nodiscard]] const void* getData() const;
    [[nodiscard]] uint32_t getSize() const;
    [[nodiscard]] uint32_t getRemainingSize() const;

private:
    template <typename T>
    T read();

    template <typename T>
    void write(T value);

    void readRaw(void* data, uint32_t count);
    void writeRaw(const void* data, uint32_t count);

    std::vector<uint8_t> data;
    uint32_t cursor {};
};

#endif // PARCEL_H