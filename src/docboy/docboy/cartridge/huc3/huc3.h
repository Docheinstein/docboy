#ifndef HUC3_H
#define HUC3_H

#include "docboy/cartridge/cartridge.h"
#include "docboy/common/macros.h"

template <uint32_t RomSize, uint32_t RamSize>
class HuC3 final : public ICartridge {
public:
    DEBUGGABLE_CLASS()

    HuC3(const uint8_t* data, uint32_t length);

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
#endif

    void save_state(Parcel& parcel) const override;
    void load_state(Parcel& parcel) override;

    void reset() override;

private:
    void execute_rtc_command();

    enum class RamMapping : uint8_t {
        RamReadOnly = 0x00,
        RamReadWrite = 0x0A,
        RtcWrite = 0x0B,
        RtcRead = 0x0C,
        RtcSemaphore = 0x0D,
        IR = 0x0E,
    } ram_mapping {};

    struct Rtc {
        uint8_t command {};
        uint8_t argument {};
        uint8_t read {};
        uint8_t address {};
        uint32_t cycles_since_last_tick {};
#pragma pack(push, 1)
        // These fields must have no padding in between because we need
        // to serialize RTC as a unique consecutive chunk of data.
        int64_t last_tick_time {};
        uint8_t memory[256] {};
        struct {
            uint16_t minutes {};
            uint16_t days {};
        } clock;
#pragma pack(pop)
    } rtc {};

    void tick_rtc(int64_t delta);

    uint16_t rom_bank_selector {};
    uint8_t ram_bank_selector {};

    uint8_t rom[RomSize] {};
    uint8_t ram[RamSize] {};
};

#include "docboy/cartridge/huc3/huc3.tpp"

#endif // HUC3_H