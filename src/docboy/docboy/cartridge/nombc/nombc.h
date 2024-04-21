#ifndef NOMBC_H
#define NOMBC_H

#include "docboy/cartridge/cartridge.h"

template <uint32_t RamSize>
class NoMbc final : public ICartridge {
public:
    explicit NoMbc(const uint8_t* data, uint32_t length);

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
    static constexpr uint16_t RomSize = 0x8000;
    static constexpr bool Ram = RamSize > 0;

    uint8_t rom[RomSize] {};
    uint8_t ram[RamSize] {};
};

#include "nombc.tpp"

#endif // NOMBC_H