#ifndef MBC1_H
#define MBC1_H

#include "docboy/cartridge/cartridge.h"

template <uint32_t RomSize, uint32_t RamSize, bool Battery>
class Mbc1 final : public ICartridge {
public:
    Mbc1(const uint8_t* data, uint32_t length);

    [[nodiscard]] uint8_t readRom(uint16_t address) const override;
    void writeRom(uint16_t address, uint8_t value) override;

    [[nodiscard]] uint8_t readRam(uint16_t address) const override;
    void writeRam(uint16_t address, uint8_t value) override;

    [[nodiscard]] uint8_t* getRamSaveData() override;
    [[nodiscard]] uint32_t getRamSaveSize() const override;

    void saveState(Parcel& parcel) const override;
    void loadState(Parcel& parcel) override;

private:
    static constexpr bool Ram = RamSize > 0;

    bool ramEnabled {};
    uint8_t romBankSelector {0b1};
    uint8_t upperRomBankSelector_ramBankSelector {};
    uint8_t bankingMode {};

    uint8_t rom[RomSize] {};
    uint8_t ram[RamSize] {};
};

#include "mbc1.tpp"

#endif // MBC1_H