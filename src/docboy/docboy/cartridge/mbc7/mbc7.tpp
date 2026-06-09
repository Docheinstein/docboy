#include <cmath>
#include <cstring>

#include "utils/arrays.h"
#include "utils/asserts.h"
#include "utils/bits.h"
#include "utils/exceptions.h"
#include "utils/parcel.h"

template <uint32_t RomSize, uint16_t EepromSize>
template <uint8_t DataSize>
void Mbc7<RomSize, EepromSize>::Eeprom::Register<DataSize>::set_next_bit(bool b) {
    set_bit(data, DataSize - 1 - cursor, b);
    ++cursor;
}

template <uint32_t RomSize, uint16_t EepromSize>
template <uint8_t DataSize>
bool Mbc7<RomSize, EepromSize>::Eeprom::Register<DataSize>::get_next_bit() {
    bool b = get_bit(data, DataSize - 1 - cursor);
    ++cursor;
    return b;
}

template <uint32_t RomSize, uint16_t EepromSize>
template <uint8_t DataSize>
bool Mbc7<RomSize, EepromSize>::Eeprom::Register<DataSize>::is_full() const {
    return cursor == DataSize;
}

template <uint32_t RomSize, uint16_t EepromSize>
Mbc7<RomSize, EepromSize>::Mbc7(const uint8_t* data, uint32_t length) {
    ASSERT(length <= array_size(rom), "Mbc7: actual ROM size (" + std::to_string(length) +
                                          ") exceeds nominal ROM size (" + std::to_string(array_size(rom)) + ")");
    memcpy(rom, data, length);

    peripherals.accelerometer = true;
}

template <uint32_t RomSize, uint16_t EepromSize>
uint8_t Mbc7<RomSize, EepromSize>::read_rom(uint16_t address) const {
    ASSERT(address < 0x8000);

    // 0000 - 0x3FFF
    if (address < 0x4000) {
        return rom[address];
    }

    // 4000 - 0x7FFF
    uint32_t rom_address = rom_bank_selector << 14 | keep_bits<14>(address);
    rom_address = mask_by_pow2<RomSize>(rom_address);
    return rom[rom_address];
}

template <uint32_t RomSize, uint16_t EepromSize>
void Mbc7<RomSize, EepromSize>::write_rom(uint16_t address, uint8_t value) {
    if (address < 0x4000) {
        // 0000 - 0x1FFF
        if (address < 0x2000) {
            ram_enabled_primary = keep_bits<4>(value) == 0xA;
            return;
        }

        // 0x2000 - 0x3FFF
        rom_bank_selector = value;
        return;
    }

    if (address < 0x6000) {
        // 0x4000 - 0x5FFF
        ram_enabled_secondary = value == 0x40;
    }
}

template <uint32_t RomSize, uint16_t EepromSize>
uint8_t Mbc7<RomSize, EepromSize>::read_ram(uint16_t address) const {
    ASSERT(address >= 0xA000 && address < 0xC000);

    // 0xA000 - 0xBFFF
    if (ram_enabled_primary && ram_enabled_secondary) {
        uint8_t ram_register = keep_bits_range<7, 4>(address);
        switch (ram_register) {
        case 0x20:
            return accelerometer.latch.x.low;
        case 0x30:
            return accelerometer.latch.x.high;
        case 0x40:
            return accelerometer.latch.y.low;
        case 0x50:
            return accelerometer.latch.y.high;
        case 0x60:
            return 0x00;
        case 0x70:
            return 0xFF;
        case 0x80:
            return eeprom.cs << Specs::Bits::Mbc::Mbc7::Eeprom::CHIP_SELECT |
                   eeprom.clk << Specs::Bits::Mbc::Mbc7::Eeprom::CHIP_SELECT |
                   eeprom.data_in << Specs::Bits::Mbc::Mbc7::Eeprom::DATA_IN |
                   eeprom.data_out << Specs::Bits::Mbc::Mbc7::Eeprom::DATA_OUT;
        default:
            return 0xFF;
        }
    }

    return 0xFF;
}

template <uint32_t RomSize, uint16_t EepromSize>
void Mbc7<RomSize, EepromSize>::write_ram(uint16_t address, uint8_t value) {
    ASSERT(address >= 0xA000 && address < 0xC000);

    // 0xA000 - 0xBFFF
    if (ram_enabled_primary && ram_enabled_secondary) {
        uint8_t ram_register = keep_bits_range<7, 4>(address);
        switch (ram_register) {
        case 0x00:
            if (value == 0x55) {
                // Reset accelerometer latch.
                accelerometer.latched = false;

                accelerometer.latch.x.low = 0x00;
                accelerometer.latch.x.high = 0x80;
                accelerometer.latch.y.low = 0x00;
                accelerometer.latch.y.high = 0x80;
            }
            break;
        case 0x10:
            // Copy accelerometer sensor values into latch.
            // Note: copy won't happen if the accelerometer hasn't been reset previously.
            if (value == 0xAA && !accelerometer.latched) {
                accelerometer.latched = true;

                auto accel_x = static_cast<uint16_t>(0x81D0 + 112.0 * accelerometer.sensor.x);
                auto accel_y = static_cast<uint16_t>(0x81D0 + 112.0 * accelerometer.sensor.y);

                accelerometer.latch.x.low = keep_bits_range<7, 0>(accel_x);
                accelerometer.latch.x.high = get_bits_range<15, 8>(accel_x);
                accelerometer.latch.y.low = keep_bits_range<7, 0>(accel_y);
                accelerometer.latch.y.high = get_bits_range<15, 8>(accel_y);
            }
            break;
        case 0x80: {
            std::cout << "EEPROM(A080): " << hex(value) << std::endl;

            // EEPROM register write.

            bool data_in = test_bit<Specs::Bits::Mbc::Mbc7::Eeprom::DATA_IN>(value);
            bool clk = test_bit<Specs::Bits::Mbc::Mbc7::Eeprom::CLOCK>(value);
            bool cs = test_bit<Specs::Bits::Mbc::Mbc7::Eeprom::CHIP_SELECT>(value);

            bool rising_clock = !eeprom.clk && clk;
            bool falling_cs = eeprom.cs && !cs;

            if (rising_clock) {
                if (eeprom.state == Eeprom::State::Idle && data_in && cs) {
                    std::cout << "new_instruction: " << std::endl;

                    // Start a new instruction.
                    eeprom.state = Eeprom::State::Instruction;
                    eeprom.instruction.cursor = 0;
                } else if (eeprom.state == Eeprom::State::Instruction) {
                    // Receive a bit for the current instruction.
                    eeprom.instruction.set_next_bit(data_in);

                    if (eeprom.instruction.is_full()) {
                        eeprom.command = parse_eeprom_command(eeprom.instruction.data);
                        handle_eeprom_instruction();
                    }
                } else if (eeprom.state == Eeprom::State::Input) {
                    // Receive a bit for the current instruction parameter.
                    eeprom.input.set_next_bit(data_in);

                    if (eeprom.input.is_full()) {
                        handle_eeprom_parametric_instruction();
                    }
                } else if (eeprom.state == Eeprom::State::Output) {
                    // Emit a bit for the current read instruction.
                    eeprom.data_out = eeprom.output.get_next_bit();

                    if (eeprom.output.is_full()) {
                        eeprom.state = Eeprom::State::Done;
                    }
                }
            }

            /* TODO: else if? */ if (falling_cs) {
                if (eeprom.state == Eeprom::State::Done || eeprom.state == Eeprom::State::Idle) {
                    // No-op.
                } else if (eeprom.state == Eeprom::State::PendingWrite) {
                    std::cout << "handle_eeprom_pending_write: " << uint8_t(eeprom.command) << std::endl;

                    // Actually write to EEPROM as soon as CS goes down while there's a pending write/erase.
                    handle_eeprom_pending_write();
                } else {
                    // Abort the current instruction and reset EEPROM state.
                    eeprom.instruction.cursor = 0;
                    eeprom.input.cursor = 0;
                    eeprom.output.cursor = 0;
                }

                // TODO: Idle or Done on falling CS when pending write is executed?
                eeprom.state = Eeprom::State::Idle;
            }

            eeprom.data_in = data_in;
            eeprom.clk = clk;
            eeprom.cs = cs;

            break;
        }
        default:
            break;
        }
    }
}

template <uint32_t RomSize, uint16_t EepromSize>
void* Mbc7<RomSize, EepromSize>::get_ram_save_data() {
    // Treat EEPROM as RAM data to save.
    return eeprom.data;
}

template <uint32_t RomSize, uint16_t EepromSize>
uint32_t Mbc7<RomSize, EepromSize>::get_ram_save_size() const {
    // Treat EEPROM as RAM data to save.
    return sizeof(eeprom.data);
}

template <uint32_t RomSize, uint16_t EepromSize>
void Mbc7<RomSize, EepromSize>::set_accelerometer(double x, double y) {
    accelerometer.sensor.x = x;
    accelerometer.sensor.y = y;
}

#ifdef ENABLE_DEBUGGER
template <uint32_t RomSize, uint16_t EepromSize>
uint8_t* Mbc7<RomSize, EepromSize>::get_rom_data() {
    return rom;
}

template <uint32_t RomSize, uint16_t EepromSize>
uint32_t Mbc7<RomSize, EepromSize>::get_rom_size() const {
    return RomSize;
}

template <uint32_t RomSize, uint16_t EepromSize>
uint8_t* Mbc7<RomSize, EepromSize>::get_ram_data() {
    // No actual RAM: just EEPROM.
    return nullptr;
}

template <uint32_t RomSize, uint16_t EepromSize>
uint32_t Mbc7<RomSize, EepromSize>::get_ram_size() const {
    // No actual RAM: just EEPROM.
    return 0;
}

template <uint32_t RomSize, uint16_t EepromSize>
uint8_t Mbc7<RomSize, EepromSize>::read_rom_raw(uint16_t bank, uint16_t address) const {
    ASSERT(address < 0x8000);
    uint32_t rom_address = bank << 14 | keep_bits<14>(address);
    rom_address = mask_by_pow2<RomSize>(rom_address);
    return rom[rom_address];
}

template <uint32_t RomSize, uint16_t EepromSize>
uint16_t Mbc7<RomSize, EepromSize>::get_rom_bank(uint16_t address) const {
    ASSERT(address < 0x8000);

    // 0000 - 0x3FFF
    if (address < 0x4000) {
        return 0;
    }

    // 4000 - 0x7FFF
    return rom_bank_selector;
}

template <uint32_t RomSize, uint16_t EepromSize>
uint8_t Mbc7<RomSize, EepromSize>::read_ram_raw(uint16_t bank, uint16_t address) const {
    ASSERT(address >= 0xA000 && address < 0xC000);
    return 0xFF;
}

template <uint32_t RomSize, uint16_t EepromSize>
uint16_t Mbc7<RomSize, EepromSize>::get_ram_bank(uint16_t address) const {
    ASSERT(address >= 0xA000 && address < 0xC000);
    return 0;
}
#endif

template <uint32_t RomSize, uint16_t EepromSize>
typename Mbc7<RomSize, EepromSize>::Eeprom::Command Mbc7<RomSize, EepromSize>::parse_eeprom_command(uint16_t value) {
    static constexpr typename Eeprom::Command COMMANDS[16] = {
        /* 0000 */ Eeprom::Command::EWDS,
        /* 0001 */ Eeprom::Command::WRAL,
        /* 0010 */ Eeprom::Command::ERAL,
        /* 0011 */ Eeprom::Command::EWEN,

        /* 0100 */ Eeprom::Command::WRITE,
        /* 0101 */ Eeprom::Command::WRITE,
        /* 0110 */ Eeprom::Command::WRITE,
        /* 0111 */ Eeprom::Command::WRITE,

        /* 1000 */ Eeprom::Command::READ,
        /* 1001 */ Eeprom::Command::READ,
        /* 1010 */ Eeprom::Command::READ,
        /* 1011 */ Eeprom::Command::READ,

        /* 1100 */ Eeprom::Command::ERASE,
        /* 1101 */ Eeprom::Command::ERASE,
        /* 1110 */ Eeprom::Command::ERASE,
        /* 1111 */ Eeprom::Command::ERASE,
    };

    return COMMANDS[get_bits_range<9, 6>(value)];
}

template <uint32_t RomSize, uint16_t EepromSize>
void Mbc7<RomSize, EepromSize>::handle_eeprom_instruction() {
    ASSERT(eeprom.state == Eeprom::State::Instruction);

    switch (eeprom.command) {
    case Eeprom::Command::EWDS:
        // EWDS: Disable Erase/Write
        eeprom.state = Eeprom::State::Done;
        eeprom.erase_write_enabled = false;
        break;
    case Eeprom::Command::EWEN:
        // EWEN: Enable Erase/Write
        eeprom.state = Eeprom::State::Done;
        eeprom.erase_write_enabled = true;
        break;
    case Eeprom::Command::READ: {
        // READ: Read EEPROM at address (instruction parameter) and emits data (16 bits output)
        eeprom.state = Eeprom::State::Output;
        uint16_t address = keep_bits_range<Eeprom::AddressSpaceBits - 1, 0>(eeprom.instruction.data);
        eeprom.output.data = eeprom.data[address];
        eeprom.output.cursor = 0;
        break;
    }
    case Eeprom::Command::WRITE:
        // WRITE: Write value (16 bits input) to address (instruction parameter)
        eeprom.state = Eeprom::State::Input;
        eeprom.input.cursor = 0;
        break;
    case Eeprom::Command::ERASE:
        // ERASE: Write FFFF to address (instruction parameter)
        eeprom.state = Eeprom::State::PendingWrite;
        break;
    case Eeprom::Command::WRAL:
        // WRAL: Fill EEPROM with given value (16 bits input)
        eeprom.state = Eeprom::State::Input;
        eeprom.input.cursor = 0;
        break;
    case Eeprom::Command::ERAL:
        // ERAL: Fill EEPROM with FFFF
        eeprom.state = Eeprom::State::PendingWrite;
        break;
    default:
        ASSERT_NO_ENTRY();
    }

    ASSERT(eeprom.state != Eeprom::State::Instruction);
}

template <uint32_t RomSize, uint16_t EepromSize>
void Mbc7<RomSize, EepromSize>::handle_eeprom_parametric_instruction() {
    ASSERT(eeprom.state == Eeprom::State::Input);

    // EEPROM data write is not executed immediately: instead, it is executed at the next falling edge of CS.
    switch (eeprom.command) {
    case Eeprom::Command::WRITE:
        eeprom.state = Eeprom::State::PendingWrite;
        break;
    case Eeprom::Command::WRAL:
        eeprom.state = Eeprom::State::PendingWrite;
        break;
    default:
        ASSERT_NO_ENTRY();
    }
}

template <uint32_t RomSize, uint16_t EepromSize>
void Mbc7<RomSize, EepromSize>::handle_eeprom_pending_write() {
    ASSERT(eeprom.state == Eeprom::State::PendingWrite);

    if (eeprom.erase_write_enabled) {
        switch (eeprom.command) {
        case Eeprom::Command::WRITE: {
            // WRITE: Write value (16 bits input) to address (instruction parameter)
            uint16_t address = keep_bits_range<Eeprom::AddressSpaceBits - 1, 0>(eeprom.instruction.data);
            eeprom.data[address] = eeprom.input.data;
            break;
        }
        case Eeprom::Command::ERASE: {
            // ERASE: Write FFFF to address (instruction parameter)
            uint16_t address = keep_bits_range<Eeprom::AddressSpaceBits - 1, 0>(eeprom.instruction.data);
            eeprom.data[address] = 0xFFFF;
            break;
        }
        case Eeprom::Command::WRAL:
            // WRAL: Fill EEPROM with given value (16 bits input)
            for (uint32_t i = 0; i < array_size(eeprom.data); ++i) {
                eeprom.data[i] = eeprom.input.data;
            }
            break;
        case Eeprom::Command::ERAL:
            // ERAL: Fill EEPROM with FFFF
            for (uint32_t i = 0; i < array_size(eeprom.data); ++i) {
                eeprom.data[i] = 0xFFFF;
            }
            break;
        default:
            ASSERT_NO_ENTRY();
        }

        // For the sake of simplicity, we immediately execute the writing and mark it as completed by raising DO.
        // TODO: actually, each command takes a different amount of cycles, and DO is raised only at the end it.
        eeprom.data_out = true;
    }

    eeprom.state = Eeprom::State::Done;

    ASSERT(eeprom.state == Eeprom::State::Done);
}

template <uint32_t RomSize, uint16_t EepromSize>
void Mbc7<RomSize, EepromSize>::save_state(Parcel& parcel) const {
    PARCEL_WRITE_BOOL(parcel, ram_enabled_primary);
    PARCEL_WRITE_BOOL(parcel, ram_enabled_secondary);
    PARCEL_WRITE_UINT16(parcel, rom_bank_selector);

    PARCEL_WRITE_UINT8(parcel, accelerometer.latch.x.low);
    PARCEL_WRITE_UINT8(parcel, accelerometer.latch.x.high);
    PARCEL_WRITE_UINT8(parcel, accelerometer.latch.y.low);
    PARCEL_WRITE_UINT8(parcel, accelerometer.latch.y.high);
    PARCEL_WRITE_DOUBLE(parcel, accelerometer.sensor.x);
    PARCEL_WRITE_DOUBLE(parcel, accelerometer.sensor.y);
    PARCEL_WRITE_BOOL(parcel, accelerometer.latched);

    PARCEL_WRITE_BOOL(parcel, eeprom.data_out);
    PARCEL_WRITE_BOOL(parcel, eeprom.data_in);
    PARCEL_WRITE_BOOL(parcel, eeprom.clk);
    PARCEL_WRITE_BOOL(parcel, eeprom.cs);
    PARCEL_WRITE_BOOL(parcel, eeprom.erase_write_enabled);
    PARCEL_WRITE_UINT8(parcel, static_cast<uint8_t>(eeprom.state));
    PARCEL_WRITE_UINT8(parcel, static_cast<uint8_t>(eeprom.command));
    PARCEL_WRITE_UINT16(parcel, eeprom.instruction.data);
    PARCEL_WRITE_UINT8(parcel, eeprom.instruction.cursor);
    PARCEL_WRITE_UINT16(parcel, eeprom.input.data);
    PARCEL_WRITE_UINT8(parcel, eeprom.input.cursor);
    PARCEL_WRITE_UINT16(parcel, eeprom.output.data);
    PARCEL_WRITE_UINT8(parcel, eeprom.output.cursor);
    PARCEL_WRITE_BYTES(parcel, eeprom.data, sizeof(eeprom.data));
}

template <uint32_t RomSize, uint16_t EepromSize>
void Mbc7<RomSize, EepromSize>::load_state(Parcel& parcel) {
    ram_enabled_primary = parcel.read_bool();
    ram_enabled_secondary = parcel.read_bool();
    rom_bank_selector = parcel.read_uint16();

    accelerometer.latch.x.low = parcel.read_uint8();
    accelerometer.latch.x.high = parcel.read_uint8();
    accelerometer.latch.y.low = parcel.read_uint8();
    accelerometer.latch.y.high = parcel.read_uint8();
    accelerometer.sensor.x = parcel.read_double();
    accelerometer.sensor.y = parcel.read_double();
    accelerometer.latched = parcel.read_bool();

    eeprom.data_out = parcel.read_bool();
    eeprom.data_in = parcel.read_bool();
    eeprom.clk = parcel.read_bool();
    eeprom.cs = parcel.read_bool();
    eeprom.erase_write_enabled = parcel.read_bool();
    eeprom.state = static_cast<typename Eeprom::State>(parcel.read_uint8());
    eeprom.command = static_cast<typename Eeprom::Command>(parcel.read_uint8());
    eeprom.instruction.data = parcel.read_uint16();
    eeprom.instruction.cursor = parcel.read_uint8();
    eeprom.input.data = parcel.read_uint16();
    eeprom.input.cursor = parcel.read_uint8();
    eeprom.output.data = parcel.read_uint16();
    eeprom.output.cursor = parcel.read_uint8();
    parcel.read_bytes(eeprom.data, sizeof(eeprom.data));
}

template <uint32_t RomSize, uint16_t EepromSize>
void Mbc7<RomSize, EepromSize>::reset() {
    ram_enabled_primary = false;
    ram_enabled_secondary = false;
    rom_bank_selector = 0x1;

    accelerometer.latch.x.low = 0x00;
    accelerometer.latch.x.high = 0x80;
    accelerometer.latch.y.low = 0x00;
    accelerometer.latch.y.high = 0x80;
    accelerometer.sensor.x = 0.0;
    accelerometer.sensor.y = 0.0;
    accelerometer.latched = false;

    eeprom.data_out = 0;
    eeprom.data_in = 0;
    eeprom.clk = 0;
    eeprom.cs = 0;
    eeprom.erase_write_enabled = false;
    eeprom.state = Eeprom::State::Idle;
    eeprom.command = Eeprom::Command::EWDS;
    eeprom.instruction.data = 0;
    eeprom.instruction.cursor = 0;
    eeprom.input.data = 0;
    eeprom.input.cursor = 0;
    eeprom.output.data = 0;
    eeprom.output.cursor = 0;

    for (uint32_t i = 0; i < array_size(eeprom.data); ++i) {
        eeprom.data[i] = 0xFFFF;
    }
}
