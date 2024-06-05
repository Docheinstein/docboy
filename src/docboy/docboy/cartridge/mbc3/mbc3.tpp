#include <cstring>

#include "utils/arrays.h"
#include "utils/bits.h"
#include "utils/exceptions.h"
#include "utils/formatters.h"

template <uint32_t RomSize, uint32_t RamSize, bool Battery, bool Timer>
Mbc3<RomSize, RamSize, Battery, Timer>::Mbc3(const uint8_t* data, uint32_t length) :
    rtc_real_map {
        /* 08 */ &rtc.real.seconds,
        /* 09 */ &rtc.real.minutes,
        /* 0A */ &rtc.real.hours,
        /* 0B */ &rtc.real.days.low,
        /* 0C */ &rtc.real.days.high,
    } {
    ASSERT(length <= array_size(rom), "Mbc3: actual ROM size (" + std::to_string(length) +
                                         ") exceeds nominal ROM size (" + std::to_string(array_size(rom)) + ")");
    memcpy(rom, data, length);

    // Use the GB clock as RTC timing source for having precise timing.
    need_ticks = Timer;
}

template <uint32_t RomSize, uint32_t RamSize, bool Battery, bool Timer>
uint8_t Mbc3<RomSize, RamSize, Battery, Timer>::read_rom(uint16_t address) const {
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

template <uint32_t RomSize, uint32_t RamSize, bool Battery, bool Timer>
void Mbc3<RomSize, RamSize, Battery, Timer>::write_rom(uint16_t address, uint8_t value) {
    ASSERT(address < 0x8000);

    if (address < 0x4000) {
        // 0000 - 0x1FFF
        if (address < 0x2000) {
            ram_enabled = keep_bits<4>(value) == 0xA;
            return;
        }
        // 0x2000 - 0x3FFF
        rom_bank_selector = keep_bits<7>(value);
        rom_bank_selector = rom_bank_selector > 0 ? rom_bank_selector : 0b1;
        return;
    }

    // 0x4000 - 0x5FFF
    if (address < 0x6000) {
        ram_bank_selector_rtc_register_selector = keep_bits<4>(value);
        return;
    }

    // 0x6000 - 0x7FFF
    if constexpr (Timer) {
        rtc.latched = rtc.real;
    }
}

template <uint32_t RomSize, uint32_t RamSize, bool Battery, bool Timer>
uint8_t Mbc3<RomSize, RamSize, Battery, Timer>::read_ram(uint16_t address) const {
    ASSERT(address >= 0xA000 && address < 0xC000);

    // 0xA000 - 0xBFFF
    if (ram_enabled) {
        if (ram_bank_selector_rtc_register_selector < 0x4) {
            if constexpr (Ram) {
                uint32_t ram_address = (ram_bank_selector_rtc_register_selector << 13) | keep_bits<13>(address);
                ram_address = mask_by_pow2<RamSize>(ram_address);
                return ram[ram_address];
            }
        } else if (ram_bank_selector_rtc_register_selector >= 0x08 && ram_bank_selector_rtc_register_selector <= 0x0C) {
            if constexpr (Timer) {
                switch (ram_bank_selector_rtc_register_selector) {
                case 0x08:
                    return keep_bits<6>(rtc.latched.seconds);
                case 0x09:
                    return keep_bits<6>(rtc.latched.minutes);
                case 0x0A:
                    return keep_bits<5>(rtc.latched.hours);
                case 0x0B:
                    return rtc.latched.days.low;
                default:
                    ASSERT(ram_bank_selector_rtc_register_selector == 0x0C);
                    return get_bits<7, 6, 0>(rtc.latched.days.high);
                }
            }
        }
    }

    return 0xFF;
}

template <uint32_t RomSize, uint32_t RamSize, bool Battery, bool Timer>
void Mbc3<RomSize, RamSize, Battery, Timer>::write_ram(uint16_t address, uint8_t value) {
    ASSERT(address >= 0xA000 && address < 0xC000);

    // 0xA000 - 0xBFFF
    if (ram_enabled) {
        if (ram_bank_selector_rtc_register_selector < 0x4) {
            if constexpr (Ram) {
                uint32_t ram_address = (ram_bank_selector_rtc_register_selector << 13) | keep_bits<13>(address);
                ram_address = mask_by_pow2<RamSize>(ram_address);
                ram[ram_address] = value;
            }
        } else if (ram_bank_selector_rtc_register_selector >= 0x08 && ram_bank_selector_rtc_register_selector <= 0x0C) {
            if constexpr (Timer) {
                const uint8_t rtc_reg = keep_bits<3>(ram_bank_selector_rtc_register_selector);
                *rtc_real_map[rtc_reg] = value;
                if (rtc_reg == 0) {
                    // Writing to RTC seconds register resets RTC internal cycle counter.
                    rtc.real.cycles = 0;
                }
            }
        }
    }
}

template <uint32_t RomSize, uint32_t RamSize, bool Battery, bool Timer>
uint8_t* Mbc3<RomSize, RamSize, Battery, Timer>::get_ram_save_data() {
    if constexpr (Ram && Battery) {
        return ram;
    }
    return nullptr;
}

template <uint32_t RomSize, uint32_t RamSize, bool Battery, bool Timer>
uint32_t Mbc3<RomSize, RamSize, Battery, Timer>::get_ram_save_size() const {
    if constexpr (Ram && Battery) {
        return RamSize;
    }
    return 0;
}

template <uint32_t RomSize, uint32_t RamSize, bool Battery, bool Timer>
void Mbc3<RomSize, RamSize, Battery, Timer>::on_tick() {
    if constexpr (Timer) {
        rtc.real.tick();
    }
}

#ifdef ENABLE_DEBUGGER
template <uint32_t RomSize, uint32_t RamSize, bool Battery, bool Timer>
uint8_t* Mbc3<RomSize, RamSize, Battery, Timer>::get_rom_data() {
    return rom;
}

template <uint32_t RomSize, uint32_t RamSize, bool Battery, bool Timer>
uint32_t Mbc3<RomSize, RamSize, Battery, Timer>::get_rom_size() const {
    return RomSize;
}
#endif

template <uint32_t RomSize, uint32_t RamSize, bool Battery, bool Timer>
void Mbc3<RomSize, RamSize, Battery, Timer>::load_state(Parcel& parcel) {
    ram_enabled = parcel.read_bool();
    rom_bank_selector = parcel.read_uint8();
    ram_bank_selector_rtc_register_selector = parcel.read_uint8();
    rtc.real.cycles = parcel.read_uint32();
    rtc.real.seconds = parcel.read_uint8();
    rtc.real.minutes = parcel.read_uint8();
    rtc.real.hours = parcel.read_uint8();
    rtc.real.days.low = parcel.read_uint8();
    rtc.real.days.high = parcel.read_uint8();
    rtc.latched.seconds = parcel.read_uint8();
    rtc.latched.minutes = parcel.read_uint8();
    rtc.latched.hours = parcel.read_uint8();
    rtc.latched.days.low = parcel.read_uint8();
    rtc.latched.days.high = parcel.read_uint8();
    parcel.read_bytes(rom, RomSize);
    if constexpr (Ram) {
        parcel.read_bytes(ram, RamSize);
    }
}

template <uint32_t RomSize, uint32_t RamSize, bool Battery, bool Timer>
void Mbc3<RomSize, RamSize, Battery, Timer>::save_state(Parcel& parcel) const {
    parcel.write_bool(ram_enabled);
    parcel.write_uint8(rom_bank_selector);
    parcel.write_uint8(ram_bank_selector_rtc_register_selector);
    parcel.write_uint32(rtc.real.cycles);
    parcel.write_uint8(rtc.real.seconds);
    parcel.write_uint8(rtc.real.minutes);
    parcel.write_uint8(rtc.real.hours);
    parcel.write_uint8(rtc.real.days.low);
    parcel.write_uint8(rtc.real.days.high);
    parcel.write_uint8(rtc.latched.seconds);
    parcel.write_uint8(rtc.latched.minutes);
    parcel.write_uint8(rtc.latched.hours);
    parcel.write_uint8(rtc.latched.days.low);
    parcel.write_uint8(rtc.latched.days.high);
    parcel.write_bytes(rom, RomSize);
    if constexpr (Ram) {
        parcel.write_bytes(ram, RamSize);
    }
}

template <uint32_t RomSize, uint32_t RamSize, bool Battery, bool Timer>
void Mbc3<RomSize, RamSize, Battery, Timer>::reset() {
    ram_enabled = false;
    rom_bank_selector = 0b1;
    ram_bank_selector_rtc_register_selector = 0;

    rtc.real.reset();
    rtc.latched.reset();

    memset(ram, 0, RamSize);
}
