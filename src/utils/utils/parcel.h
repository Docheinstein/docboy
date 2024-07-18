#ifndef UTILSPARCEL_H
#define UTILSPARCEL_H

#include <cstdint>
#include <vector>

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

    std::vector<uint8_t> data;
    uint32_t cursor {};
};

#endif // UTILSPARCEL_H