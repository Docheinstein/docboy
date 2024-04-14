#ifndef ROMRAM_H
#define ROMRAM_H

#include "docboy/cartridge/cartridge.h"

template <uint32_t RomSize, uint32_t RamSize, bool Battery>
class RomRam final : public ICartridge {
public:
    DEBUGGABLE_CLASS()

    RomRam(const uint8_t* data, uint32_t length);

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
    static constexpr bool Ram = RamSize > 0;

    uint8_t rom[RomSize] {};
    uint8_t ram[RamSize] {};
};

#include "romram.tpp"

#endif // ROMRAM_H