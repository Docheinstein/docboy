#include "state.h"
#include <cassert>
#include <cstring>

State::State(const std::vector<uint8_t>& dataIn) {
    data = dataIn;
}

bool State::readBool() {
    return readData<bool>();
}

uint8_t State::readUInt8() {
    return readData<uint8_t>();
}

uint16_t State::readUInt16() {
    return readData<uint16_t>();
}

uint32_t State::readUInt32() {
    return readData<uint32_t>();
}

uint64_t State::readUInt64() {
    return readData<uint64_t>();
}

int8_t State::readInt8() {
    return readData<int8_t>();
}

int16_t State::readInt16() {
    return readData<int16_t>();
}

int32_t State::readInt32() {
    return readData<int32_t>();
}

int64_t State::readInt64() {
    return readData<int64_t>();
}

std::vector<uint8_t> State::readBytes() {
    const auto count = readUInt32();
    std::vector<uint8_t> result(count);
    readData(result.data(), count);
    return result;
}

void State::readBytes(void* result) {
    const auto count = readUInt32();
    readData(result, count);
}

void State::writeBool(bool value) {
    writeData(value);
}

void State::writeUInt8(uint8_t value) {
    writeData(value);
}

void State::writeUInt16(uint16_t value) {
    writeData(value);
}

void State::writeUInt32(uint32_t value) {
    writeData(value);
}

void State::writeUInt64(uint64_t value) {
    writeData(value);
}

void State::writeInt8(int8_t value) {
    writeData(value);
}

void State::writeInt16(int16_t value) {
    writeData(value);
}

void State::writeInt32(int32_t value) {
    writeData(value);
}

void State::writeInt64(int64_t value) {
    writeData(value);
}

void State::writeBytes(const std::vector<uint8_t>& bytes) {
    const auto count = bytes.size();
    writeUInt32(count);
    writeData(bytes.data(), count);
}

void State::writeBytes(void* bytes, uint32_t count) {
    writeUInt32(count);
    writeData(bytes, count);
}

bool State::operator==(const State& other) {
    return data.size() == other.data.size() && memcmp(data.data(), other.data.data(), data.size()) == 0;
}

uint64_t State::hash() const {
    uint64_t h = 17;
    for (const auto& byte : data) {
        auto value = static_cast<uint8_t>(byte);
        h = h * 31 + value;
    }
    return h;
}

const std::vector<uint8_t>& State::getData() const {
    return data;
}

template <typename T, typename std::enable_if_t<std::is_integral_v<T>>*>
T State::readData() {
    T result;
    readData<T>(&result, sizeof(T));
    return result;
}

template <typename T, typename std::enable_if_t<!std::is_pointer_v<T>>*>
void State::readData(T* result, uint32_t count) {
    assert(cursor + count <= data.size());
    memcpy(result, data.data() + cursor, count);
    cursor += count;
}

template <typename T, typename std::enable_if_t<std::is_integral_v<T>>*>
void State::writeData(T value) {
    writeData(&value, sizeof(value));
}

template <typename T, typename std::enable_if_t<!std::is_pointer_v<T>>*>
void State::writeData(const T* value, uint32_t count) {
    const auto size = data.size();
    data.resize(size + count);
    memcpy(data.data() + size, value, count);
}