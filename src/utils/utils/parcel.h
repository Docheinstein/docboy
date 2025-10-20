#ifndef UTILSPARCEL_H
#define UTILSPARCEL_H

#include <cstdint>
#include <vector>

// TODO: use std::source_location instead of MACROs when upgrading to C++20.

#ifdef ENABLE_STATE_DEBUG_SYMBOLS
#define PARCEL_WRITE_BOOL(parcel, value)                                                                               \
    do {                                                                                                               \
        (parcel).write_string(__FILE__ ":" #value);                                                                    \
        (parcel).write_bool(value);                                                                                    \
    } while (0)
#define PARCEL_WRITE_UINT8(parcel, value)                                                                              \
    do {                                                                                                               \
        (parcel).write_string(__FILE__ ":" #value);                                                                    \
        (parcel).write_uint8(value);                                                                                   \
    } while (0)
#define PARCEL_WRITE_UINT16(parcel, value)                                                                             \
    do {                                                                                                               \
        (parcel).write_string(__FILE__ ":" #value);                                                                    \
        (parcel).write_uint16(value);                                                                                  \
    } while (0)
#define PARCEL_WRITE_UINT32(parcel, value)                                                                             \
    do {                                                                                                               \
        (parcel).write_string(__FILE__ ":" #value);                                                                    \
        (parcel).write_uint32(value);                                                                                  \
    } while (0)
#define PARCEL_WRITE_UINT64(parcel, value)                                                                             \
    do {                                                                                                               \
        (parcel).write_string(__FILE__ ":" #value);                                                                    \
        (parcel).write_uint64(value);                                                                                  \
    } while (0)
#define PARCEL_WRITE_INT8(parcel, value)                                                                               \
    do {                                                                                                               \
        (parcel).write_string(__FILE__ ":" #value);                                                                    \
        (parcel).write_int8(value);                                                                                    \
    } while (0)
#define PARCEL_WRITE_INT16(parcel, value)                                                                              \
    do {                                                                                                               \
        (parcel).write_string(__FILE__ ":" #value);                                                                    \
        (parcel).write_int16(value);                                                                                   \
    } while (0)
#define PARCEL_WRITE_INT32(parcel, value)                                                                              \
    do {                                                                                                               \
        (parcel).write_string(__FILE__ ":" #value);                                                                    \
        (parcel).write_int32(value);                                                                                   \
    } while (0)
#define PARCEL_WRITE_INT64(parcel, value)                                                                              \
    do {                                                                                                               \
        (parcel).write_string(__FILE__ ":" #value);                                                                    \
        (parcel).write_int64(value);                                                                                   \
    } while (0)
#define PARCEL_WRITE_DOUBLE(parcel, value)                                                                             \
    do {                                                                                                               \
        (parcel).write_string(__FILE__ ":" #value);                                                                    \
        (parcel).write_double(value);                                                                                  \
    } while (0)
#define PARCEL_WRITE_STRING(parcel, value)                                                                             \
    do {                                                                                                               \
        (parcel).write_string(__FILE__ ":" #value);                                                                    \
        (parcel).write_string(value);                                                                                  \
    } while (0)
#define PARCEL_WRITE_BYTES(parcel, data, count)                                                                        \
    do {                                                                                                               \
        (parcel).write_string(__FILE__ ":" #data ":" #count);                                                          \
        (parcel).write_bytes(data, count);                                                                             \
    } while (0)
#else
#define PARCEL_WRITE_BOOL(parcel, value) (parcel).write_bool(value)
#define PARCEL_WRITE_UINT8(parcel, value) (parcel).write_uint8(value)
#define PARCEL_WRITE_UINT16(parcel, value) (parcel).write_uint16(value)
#define PARCEL_WRITE_UINT32(parcel, value) (parcel).write_uint32(value)
#define PARCEL_WRITE_UINT64(parcel, value) (parcel).write_uint64(value)
#define PARCEL_WRITE_INT8(parcel, value) (parcel).write_int8(value)
#define PARCEL_WRITE_INT16(parcel, value) (parcel).write_int16(value)
#define PARCEL_WRITE_INT32(parcel, value) (parcel).write_int32(value)
#define PARCEL_WRITE_INT64(parcel, value) (parcel).write_int64(value)
#define PARCEL_WRITE_DOUBLE(parcel, value) (parcel).write_double(value)
#define PARCEL_WRITE_BYTES(parcel, data, count) (parcel).write_bytes(data, count)
#define PARCEL_WRITE_STRING(parcel, value) (parcel).write_string(value)
#endif

class Parcel {
public:
    Parcel() = default;

    explicit Parcel(const void* data, uint32_t size);

    bool read_bool();

    uint8_t read_uint8();
    uint16_t read_uint16();
    uint32_t read_uint32();
    uint64_t read_uint64();

    int8_t read_int8();
    int16_t read_int16();
    int32_t read_int32();
    int64_t read_int64();

    double read_double();

    void read_bytes(void* data, uint32_t count);

    void write_bool(bool value);

    void write_uint8(uint8_t value);
    void write_uint16(uint16_t value);
    void write_uint32(uint32_t value);
    void write_uint64(uint64_t value);

    void write_int8(int8_t value);
    void write_int16(int16_t value);
    void write_int32(int32_t value);
    void write_int64(int64_t value);

    void write_double(double value);

    void write_bytes(const void* data, uint32_t count);

#ifdef ENABLE_STATE_DEBUG_SYMBOLS
    void write_string(const char* data);
#endif

    const void* get_data() const;
    uint32_t get_size() const;
    uint32_t get_remaining_size() const;

private:
    template <typename T>
    T read();

    template <typename T>
    void write(T value);

    void read_raw(void* data, uint32_t count);
    void write_raw(const void* data, uint32_t count);

#ifdef ENABLE_STATE_DEBUG_SYMBOLS
    void skip_string();
#endif

    std::vector<uint8_t> data;
    uint32_t cursor {};
};

#endif // UTILSPARCEL_H