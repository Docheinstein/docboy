#ifndef MMM01_H
#define MMM01_H

#include "docboy/cartridge/cartridge.h"
#include "docboy/common/macros.h"

template <uint32_t RomSize, uint32_t RamSize, bool Battery>
class Mmm01 final : public ICartridge {
public:
    DEBUGGABLE_CLASS()

    Mmm01(const uint8_t* data, uint32_t length);

    uint8_t read_rom(uint16_t address) const override;
    void write_rom(uint16_t address, uint8_t value) override;

    uint8_t read_ram(uint16_t address) const override;
    void write_ram(uint16_t address, uint8_t value) override;

    void* get_ram_save_data() override;
    uint32_t get_ram_save_size() const override;

#ifdef ENABLE_DEBUGGER
    uint8_t* get_rom_data() override;
    uint32_t get_rom_size() const override;

    uint8_t* get_ram_data() override;
    uint32_t get_ram_size() const override;

    uint8_t read_rom_raw(uint16_t bank, uint16_t address) const override;
    uint16_t get_rom_bank(uint16_t address) const override;

    uint8_t read_ram_raw(uint16_t bank, uint16_t address) const override;
    uint16_t get_ram_bank(uint16_t address) const override;
#endif

    void save_state(Parcel& parcel) const override;
    void load_state(Parcel& parcel) override;

    void reset() override;

private:
    static constexpr bool Ram = RamSize > 0;

    uint16_t get_rom_bank_0000_3FFF() const;
    uint16_t get_rom_bank_4000_7FFF() const;
    uint8_t get_ram_bank_A000_BFFF() const;

    // 0x0000 - 0x1FFF
    bool ram_enabled {};
    uint8_t ram_bank_mask {}; // 2 bits
    bool mapped {};

    // 0x2000 - 0x3FFF
    uint8_t rom_bank_low {}; // 5 bits
    uint8_t rom_bank_mid {}; // 2 bits

    // 0x4000 - 0x5FFF
    uint8_t ram_bank_low {};  // 2 bits
    uint8_t ram_bank_high {}; // 2 bits
    uint8_t rom_bank_high {}; // 2 bits
    bool mode_write_lock {};

    // 0x6000 - 0x7FFF
    uint8_t banking_mode {};
    uint8_t rom_bank_mask {}; // 4 bits (locks bits 4:1 of rom_bank_low)
    bool multiplex {};

    uint8_t rom[RomSize] {};
    uint8_t ram[RamSize] {};
};

#include "docboy/cartridge/mmm01/mmm01.tpp"

#endif // MMM01_H
