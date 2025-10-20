#include "utils/parcel.h"

#include <cstring>

#include "utils/asserts.h"

namespace {
#ifdef ENABLE_ASSERTS
enum Types : uint8_t {
    Bool = 0,
    Uint8 = 1,
    Uint16 = 2,
    Uint32 = 3,
    Uint64 = 4,
    Int8 = 5,
    Int16 = 6,
    Int32 = 7,
    Int64 = 8,
    Double = 9,
    String = 10,
    Bytes = 11,
};
#endif
} // namespace

bool Parcel::read_bool() {
#ifdef ENABLE_STATE_DEBUG_SYMBOLS
    skip_string();
#endif
#ifdef ENABLE_ASSERTS
    ASSERT(read<uint8_t>() == Bool);
#endif
    return read<bool>();
}

uint8_t Parcel::read_uint8() {
#ifdef ENABLE_STATE_DEBUG_SYMBOLS
    skip_string();
#endif
#ifdef ENABLE_ASSERTS
    ASSERT(read<uint8_t>() == Uint8);
#endif
    return read<uint8_t>();
}

uint16_t Parcel::read_uint16() {
#ifdef ENABLE_STATE_DEBUG_SYMBOLS
    skip_string();
#endif
#ifdef ENABLE_ASSERTS
    ASSERT(read<uint8_t>() == Uint16);
#endif
    return read<uint16_t>();
}

uint32_t Parcel::read_uint32() {
#ifdef ENABLE_STATE_DEBUG_SYMBOLS
    skip_string();
#endif
#ifdef ENABLE_ASSERTS
    ASSERT(read<uint8_t>() == Uint32);
#endif
    return read<uint32_t>();
}

uint64_t Parcel::read_uint64() {
#ifdef ENABLE_STATE_DEBUG_SYMBOLS
    skip_string();
#endif
#ifdef ENABLE_ASSERTS
    ASSERT(read<uint8_t>() == Uint64);
#endif
    return read<uint64_t>();
}

int8_t Parcel::read_int8() {
#ifdef ENABLE_STATE_DEBUG_SYMBOLS
    skip_string();
#endif
#ifdef ENABLE_ASSERTS
    ASSERT(read<uint8_t>() == Int8);
#endif
    return read<int8_t>();
}

int16_t Parcel::read_int16() {
#ifdef ENABLE_STATE_DEBUG_SYMBOLS
    skip_string();
#endif
#ifdef ENABLE_ASSERTS
    ASSERT(read<uint8_t>() == Int16);
#endif
    return read<int16_t>();
}

int32_t Parcel::read_int32() {
#ifdef ENABLE_STATE_DEBUG_SYMBOLS
    skip_string();
#endif
#ifdef ENABLE_ASSERTS
    ASSERT(read<uint8_t>() == Int32);
#endif
    return read<int32_t>();
}

int64_t Parcel::read_int64() {
#ifdef ENABLE_STATE_DEBUG_SYMBOLS
    skip_string();
#endif
#ifdef ENABLE_ASSERTS
    ASSERT(read<uint8_t>() == Int64);
#endif
    return read<int64_t>();
}

double Parcel::read_double() {
#ifdef ENABLE_STATE_DEBUG_SYMBOLS
    skip_string();
#endif
#ifdef ENABLE_ASSERTS
    ASSERT(read<uint8_t>() == Double);
#endif
    return read<double>();
}

void Parcel::read_bytes(void* data_, uint32_t count) {
#ifdef ENABLE_STATE_DEBUG_SYMBOLS
    skip_string();
#endif
#ifdef ENABLE_ASSERTS
    ASSERT(read<uint8_t>() == Bytes);
#endif
#ifdef ENABLE_STATE_DEBUG_SYMBOLS
    cursor += sizeof(uint32_t);
#endif
    read_raw(data_, count);
}

#ifdef ENABLE_STATE_DEBUG_SYMBOLS
void Parcel::skip_string() {
#ifdef ENABLE_ASSERTS
    ASSERT(read<uint8_t>() == String);
#endif
    cursor += read<uint32_t>();
}
#endif

void Parcel::write_bool(bool value) {
#ifdef ENABLE_ASSERTS
    write<uint8_t>(Bool);
#endif
    write(value);
}

void Parcel::write_uint8(uint8_t value) {
#ifdef ENABLE_ASSERTS
    write<uint8_t>(Uint8);
#endif
    write(value);
}

void Parcel::write_uint16(uint16_t value) {
#ifdef ENABLE_ASSERTS
    write<uint8_t>(Uint16);
#endif
    write(value);
}

void Parcel::write_uint32(uint32_t value) {
#ifdef ENABLE_ASSERTS
    write<uint8_t>(Uint32);
#endif
    write(value);
}

void Parcel::write_uint64(uint64_t value) {
#ifdef ENABLE_ASSERTS
    write<uint8_t>(Uint64);
#endif
    write(value);
}

void Parcel::write_int8(int8_t value) {
#ifdef ENABLE_ASSERTS
    write<uint8_t>(Int8);
#endif
    write(value);
}

void Parcel::write_int16(int16_t value) {
#ifdef ENABLE_ASSERTS
    write<uint8_t>(Int16);
#endif
    write(value);
}

void Parcel::write_int32(int32_t value) {
#ifdef ENABLE_ASSERTS
    write<uint8_t>(Int32);
#endif
    write(value);
}

void Parcel::write_int64(int64_t value) {
#ifdef ENABLE_ASSERTS
    write<uint8_t>(Int64);
#endif
    write(value);
}

void Parcel::write_double(double value) {
#ifdef ENABLE_ASSERTS
    write<uint8_t>(Double);
#endif
    write(value);
}

void Parcel::write_bytes(const void* data_, uint32_t count) {
#ifdef ENABLE_ASSERTS
    write<uint8_t>(Bytes);
#endif
#ifdef ENABLE_STATE_DEBUG_SYMBOLS
    write(count);
#endif
    write_raw(data_, count);
}

#ifdef ENABLE_STATE_DEBUG_SYMBOLS
void Parcel::write_string(const char* str) {
#ifdef ENABLE_ASSERTS
    write<uint8_t>(String);
#endif
    uint32_t len = strlen(str);
    write(len);
    write_raw(str, len);
}
#endif

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