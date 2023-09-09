#ifndef NOMBC_H
#define NOMBC_H

#include "core/cartridge/cartridge.h"

class NoMBC : public Cartridge {
public:
    explicit NoMBC(const std::vector<uint8_t>& data);
    explicit NoMBC(std::vector<uint8_t>&& data);

    [[nodiscard]] uint8_t read(uint16_t address) const override;
    void write(uint16_t address, uint8_t value) override;
};

#endif // NOMBC_H