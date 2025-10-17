#ifndef MBC1_H
#define MBC1_H

#include "docboy/cartridge/cartridge.h"
#include "docboy/common/macros.h"

template <uint32_t RomSize, uint32_t RamSize, bool Battery, uint8_t RomBankSelectorBits = 5>
class Mbc1 final : public ICartridge {
public:
    DEBUGGABLE_CLASS()

    Mbc1(const uint8_t* data, uint32_t length);

    uint8_t read_rom(uint16_t address) const override;
    void write_rom(uint16_t address, uint8_t value) override;

    uint8_t read_ram(uint16_t address) const override;
    void write_ram(uint16_t address, uint8_t value) override;

    void* get_ram_save_data() override;
    uint32_t get_ram_save_size() const override;

#ifdef ENABLE_DEBUGGER
    uint8_t* get_rom_data() override;
    uint32_t get_rom_size() const override;
#endif

    void save_state(Parcel& parcel) const override;
    void load_state(Parcel& parcel) override;

    void reset() override;

private:
    static constexpr bool Ram = RamSize > 0;

    bool ram_enabled {};
    uint8_t rom_bank_selector {};
    uint8_t upper_rom_bank_selector_ram_bank_selector {};
    uint8_t banking_mode {};

    uint8_t rom[RomSize] {};
    uint8_t ram[RamSize] {};
};

#include "docboy/cartridge/mbc1/mbc1.tpp"

#endif // MBC1_H