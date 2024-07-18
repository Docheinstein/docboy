#include "utils/parcel.h"

#include <cstring>

#include "utils/asserts.h"

bool Parcel::read_bool() {
    return read<bool>();
}

uint8_t Parcel::read_uint8() {
    return read<uint8_t>();
}

uint16_t Parcel::read_uint16() {
    return read<uint16_t>();
}

uint32_t Parcel::read_uint32() {
    return read<uint32_t>();
}

uint64_t Parcel::read_uint64() {
    return read<uint64_t>();
}

int8_t Parcel::read_int8() {
    return read<int8_t>();
}

int16_t Parcel::read_int16() {
    return read<int16_t>();
}

int32_t Parcel::read_int32() {
    return read<int32_t>();
}

int64_t Parcel::read_int64() {
    return read<int64_t>();
}

double Parcel::read_double() {
    return read<double>();
}

void Parcel::read_bytes(void* data_, uint32_t count) {
    read_raw(data_, count);
}

void Parcel::write_bool(bool value) {
    write(value);
}

void Parcel::write_uint8(uint8_t value) {
    write(value);
}

void Parcel::write_uint16(uint16_t value) {
    write(value);
}

void Parcel::write_uint32(uint32_t value) {
    write(value);
}

void Parcel::write_uint64(uint64_t value) {
    write(value);
}

void Parcel::write_int8(int8_t value) {
    write(value);
}

void Parcel::write_int16(int16_t value) {
    write(value);
}

void Parcel::write_int32(int32_t value) {
    write(value);
}

void Parcel::write_int64(int64_t value) {
    write(value);
}

void Parcel::write_double(double value) {
    write(value);
}

void Parcel::write_bytes(const void* data_, uint32_t count) {
    write_raw(data_, count);
}

template <typename T>
T Parcel::read() {
    T result;
    read_raw(&result, sizeof(T));
    return result;
}

template <typename T>
void Parcel::write(T value) {
    write_raw(&value, sizeof(T));
}

void Parcel::read_raw(void* result, uint32_t count) {
    ASSERT(get_remaining_size() > 0);
    memcpy(result, data.data() + cursor, count);
    cursor += count;
}

void Parcel::write_raw(const void* bytes, uint32_t count) {
    const auto size = data.size();
    data.resize(size + count);
    memcpy(data.data() + size, bytes, count);
}

const void* Parcel::get_data() const {
    return data.data();
}

uint32_t Parcel::get_size() const {
    return data.size();
}

Parcel::Parcel(const void* data_, uint32_t size) {
    data.resize(size);
    memcpy(data.data(), data_, size);
}

uint32_t Parcel::get_remaining_size() const {
    return get_size() - cursor;
}