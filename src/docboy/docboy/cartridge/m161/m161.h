#ifndef M161_H
#define M161_H

#include "docboy/cartridge/cartridge.h"
#include "docboy/common/macros.h"

class M161_ : public ICartridge {
    DEBUGGABLE_CLASS()

protected:
    uint8_t rom_bank_selector {};
    bool locked {};
};

template <uint32_t RomSize>
class M161 final : public M161_ {
public:
    DEBUGGABLE_CLASS()

    M161(const uint8_t* data, uint32_t length);

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
    uint8_t rom[RomSize] {};
};

#include "docboy/cartridge/m161/m161.tpp"

#endif // M161_H
