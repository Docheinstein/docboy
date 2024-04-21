#ifndef MBC5_H
#define MBC5_H

#include "docboy/cartridge/cartridge.h"

template <uint32_t RomSize, uint32_t RamSize, bool Battery>
class Mbc5 final : public ICartridge {
public:
    DEBUGGABLE_CLASS()

    Mbc5(const uint8_t* data, uint32_t length);

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

    void reset() override;

private:
    static constexpr bool Ram = RamSize > 0;

    bool ramEnabled {};
    uint16_t romBankSelector {};
    uint8_t ramBankSelector {};

    uint8_t rom[RomSize] {};
    uint8_t ram[RamSize] {};
};

#include "mbc5.tpp"

#endif // MBC5_H