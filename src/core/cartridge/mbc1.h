#ifndef MBC1_H
#define MBC1_H

#include "cartridge.h"
#include "memory.h"

class MBC1 : public Cartridge {
public:
    explicit MBC1(const std::vector<uint8_t> &data);
    explicit MBC1(std::vector<uint8_t> &&data);

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