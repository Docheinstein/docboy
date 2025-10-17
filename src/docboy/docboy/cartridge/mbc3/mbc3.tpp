#include <cstring>

#include "utils/algo.h"
#include "utils/arrays.h"
#include "utils/asserts.h"
#include "utils/bits.h"
#include "utils/exceptions.h"

template <uint32_t RomSize, uint32_t RamSize, bool Battery, bool Timer>
Mbc3<RomSize, RamSize, Battery, Timer>::Mbc3(const uint8_t* data, uint32_t length) {
    ASSERT(length <= array_size(rom), "Mbc3: actual ROM size (" + std::to_string(length) +
                                          ") exceeds nominal ROM size (" + std::to_string(array_size(rom)) + ")");
    memcpy(rom, data, length);

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
        // RTC clock is loaded into the latched one only on latch's raising edge.
        if (!test_bit<0>(rtc.latch) && test_bit<0>(value)) {
            rtc.latched = rtc.clock;
        }
        rtc.latch = value;
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
        } else {
            if constexpr (Timer) {
                switch (ram_bank_selector_rtc_register_selector) {
                case 0x08:
                    return keep_bits<6>(rtc.latched.seconds);
                case 0x09:
                    return keep_bits<6>(rtc.latched.minutes);
                case 0x0A:
                    return keep_bits<5>(rtc.latched.hours);
                case 0x0B:
                    return rtc.latched.days_low;
                case 0x0C:
                    return get_bits<Specs::Bits::Rtc::DH::DAY_OVERFLOW, Specs::Bits::Rtc::DH::STOPPED,
                                    Specs::Bits::Rtc::DH::DAY>(rtc.latched.days_high);
                default:
                    break;
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
        } else {
            if constexpr (Timer) {
                switch (ram_bank_selector_rtc_register_selector) {
                case 0x08:
                    rtc.clock.seconds = keep_bits<6>(value);
                    // Writing to the seconds register resets the internal counter.
                    rtc.cycles_since_last_tick = 0;
                    rtc.last_tick_time = std::time(nullptr);
                    break;
                case 0x09:
                    rtc.clock.minutes = keep_bits<6>(value);
                    break;
                case 0x0A:
                    rtc.clock.hours = keep_bits<5>(value);
                    break;
                case 0x0B:
                    rtc.clock.days_low = value;
                    break;
                case 0x0C:
                    rtc.clock.days_high = get_bits<Specs::Bits::Rtc::DH::DAY_OVERFLOW, Specs::Bits::Rtc::DH::STOPPED,
                                                   Specs::Bits::Rtc::DH::DAY>(value);
                    break;
                default:
                    break;
                }
            }
        }
    }
}

template <uint32_t RomSize, uint32_t RamSize, bool Battery, bool Timer>
void Mbc3<RomSize, RamSize, Battery, Timer>::on_tick() {
    if constexpr (Timer) {
        if (test_bit<Specs::Bits::Rtc::DH::STOPPED>(rtc.clock.days_high)) {
            // RTC does not tick while it's stopped.
            return;
        }

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
            tick_rtc(delta);
            rtc.last_tick_time = tick_time;
        }
    }
}

template <uint32_t RomSize, uint32_t RamSize, bool Battery, bool Timer>
void Mbc3<RomSize, RamSize, Battery, Timer>::tick_rtc(int64_t delta /* seconds */) {
    static_assert(Timer);

    if (delta == 1) {
        // This is the standard and more common case.
        // Increment the seconds and eventually recursively overflow with carry into the other registers.
        if (++rtc.clock.seconds == 60) {
            rtc.clock.seconds = 0;
            if (++rtc.clock.minutes == 60) {
                rtc.clock.minutes = 0;
                if (++rtc.clock.hours == 24) {
                    rtc.clock.hours = 0;
                    if (++rtc.clock.days_low == 0) {
                        // Overflow flag is set if the high bit of days is already set and days exceeded 255 another
                        // time. Note that is it never cleared automatically: it must be reset manually.
                        if (test_bit<Specs::Bits::Rtc::DH::DAY>(rtc.clock.days_high)) {
                            set_bit<Specs::Bits::Rtc::DH::DAY_OVERFLOW>(rtc.clock.days_high);
                        }
                        toggle_bit<Specs::Bits::Rtc::DH::DAY>(rtc.clock.days_high);
                    }
                }
            }
        }
    } else if (delta > 0) {
        // This covers the most general case of RTC increment (delta > 1).
        // It's unlikely that we get here, but it might happen mainly for two reasons:
        // 1) A save is loaded: in this case the elapsed time since the last tick can be arbitrarily high.
        // 2) The emulator is running slower than the expected speed.
        // TODO: some edge cases are probably not covered in this general case.
        //       e.g. a register was already out of range (for instance 61) and the increment (say 2) wouldn't make it
        //       overflow.
        auto [delta_days, reminder_days] = divide(static_cast<uint32_t>(delta), 86400U);
        auto [delta_hours, reminder_hours] = divide(reminder_days, 3600U);
        auto [delta_minutes, delta_seconds] = divide(reminder_hours, 60U);

        auto [carry_seconds, seconds] = divide(rtc.clock.seconds + delta_seconds, 60U);
        auto [carry_minutes, minutes] = divide(rtc.clock.minutes + delta_minutes + carry_seconds, 60U);
        auto [carry_hours, hours] = divide(rtc.clock.hours + delta_hours + carry_minutes, 24U);
        auto [carry_days, days] =
            divide((get_bit<Specs::Bits::Rtc::DH::DAY>(rtc.clock.days_high) << 8 | rtc.clock.days_low) + delta_days +
                       carry_hours,
                   512U);

        rtc.clock.seconds = seconds;
        rtc.clock.minutes = minutes;
        rtc.clock.hours = hours;
        rtc.clock.days_low = keep_bits<8>(days);
        set_bit<Specs::Bits::Rtc::DH::DAY>(rtc.clock.days_high, test_bit<8>(days));
        if (carry_days) {
            set_bit<Specs::Bits::Rtc::DH::DAY_OVERFLOW>(rtc.clock.days_high);
        }

        ASSERT(rtc.clock.seconds < 60);
        ASSERT(rtc.clock.minutes < 60);
        ASSERT(rtc.clock.hours < 24);
    }
}

template <uint32_t RomSize, uint32_t RamSize, bool Battery, bool Timer>
void* Mbc3<RomSize, RamSize, Battery, Timer>::get_ram_save_data() {
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
void* Mbc3<RomSize, RamSize, Battery, Timer>::get_rtc_save_data() {
    if constexpr (Timer && Battery) {
        return &rtc.last_tick_time;
    }
    return nullptr;
}

template <uint32_t RomSize, uint32_t RamSize, bool Battery, bool Timer>
uint32_t Mbc3<RomSize, RamSize, Battery, Timer>::get_rtc_save_size() const {
    static_assert(sizeof(RtcRegisters) == 5);
    static_assert(offsetof(Rtc, last_tick_time) + sizeof(Rtc::last_tick_time) == offsetof(Rtc, clock));

    if constexpr (Timer && Battery) {
        // We want to serialize both the realtime clock and the last tick time,
        // so that we can figure out how much (real) time is elapsed when the
        // save will be loaded again.
        return sizeof(rtc.last_tick_time) + sizeof(rtc.clock);
    }
    return 0;
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
    rtc.cycles_since_last_tick = parcel.read_uint32();
    rtc.last_tick_time = parcel.read_int64();
    rtc.clock.seconds = parcel.read_uint8();
    rtc.clock.minutes = parcel.read_uint8();
    rtc.clock.hours = parcel.read_uint8();
    rtc.clock.days_low = parcel.read_uint8();
    rtc.clock.days_high = parcel.read_uint8();
    rtc.latched.seconds = parcel.read_uint8();
    rtc.latched.minutes = parcel.read_uint8();
    rtc.latched.hours = parcel.read_uint8();
    rtc.latched.days_low = parcel.read_uint8();
    rtc.latched.days_high = parcel.read_uint8();
    rtc.latch = parcel.read_uint8();
    if constexpr (Ram) {
        parcel.read_bytes(ram, RamSize);
    }
}

template <uint32_t RomSize, uint32_t RamSize, bool Battery, bool Timer>
void Mbc3<RomSize, RamSize, Battery, Timer>::save_state(Parcel& parcel) const {
    parcel.write_bool(ram_enabled);
    parcel.write_uint8(rom_bank_selector);
    parcel.write_uint8(ram_bank_selector_rtc_register_selector);
    parcel.write_uint32(rtc.cycles_since_last_tick);
    parcel.write_int64(rtc.last_tick_time);
    parcel.write_uint8(rtc.clock.seconds);
    parcel.write_uint8(rtc.clock.minutes);
    parcel.write_uint8(rtc.clock.hours);
    parcel.write_uint8(rtc.clock.days_low);
    parcel.write_uint8(rtc.clock.days_high);
    parcel.write_uint8(rtc.latched.seconds);
    parcel.write_uint8(rtc.latched.minutes);
    parcel.write_uint8(rtc.latched.hours);
    parcel.write_uint8(rtc.latched.days_low);
    parcel.write_uint8(rtc.latched.days_high);
    parcel.write_uint8(rtc.latch);
    if constexpr (Ram) {
        parcel.write_bytes(ram, RamSize);
    }
}

template <uint32_t RomSize, uint32_t RamSize, bool Battery, bool Timer>
void Mbc3<RomSize, RamSize, Battery, Timer>::reset() {
    ram_enabled = false;
    rom_bank_selector = 0b1;
    ram_bank_selector_rtc_register_selector = 0;

    rtc.cycles_since_last_tick = 0;
    rtc.last_tick_time = std::time(nullptr);
    rtc.clock.seconds = 0;
    rtc.clock.minutes = 0;
    rtc.clock.hours = 0;
    rtc.clock.days_low = 0;
    rtc.clock.days_high = 0;
    rtc.latched.seconds = 0;
    rtc.latched.minutes = 0;
    rtc.latched.hours = 0;
    rtc.latched.days_low = 0;
    rtc.latched.days_high = 0;
    rtc.latch = 0;

    memset(ram, 0, RamSize);
}
