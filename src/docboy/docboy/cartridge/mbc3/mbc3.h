#ifndef MBC3_H
#define MBC3_H

#include "docboy/cartridge/cartridge.h"
#include "docboy/cartridge/rtc/rtc.h"
#include "docboy/common/macros.h"

template <uint32_t RomSize, uint32_t RamSize, bool Battery, bool Timer>
class Mbc3 final : public ICartridge {
public:
    DEBUGGABLE_CLASS()

    Mbc3(const uint8_t* data, uint32_t length);

    uint8_t read_rom(uint16_t address) const override;
    void write_rom(uint16_t address, uint8_t value) override;

    uint8_t read_ram(uint16_t address) const override;
    void write_ram(uint16_t address, uint8_t value) override;

    void on_tick() override;

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
    static constexpr bool Ram = RamSize > 0;

    bool ram_enabled {};
    uint8_t rom_bank_selector {};
    uint8_t ram_bank_selector_rtc_register_selector {};

    struct {
        Rtc real {};
        RtcRegisters latched {};
    } rtc;

    uint8_t* const rtc_real_map[5] {};

    uint8_t rom[RomSize] {};
    uint8_t ram[RamSize] {};
};

#include "docboy/cartridge/mbc3/mbc3.tpp"

#endif // MBC3_H