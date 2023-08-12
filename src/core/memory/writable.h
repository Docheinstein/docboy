#ifndef WRITABLEMEMORY_H
#define WRITABLEMEMORY_H

#include <cstdint>
#include <string>

class WriteMemoryException : public std::exception {
public:
    explicit WriteMemoryException(const std::string& what);
    [[nodiscard]] const char* what() const noexcept override;

private:
    std::string error;
};

class IWritableMemory {
public:
    virtual ~IWritableMemory() = default;
    virtual void write(uint16_t index, uint8_t value) = 0;
};

#endif // WRITABLEMEMORY_H