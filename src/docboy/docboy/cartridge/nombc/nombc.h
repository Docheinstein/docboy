#ifndef NOMBC_H
#define NOMBC_H

#include "docboy/cartridge/cartridge.h"

template <uint32_t RamSize>
class NoMbc final : public ICartridge {
public:
    explicit NoMbc(const uint8_t* data, uint32_t length);

    uint8_t read_rom(uint16_t address) const override;
    void write_rom(uint16_t address, uint8_t value) override;

    uint8_t read_ram(uint16_t address) const override;
    void write_ram(uint16_t address, uint8_t value) override;

    uint8_t* get_ram_save_data() override;
    uint32_t get_ram_save_size() const override;

#ifdef ENABLE_DEBUGGER
    uint8_t* get_rom_data() override;
    uint32_t get_rom_size() const override;
#endif

    void save_state(Parcel& parcel) const override;
    void load_state(Parcel& parcel) override;

    void reset() override;

private:
    static constexpr uint16_t RomSize = 0x8000;
    static constexpr bool Ram = RamSize > 0;

    uint8_t rom[RomSize] {};
    uint8_t ram[RamSize] {};
};

#include "docboy/cartridge/nombc/nombc.tpp"

#endif // NOMBC_H