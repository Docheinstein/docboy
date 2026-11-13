#ifndef MBC7_H
#define MBC7_H

#include "docboy/cartridge/cartridge.h"
#include "docboy/common/macros.h"

struct Mbc7_ : ICartridge {
    struct Eeprom {
        static constexpr uint16_t MaxEepromSize = 512;
        static constexpr uint16_t MaxAddressSpaceSize = MaxEepromSize / sizeof(uint16_t);

        template <uint8_t DataSize>
        struct Register {
            static constexpr uint8_t Size = DataSize;

            uint16_t data {};
            uint8_t cursor {};

            void set_next_bit(bool b);
            bool get_next_bit();
            bool is_full() const;
        };

        bool data_out {};
        bool data_in {};
        bool clk {};
        bool cs {};

        bool erase_write_enabled {};

        enum class State : uint8_t {
            Idle,         // Waiting for new instruction
            Instruction,  // Waiting for 10 bits of instruction
            Input,        // Waiting for 16 bits of instruction parameter
            Output,       // Emitting 16 bits of output
            PendingWrite, // Pending erase/write command, waiting for CS low
            Done          // Instruction executed
        } state {};

        enum class Command : uint8_t { EWEN, EWDS, READ, WRITE, ERASE, WRAL, ERAL } command {};

        Register<10> instruction {};
        Register<16> input {};
        Register<16> output {};

        uint16_t data[MaxAddressSpaceSize] {};
    };

    struct Accelerometer {
        struct {
            struct {
                uint8_t low {};
                uint8_t high {};
            } x;
            struct {
                uint8_t low {};
                uint8_t high {};
            } y;
        } latch;

        struct {
            double x {};
            double y {};
        } sensor;

        bool latched {};
    };

    bool ram_enabled_primary {};
    bool ram_enabled_secondary {};

    uint8_t rom_bank_selector {};

    Accelerometer accelerometer {};
    Eeprom eeprom {};
};

template <uint32_t RomSize, uint16_t EepromSize>
class Mbc7 final : public Mbc7_ {
public:
    DEBUGGABLE_CLASS()

    Mbc7(const uint8_t* data, uint32_t length);

    uint8_t read_rom(uint16_t address) const override;
    void write_rom(uint16_t address, uint8_t value) override;

    uint8_t read_ram(uint16_t address) const override;
    void write_ram(uint16_t address, uint8_t value) override;

    void* get_ram_save_data() override;
    uint32_t get_ram_save_size() const override;

    void set_accelerometer(double x, double y) override;

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
    static_assert(EepromSize <= Eeprom::MaxEepromSize);

    static constexpr uint16_t EepromAddressSpaceSize = EepromSize / sizeof(uint16_t);
    static constexpr uint16_t EepromAddressSpaceBits = log_2<EepromAddressSpaceSize>;

    static Eeprom::Command parse_eeprom_command(uint16_t value);

    void handle_eeprom_instruction();
    void handle_eeprom_parametric_instruction();
    void handle_eeprom_pending_write();

    uint8_t rom[RomSize] {};
};

#include "docboy/cartridge/mbc7/mbc7.tpp"

#endif // MBC7_H