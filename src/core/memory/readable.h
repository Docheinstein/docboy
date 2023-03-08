#ifndef READABLE_H
#define READABLE_H

#include <cstdint>
#include <cstddef>
#include <vector>

class IReadable {
public:
    virtual ~IReadable() = default;
    [[nodiscard]] virtual uint8_t read(uint16_t index) const = 0;
};


class Readable : public virtual IReadable {
public:
    explicit Readable(size_t size);
    explicit Readable(const std::vector<uint8_t> &data);
    explicit Readable(std::vector<uint8_t> &&data);
    [[nodiscard]] uint8_t read(uint16_t index) const override;
private:
    std::vector<uint8_t> memory;
};


#endif // READABLE_H