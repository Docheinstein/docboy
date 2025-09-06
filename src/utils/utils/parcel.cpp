#include "utils/parcel.h"

#include <cstring>

#include "utils/asserts.h"

#ifdef ENABLE_ASSERTS
namespace {
enum Types : uint8_t {
    Bool = 0,
    Uint8,
    Uint16,
    Uint32,
    Uint64,
    Int8,
    Int16,
    Int32,
    Int64,
    Double,
    Bytes,
};
}
#endif

bool Parcel::read_bool() {
    ASSERT(read<uint8_t>() == Types::Bool);
    return read<bool>();
}

uint8_t Parcel::read_uint8() {
    ASSERT(read<uint8_t>() == Types::Uint8);
    return read<uint8_t>();
}

uint16_t Parcel::read_uint16() {
    ASSERT(read<uint8_t>() == Types::Uint16);
    return read<uint16_t>();
}

uint32_t Parcel::read_uint32() {
    ASSERT(read<uint8_t>() == Types::Uint32);
    return read<uint32_t>();
}

uint64_t Parcel::read_uint64() {
    ASSERT(read<uint8_t>() == Types::Uint64);
    return read<uint64_t>();
}

int8_t Parcel::read_int8() {
    ASSERT(read<uint8_t>() == Types::Int8);
    return read<int8_t>();
}

int16_t Parcel::read_int16() {
    ASSERT(read<uint8_t>() == Types::Int16);
    return read<int16_t>();
}

int32_t Parcel::read_int32() {
    ASSERT(read<uint8_t>() == Types::Int32);
    return read<int32_t>();
}

int64_t Parcel::read_int64() {
    ASSERT(read<uint8_t>() == Types::Int64);
    return read<int64_t>();
}

double Parcel::read_double() {
    ASSERT(read<uint8_t>() == Types::Double);
    return read<double>();
}

void Parcel::read_bytes(void* data_, uint32_t count) {
    ASSERT(read<uint8_t>() == Types::Bytes);
    read_raw(data_, count);
}

void Parcel::write_bool(bool value) {
#ifdef ENABLE_ASSERTS
    write<uint8_t>(Types::Bool);
#endif
    write(value);
}

void Parcel::write_uint8(uint8_t value) {
#ifdef ENABLE_ASSERTS
    write<uint8_t>(Types::Uint8);
#endif
    write(value);
}

void Parcel::write_uint16(uint16_t value) {
#ifdef ENABLE_ASSERTS
    write<uint8_t>(Types::Uint16);
#endif
    write(value);
}

void Parcel::write_uint32(uint32_t value) {
#ifdef ENABLE_ASSERTS
    write<uint8_t>(Types::Uint32);
#endif
    write(value);
}

void Parcel::write_uint64(uint64_t value) {
#ifdef ENABLE_ASSERTS
    write<uint8_t>(Types::Uint64);
#endif
    write(value);
}

void Parcel::write_int8(int8_t value) {
#ifdef ENABLE_ASSERTS
    write<uint8_t>(Types::Int8);
#endif
    write(value);
}

void Parcel::write_int16(int16_t value) {
#ifdef ENABLE_ASSERTS
    write<uint8_t>(Types::Int16);
#endif
    write(value);
}

void Parcel::write_int32(int32_t value) {
#ifdef ENABLE_ASSERTS
    write<uint8_t>(Types::Int32);
#endif
    write(value);
}

void Parcel::write_int64(int64_t value) {
#ifdef ENABLE_ASSERTS
    write<uint8_t>(Types::Int64);
#endif
    write(value);
}

void Parcel::write_double(double value) {
#ifdef ENABLE_ASSERTS
    write<uint8_t>(Types::Double);
#endif
    write(value);
}

void Parcel::write_bytes(const void* data_, uint32_t count) {
#ifdef ENABLE_ASSERTS
    write<uint8_t>(Types::Bytes);
#endif
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