#ifndef MBC2_H
#define MBC2_H

#include "docboy/cartridge/cartridge.h"

template <uint32_t RomSize, bool Battery>
class Mbc2 final : public ICartridge {
public:
    DEBUGGABLE_CLASS()

    Mbc2(const uint8_t* data, uint32_t length);

    [[nodiscard]] uint8_t readRom(uint16_t address) const override;
    void writeRom(uint16_t address, uint8_t value) override;

    [[nodiscard]] uint8_t readRam(uint16_t address) const override;
    void writeRam(uint16_t address, uint8_t value) override;

    [[nodiscard]] uint8_t* getRamSaveData() override;
    [[nodiscard]] uint32_t getRamSaveSize() const override;

    IF_DEBUGGER(uint8_t* getRomData() override);
    IF_DEBUGGER(uint32_t getRomSize() const override);

    void saveState(Parcel& parcel) const override;
    void loadState(Parcel& parcel) override;

private:
    static constexpr uint16_t RamSize {512};

    bool ramEnabled {};
    uint8_t romBankSelector {0b1};

    uint8_t rom[RomSize] {};
    uint8_t ram[RamSize] {};
};

#include "mbc2.tpp"

#endif // MBC2_H