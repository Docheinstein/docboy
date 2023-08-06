#ifndef READABLE_H
#define READABLE_H

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

class ReadMemoryException : public std::exception {
public:
    explicit ReadMemoryException(const std::string& what);
    [[nodiscard]] const char* what() const noexcept override;

private:
    std::string error;
};

class IReadableMemory {
public:
    virtual ~IReadableMemory() = default;
    [[nodiscard]] virtual uint8_t read(uint16_t index) const = 0;
};

class ReadOnlyMemory : public IReadableMemory {
public:
    explicit ReadOnlyMemory(size_t size);
    explicit ReadOnlyMemory(const std::vector<uint8_t>& data);
    explicit ReadOnlyMemory(std::vector<uint8_t>&& data);
    [[nodiscard]] uint8_t read(uint16_t index) const override;

private:
    std::vector<uint8_t> memory;
};

#endif // READABLE_H