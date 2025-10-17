#include <cstring>
#include <ctime>

#include "docboy/common/specs.h"

#include "utils/algo.h"
#include "utils/arrays.h"
#include "utils/asserts.h"
#include "utils/bits.h"

template <uint32_t RomSize, uint32_t RamSize>
HuC3<RomSize, RamSize>::HuC3(const uint8_t* data, uint32_t length) {
    ASSERT(length <= array_size(rom), "HuC3: actual ROM size (" + std::to_string(length) +
                                          ") exceeds nominal ROM size (" + std::to_string(array_size(rom)) + ")");
    memcpy(rom, data, length);

    need_ticks = true;
}

template <uint32_t RomSize, uint32_t RamSize>
uint8_t HuC3<RomSize, RamSize>::read_rom(uint16_t address) const {
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

template <uint32_t RomSize, uint32_t RamSize>
void HuC3<RomSize, RamSize>::write_rom(uint16_t address, uint8_t value) {
    ASSERT(address < 0x8000);

    if (address < 0x4000) {
        // 0000 - 0x1FFF
        if (address < 0x2000) {
            // Update the function mapped to RAM area (A000).
            ram_mapping = static_cast<RamMapping>(keep_bits<4>(value));
            return;
        }

        // 0x2000 - 0x3FFF
        rom_bank_selector = keep_bits<7>(value);
        return;
    }

    if (address < 0x6000) {
        // 0x4000 - 0x5FFF
        ram_bank_selector = keep_bits<2>(value);
        return;
    }

    // 0x6000 - 0x7FFF
    // Nothing useful here.
}

template <uint32_t RomSize, uint32_t RamSize>
uint8_t HuC3<RomSize, RamSize>::read_ram(uint16_t address) const {
    ASSERT(address >= 0xA000 && address < 0xC000);

    // 0xA000 - 0xBFFF
    if (ram_mapping == RamMapping::RamReadOnly || ram_mapping == RamMapping::RamReadWrite) {
        uint32_t ram_address = (ram_bank_selector << 13) | keep_bits<13>(address);
        ram_address = mask_by_pow2<RamSize>(ram_address);
        return ram[ram_address];
    }
    if (ram_mapping == RamMapping::RtcRead) {
        // Read the result of the last executed command in the lowest significant nibble.
        // The highest significant nibble contains the last executed command in bits from 6 to 4,
        // while the most significant bit is floating, likely high.
        return 0x80 | rtc.command << 4 | rtc.read;
    }
    if (ram_mapping == RamMapping::RtcSemaphore) {
        // Should read 1 when the microcontroller is ready to execute a command.
        // For the sake of simplicity, we assume it's always ready.
        return 0x1;
    }
    if (ram_mapping == RamMapping::IR) {
        // I guess 0xC0 represents no IR signal, as in HuC1?
        // But I'm not sure.
        return 0xC0;
    }

    return 0xFF;
}

template <uint32_t RomSize, uint32_t RamSize>
void HuC3<RomSize, RamSize>::write_ram(uint16_t address, uint8_t value) {
    ASSERT(address >= 0xA000 && address < 0xC000);

    // 0xA000 - 0xBFFF
    if (ram_mapping == RamMapping::RamReadWrite) {
        uint32_t ram_address = (ram_bank_selector << 13) | keep_bits<13>(address);
        ram_address = mask_by_pow2<RamSize>(ram_address);
        ram[ram_address] = value;
    } else if (ram_mapping == RamMapping::RtcWrite) {
        // Store command (but does not execute it).
        rtc.command = get_bits_range<6, 4>(value);
        rtc.argument = keep_bits<4>(value);
    } else if (ram_mapping == RamMapping::RtcSemaphore) {
        // Writing to the semaphore with the least significant bit clear executes the pending command.
        if (!test_bit<0>(value)) {
            execute_rtc_command();
        }
    } else if (ram_mapping == RamMapping::IR) {
        // Since we don't have a working implementation of IR, just ignore the value.
    }
}

template <uint32_t RomSize, uint32_t RamSize>
void HuC3<RomSize, RamSize>::on_tick() {
    // We update the internal RTC clock one time per second, and we do so
    // by using the internal GameBoy clock for an accurate timing.
    // Despite that, we use the (host system) time() function to actually
    // keep of track of the "real" elapsed time for two reasons:
    // 1) The emulation can be out-of-sync: it might either go slower or faster.
    // 2) We want to keep track of the time elapsed even if the ROM is off
    //    (for instance when a save is loaded).
    if (++rtc.cycles_since_last_tick == Specs::Frequencies::CPU) {
        rtc.cycles_since_last_tick = 0;
        int64_t tick_time = std::time(nullptr);
        int64_t delta = tick_time - rtc.last_tick_time;

        // Increase the RTC once every minute.
        int64_t delta_minutes = delta / 60;
        if (delta_minutes > 0) {
            rtc.last_tick_time += delta_minutes * 60;
            tick_rtc(delta_minutes);
        }
    }
}

template <uint32_t RomSize, uint32_t RamSize>
void HuC3<RomSize, RamSize>::tick_rtc(int64_t delta /* minutes */) {
    auto [delta_days, delta_minutes] = divide(static_cast<uint32_t>(delta), 1440U);
    auto [carry_minutes, minutes] = divide(rtc.clock.minutes + delta_minutes, 1440U);

    rtc.clock.minutes = minutes;
    rtc.clock.days = keep_bits<12>(rtc.clock.days + delta_days + carry_minutes);

    ASSERT(rtc.clock.minutes < 1440);
    ASSERT(rtc.clock.days < 4096);
}

template <uint32_t RomSize, uint32_t RamSize>
void HuC3<RomSize, RamSize>::execute_rtc_command() {
    ASSERT(rtc.command < 0x07);
    ASSERT(rtc.argument < 0x10);
    switch (rtc.command) {
    case 0x00:
        // Read value
        rtc.read = rtc.memory[rtc.address];
        break;
    case 0x01:
        // Read value and increment RTC address
        rtc.read = rtc.memory[rtc.address++];
        break;
    case 0x02:
        // Write value
        rtc.memory[rtc.address] = rtc.argument;
        break;
    case 0x03:
        // Write value and increment RTC address
        rtc.memory[rtc.address++] = rtc.argument;
        break;
    case 0x04:
        // Set the least significant nibble of the RTC address
        rtc.address = keep_bits_range<7, 4>(rtc.address) | rtc.argument;
        break;
    case 0x05:
        // Set most significant nibble of the RTC address
        rtc.address = (rtc.argument) << 4 | keep_bits<4>(rtc.address);
        break;
    case 0x06:
        switch (rtc.argument) {
        case 0x00: {
            // Copy clock to scratchpad (Little Endian).
            rtc.memory[0] = get_bits_range<3, 0>(rtc.clock.minutes);
            rtc.memory[1] = get_bits_range<7, 4>(rtc.clock.minutes);
            rtc.memory[2] = get_bits_range<11, 8>(rtc.clock.minutes);
            rtc.memory[3] = get_bits_range<3, 0>(rtc.clock.days);
            rtc.memory[4] = get_bits_range<7, 4>(rtc.clock.days);
            rtc.memory[5] = get_bits_range<11, 8>(rtc.clock.days);
            break;
        }
        case 0x01:
            // Copy scratchpad to clock (Little Endian).
            rtc.clock.minutes = (rtc.memory[2] << 8) | (rtc.memory[1] << 4) | rtc.memory[0];
            rtc.clock.days = (rtc.memory[5] << 8) | (rtc.memory[4] << 4) | rtc.memory[3];
            break;
        case 0x02:
            // Command 0x62 is submitted on start by HuC3 games.
            // The software expects the least significant nibble from RTC read to be 0x01 afterward.
            rtc.read = 0x01;
            break;
        default:
            break;
        }
        break;
    default:
        break;
    }
}

template <uint32_t RomSize, uint32_t RamSize>
void* HuC3<RomSize, RamSize>::get_ram_save_data() {
    return ram;
}

template <uint32_t RomSize, uint32_t RamSize>
uint32_t HuC3<RomSize, RamSize>::get_ram_save_size() const {
    return RamSize;
}

template <uint32_t RomSize, uint32_t RamSize>
void* HuC3<RomSize, RamSize>::get_rtc_save_data() {
    return &rtc.last_tick_time;
}

template <uint32_t RomSize, uint32_t RamSize>
uint32_t HuC3<RomSize, RamSize>::get_rtc_save_size() const {
    static_assert(offsetof(Rtc, last_tick_time) + sizeof(Rtc::last_tick_time) + sizeof(Rtc::memory) ==
                  offsetof(Rtc, clock));
    static_assert(sizeof(Rtc::clock) == sizeof(Rtc::clock.minutes) + sizeof(Rtc::clock.days));

    return sizeof(rtc.last_tick_time) + sizeof(rtc.memory) + sizeof(rtc.clock);
}

#ifdef ENABLE_DEBUGGER
template <uint32_t RomSize, uint32_t RamSize>
uint8_t* HuC3<RomSize, RamSize>::get_rom_data() {
    return rom;
}

template <uint32_t RomSize, uint32_t RamSize>
uint32_t HuC3<RomSize, RamSize>::get_rom_size() const {
    return RomSize;
}
#endif

template <uint32_t RomSize, uint32_t RamSize>
void HuC3<RomSize, RamSize>::save_state(Parcel& parcel) const {
    parcel.write_uint8(static_cast<uint8_t>(ram_mapping));
    parcel.write_uint8(rtc.command);
    parcel.write_uint8(rtc.argument);
    parcel.write_uint8(rtc.read);
    parcel.write_uint8(rtc.address);
    parcel.write_uint32(rtc.cycles_since_last_tick);
    parcel.write_int64(rtc.last_tick_time);
    parcel.write_bytes(rtc.memory, sizeof(rtc.memory));
    parcel.write_uint16(rtc.clock.minutes);
    parcel.write_uint16(rtc.clock.days);
    parcel.write_uint16(rom_bank_selector);
    parcel.write_uint8(ram_bank_selector);
    parcel.write_bytes(ram, RamSize);
}

template <uint32_t RomSize, uint32_t RamSize>
void HuC3<RomSize, RamSize>::load_state(Parcel& parcel) {
    ram_mapping = static_cast<RamMapping>(parcel.read_uint8());
    rtc.command = parcel.read_uint8();
    rtc.argument = parcel.read_uint8();
    rtc.read = parcel.read_uint8();
    rtc.address = parcel.read_uint8();
    rtc.cycles_since_last_tick = parcel.read_uint32();
    rtc.last_tick_time = parcel.read_int64();
    parcel.read_bytes(rtc.memory, sizeof(rtc.memory));
    rtc.clock.minutes = parcel.read_uint16();
    rtc.clock.days = parcel.read_uint16();
    rom_bank_selector = parcel.read_uint16();
    ram_bank_selector = parcel.read_uint8();
    parcel.read_bytes(ram, RamSize);
}

template <uint32_t RomSize, uint32_t RamSize>
void HuC3<RomSize, RamSize>::reset() {
    ram_mapping = RamMapping::RamReadOnly;
    rtc.command = 0;
    rtc.argument = 0;
    rtc.read = 0;
    rtc.address = 0;
    rtc.cycles_since_last_tick = 0;
    rtc.last_tick_time = std::time(nullptr);
    memset(rtc.memory, 0, sizeof(rtc.memory));
    rtc.clock.minutes = 0;
    rtc.clock.days = 0;
    rom_bank_selector = 0b1;
    ram_bank_selector = 0;
    memset(ram, 0, RamSize);
}
