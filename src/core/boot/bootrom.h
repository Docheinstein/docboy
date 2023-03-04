#ifndef BOOTROM_H
#define BOOTROM_H

#include <vector>
#include "core/memory.h"

typedef IReadable IBootROM;

class BootROM : public IBootROM {
public:
    explicit BootROM(const std::vector<uint8_t> &data);
    explicit BootROM(std::vector<uint8_t> &&data);

    [[nodiscard]] uint8_t read(uint16_t index) const override;

    ~BootROM() override = default;

protected:
    std::vector<uint8_t> rom;
};

#endif // BOOTROM_H