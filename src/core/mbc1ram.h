#ifndef MBC1RAM_H
#define MBC1RAM_H

#include "mbc1.h"
#include "memory.h"


class MBC1RamCartridge : public MBC1Cartridge {
public:
    explicit MBC1RamCartridge(const std::vector<uint8_t> &data);
    explicit MBC1RamCartridge(std::vector<uint8_t> &&data);

    [[nodiscard]] uint8_t read(uint16_t address) const override;
    void write(uint16_t address, uint8_t value) override;

protected:
    Memory<0x8000> ram;
};


#endif // MBC1RAM_H