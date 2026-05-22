#ifndef MBC3_H
#define MBC3_H

#include "docboy/cartridge/cartridge.h"
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

    void* get_ram_save_data() override;
    uint32_t get_ram_save_size() const override;

    void* get_rtc_save_data() override;
    uint32_t get_rtc_save_size() const override;

#ifdef ENABLE_DEBUGGER
    uint8_t* get_rom_data() override;
    uint32_t get_rom_size() const override;

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
    static constexpr uint8_t MaxRamBankSelector = RamSize < (64 * 1024) ? 0x04 : 0x08;

    struct RtcRegisters {
        uint8_t seconds {};
        uint8_t minutes {};
        uint8_t hours {};
        uint8_t days_low {};
        uint8_t days_high {};
    };

    void tick_rtc(int64_t delta);

    bool ram_enabled {};
    uint8_t rom_bank_selector {};
    uint8_t ram_bank_selector_rtc_register_selector {};

    struct Rtc {
        uint32_t cycles_since_last_tick {};
#pragma pack(push, 1)
        // These fields must have no padding in between because we need
        // to serialize RTC as a unique consecutive chunk of data.
        int64_t last_tick_time {};
        RtcRegisters clock {};
#pragma pack(pop)
        RtcRegisters latched {};
        uint8_t latch {};
    } rtc;

    uint8_t rom[RomSize] {};
    uint8_t ram[RamSize] {};
};

#include "docboy/cartridge/mbc3/mbc3.tpp"

#endif // MBC3_H