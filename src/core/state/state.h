#ifndef STATE_H
#define STATE_H

#include "readable.h"
#include "writable.h"
#include <vector>

class IStataData {
public:
    ~IStataData() = default;
    [[nodiscard]] virtual const std::vector<uint8_t>& getData() const = 0;
};

class State : public IReadableState, public IWritableState, public IStataData {
public:
    explicit State() = default;
    explicit State(const std::vector<uint8_t>& data);
    [[nodiscard]] const std::vector<uint8_t>& getData() const override;

    [[nodiscard]] bool readBool() override;
    [[nodiscard]] uint8_t readUInt8() override;
    [[nodiscard]] uint16_t readUInt16() override;
    [[nodiscard]] uint32_t readUInt32() override;
    [[nodiscard]] uint64_t readUInt64() override;
    [[nodiscard]] int8_t readInt8() override;
    [[nodiscard]] int16_t readInt16() override;
    [[nodiscard]] int32_t readInt32() override;
    [[nodiscard]] int64_t readInt64() override;
    [[nodiscard]] std::vector<uint8_t> readBytes() override;
    void readBytes(void* result) override;

    void writeBool(bool value) override;
    void writeUInt8(uint8_t value) override;
    void writeUInt16(uint16_t value) override;
    void writeUInt32(uint32_t value) override;
    void writeUInt64(uint64_t value) override;
    void writeInt8(int8_t value) override;
    void writeInt16(int16_t value) override;
    void writeInt32(int32_t value) override;
    void writeInt64(int64_t value) override;
    void writeBytes(const std::vector<uint8_t>& bytes) override;
    void writeBytes(void* bytes, uint32_t count) override;

    bool operator==(const State& other);
    [[nodiscard]] uint64_t hash() const;

private:
    std::vector<std::uint8_t> data;
    uint32_t cursor {};

    template <typename T, typename std::enable_if_t<std::is_integral_v<T>>* = nullptr>
    T readData();

    template <typename T, typename std::enable_if_t<!std::is_pointer_v<T>>* = nullptr>
    void readData(T* result, uint32_t count);

    template <typename T, typename std::enable_if_t<std::is_integral_v<T>>* = nullptr>
    void writeData(T value);

    template <typename T, typename std::enable_if_t<!std::is_pointer_v<T>>* = nullptr>
    void writeData(const T* value, uint32_t count);
};

#endif // STATE_H