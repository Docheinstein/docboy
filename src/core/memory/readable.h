#ifndef READABLE_H
#define READABLE_H

#include <cstdint>
#include <cstddef>
#include <vector>

class IReadableMemory {
public:
    virtual ~IReadableMemory() = default;
    [[nodiscard]] virtual uint8_t read(uint16_t index) const = 0;
};


class ReadOnlyMemory : public IReadableMemory {
public:
    explicit ReadOnlyMemory(size_t size);
    explicit ReadOnlyMemory(const std::vector<uint8_t> &data);
    explicit ReadOnlyMemory(std::vector<uint8_t> &&data);
    [[nodiscard]] uint8_t read(uint16_t index) const override;
private:
    std::vector<uint8_t> memory;
};


#endif // READABLE_H