#ifndef MMM01_H
#define MMM01_H

#include "docboy/cartridge/cartridge.h"
#include "docboy/common/macros.h"

class MMM01_ : public ICartridge {
    DEBUGGABLE_CLASS()

protected:
    bool mapped {};

    bool ram_enabled {};

    bool mode_write_lock {};

    bool multiplex {};

    uint8_t banking_mode {};

    uint8_t rom_bank_low {};
    uint8_t rom_bank_mid {};
    uint8_t rom_bank_high {};
    uint8_t rom_bank_mask {};

    uint8_t ram_bank_low {};
    uint8_t ram_bank_high {};
    uint8_t ram_bank_mask {};
};

template <uint32_t RomSize, uint32_t RamSize, bool Battery>
class MMM01 final : public MMM01_ {
public:
    DEBUGGABLE_CLASS()

    MMM01(const uint8_t* data, uint32_t length);

    uint8_t read_rom(uint16_t address) const override;
    void write_rom(uint16_t address, uint8_t value) override;

    uint8_t read_ram(uint16_t address) const override;
    void write_ram(uint16_t address, uint8_t value) override;

    void* get_ram_save_data() override;
    uint32_t get_ram_save_size() const override;

#ifdef ENABLE_DEBUGGER
    CartridgeType get_type() const override;

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

    uint8_t rom[RomSize] {};
    uint8_t ram[RamSize] {};
};

#include "docboy/cartridge/mmm01/mmm01.tpp"

#endif // MMM01_H
