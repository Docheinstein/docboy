#ifndef MBC3_H
#define MBC3_H

#include "docboy/cartridge/cartridge.h"

template <uint32_t RomSize, uint32_t RamSize, bool Battery, bool Timer>
class Mbc3 final : public ICartridge {
public:
    DEBUGGABLE_CLASS()

    Mbc3(const uint8_t* data, uint32_t length);

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

    bool ramAndTimerEnabled {};
    uint8_t romBankSelector {0b1}; // TODO: correct?
    uint8_t ramBankSelector_rtcRegisterSelector {};
    uint8_t latchClockData {};

    struct {
        uint8_t seconds;
        uint8_t minutes;
        uint8_t hours;
        struct {
            uint8_t low;
            uint8_t high;
        } day;
    } rtcRegisters {};

    uint8_t* const rtcRegistersMap[5];

    uint8_t rom[RomSize] {};
    uint8_t ram[RamSize] {};
};

#include "mbc3.tpp"

#endif // MBC3_H