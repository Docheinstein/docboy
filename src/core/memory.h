#ifndef MEMORY_H
#define MEMORY_H

#include <vector>
#include <cstdint>
#include <cstddef>
#include "utils/log.h"
#include <cstring>

class IReadable {
public:
    virtual ~IReadable() = default;
    [[nodiscard]] virtual uint8_t read(uint16_t index) const = 0;
};

class IWritable {
public:
    virtual ~IWritable() = default;
    virtual void write(uint16_t index, uint8_t value) = 0;
};

class IMemory : public IReadable, public IWritable {};

//class IMemory {
//public:
//    virtual ~IMemory() = default;
//    [[nodiscard]] virtual uint8_t read(uint16_t index) const = 0;
//    virtual void write(uint16_t index, uint8_t value) = 0;
//};

class Readable : public virtual IReadable {
public:
    explicit Readable(size_t size);
    explicit Readable(const std::vector<uint8_t> &data);
    explicit Readable(std::vector<uint8_t> &&data);
    [[nodiscard]] uint8_t read(uint16_t index) const override;
private:
    std::vector<uint8_t> memory;
};

class Memory : public virtual IMemory {
public:
    explicit Memory(size_t size);
    explicit Memory(const std::vector<uint8_t> &data);
    explicit Memory(std::vector<uint8_t> &&data);

    [[nodiscard]] uint8_t read(uint16_t index) const override;
    void write(uint16_t index, uint8_t value) override;

protected:
    std::vector<uint8_t> memory;
};


#endif // MEMORY_H