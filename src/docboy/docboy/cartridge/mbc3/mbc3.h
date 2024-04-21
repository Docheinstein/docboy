#ifndef MBC3_H
#define MBC3_H

#include "docboy/cartridge/cartridge.h"
#include "docboy/cartridge/rtc/rtc.h"

template <uint32_t RomSize, uint32_t RamSize, bool Battery, bool Timer>
class Mbc3 final : public ICartridge {
public:
    DEBUGGABLE_CLASS()

    Mbc3(const uint8_t* data, uint32_t length);

    [[nodiscard]] uint8_t readRom(uint16_t address) const override;
    void writeRom(uint16_t address, uint8_t value) override;

    [[nodiscard]] uint8_t readRam(uint16_t address) const override;
    void writeRam(uint16_t address, uint8_t value) override;

    void tick() override;

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
    uint8_t romBankSelector {};
    uint8_t ramBankSelector_rtcRegisterSelector {};

    struct {
        Rtc real {};
        RtcRegisters latched {};
    } rtc;

    uint8_t* const rtcRealMap[5];

    uint8_t rom[RomSize] {};
    uint8_t ram[RamSize] {};
};

#include "mbc3.tpp"

#endif // MBC3_H