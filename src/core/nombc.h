#ifndef NOMBC_H
#define NOMBC_H

#include "cartridge.h"

class NoMBCCartridge : public Cartridge {
public:
    explicit NoMBCCartridge(const std::vector<uint8_t> &data);
    explicit NoMBCCartridge(std::vector<uint8_t> &&data);

    [[nodiscard]] uint8_t read(uint16_t address) const override;
    void write(uint16_t address, uint8_t value) override;
};

#endif // NOMBC_H