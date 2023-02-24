#ifndef MBC1_H
#define MBC1_H

#include "cartridge.h"
#include "memory.h"

class MBC1Cartridge : public Cartridge {
public:
    explicit MBC1Cartridge(const std::vector<uint8_t> &data);
    explicit MBC1Cartridge(std::vector<uint8_t> &&data);

    [[nodiscard]] uint8_t read(uint16_t address) const override;
    void write(uint16_t address, uint8_t value) override;

protected:
    struct {
        bool ramEnabled;
        uint8_t romBankSelector;
        uint8_t upperRomBankSelector_ramBankSelector;
        uint8_t bankingMode;
    } mbc;
};

#endif // MBC1_H