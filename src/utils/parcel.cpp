#include "parcel.h"
#include "asserts.h"
#include <cstring>

bool Parcel::readBool() {
    return read<bool>();
}

uint8_t Parcel::readUInt8() {
    return read<uint8_t>();
}

uint16_t Parcel::readUInt16() {
    return read<uint16_t>();
}

uint32_t Parcel::readUInt32() {
    return read<uint32_t>();
}

uint64_t Parcel::readUInt64() {
    return read<uint64_t>();
}

int8_t Parcel::readInt8() {
    return read<int8_t>();
}

int16_t Parcel::readInt16() {
    return read<int16_t>();
}

int32_t Parcel::readInt32() {
    return read<int32_t>();
}

int64_t Parcel::readInt64() {
    return read<int64_t>();
}

void Parcel::readBytes(void* data_, uint32_t count) {
    readRaw(data_, count);
}

void Parcel::writeBool(bool value) {
    write(value);
}

void Parcel::writeUInt8(uint8_t value) {
    write(value);
}

void Parcel::writeUInt16(uint16_t value) {
    write(value);
}

void Parcel::writeUInt32(uint32_t value) {
    write(value);
}

void Parcel::writeUInt64(uint64_t value) {
    write(value);
}

void Parcel::writeInt8(int8_t value) {
    write(value);
}

void Parcel::writeInt16(int16_t value) {
    write(value);
}

void Parcel::writeInt32(int32_t value) {
    write(value);
}

void Parcel::writeInt64(int64_t value) {
    write(value);
}

void Parcel::writeBytes(const void* data_, uint32_t count) {
    writeRaw(data_, count);
}

template <typename T>
T Parcel::read() {
    T result;
    readRaw(&result, sizeof(T));
    return result;
}

template <typename T>
void Parcel::write(T value) {
    writeRaw(&value, sizeof(T));
}

void Parcel::readRaw(void* result, uint32_t count) {
    check(getRemainingSize() > 0);
    memcpy(result, data.data() + cursor, count);
    cursor += count;
}

void Parcel::writeRaw(const void* bytes, uint32_t count) {
    const auto size = data.size();
    data.resize(size + count);
    memcpy(data.data() + size, bytes, count);
}

const void* Parcel::getData() const {
    return data.data();
}

uint32_t Parcel::getSize() const {
    return data.size();
}

Parcel::Parcel(const void* data_, uint32_t size) {
    data.resize(size);
    memcpy(data.data(), data_, size);
}

uint32_t Parcel::getRemainingSize() const {
    return getSize() - cursor;
}
