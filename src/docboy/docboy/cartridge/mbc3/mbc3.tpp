#include "utils/arrays.h"
#include "utils/bits.hpp"
#include "utils/exceptions.hpp"
#include "utils/formatters.hpp"
#include <cstring>

template <uint32_t RomSize, uint32_t RamSize, bool Battery, bool Timer>
Mbc3<RomSize, RamSize, Battery, Timer>::Mbc3(const uint8_t* data, uint32_t length) :
    rtcRealMap {
        /* 08 */ &rtc.real.seconds,
        /* 09 */ &rtc.real.minutes,
        /* 0A */ &rtc.real.hours,
        /* 0B */ &rtc.real.days.low,
        /* 0C */ &rtc.real.days.high,
    } {
    check(length <= array_size(rom), "Mbc3: actual ROM size (" + std::to_string(length) +
                                         ") exceeds nominal ROM size (" + std::to_string(array_size(rom)) + ")");
    memcpy(rom, data, length);

    // Use the GB clock as RTC timing source for having precise timing.
    needTicks = Timer;
}

template <uint32_t RomSize, uint32_t RamSize, bool Battery, bool Timer>
uint8_t Mbc3<RomSize, RamSize, Battery, Timer>::readRom(uint16_t address) const {
    check(address < 0x8000);

    // 0000 - 0x3FFF
    if (address < 0x4000)
        return rom[address];

    // 4000 - 0x7FFF
    uint32_t romAddress = romBankSelector << 14 | keep_bits<14>(address);
    romAddress = masked<RomSize>(romAddress);
    return rom[romAddress];
}

template <uint32_t RomSize, uint32_t RamSize, bool Battery, bool Timer>
void Mbc3<RomSize, RamSize, Battery, Timer>::writeRom(uint16_t address, uint8_t value) {
    check(address < 0x8000);

    if (address < 0x4000) {
        // 0000 - 0x1FFF
        if (address < 0x2000) {
            ramEnabled = keep_bits<4>(value) == 0xA;
            return;
        }
        // 0x2000 - 0x3FFF
        romBankSelector = keep_bits<7>(value);
        romBankSelector = romBankSelector > 0 ? romBankSelector : 0b1;
        return;
    }

    // 0x4000 - 0x5FFF
    if (address < 0x6000) {
        ramBankSelector_rtcRegisterSelector = keep_bits<4>(value);
        return;
    }

    // 0x6000 - 0x7FFF
    if constexpr (Timer) {
        rtc.latched = rtc.real;
    }
}

template <uint32_t RomSize, uint32_t RamSize, bool Battery, bool Timer>
uint8_t Mbc3<RomSize, RamSize, Battery, Timer>::readRam(uint16_t address) const {
    check(address >= 0xA000 && address < 0xC000);

    // 0xA000 - 0xBFFF
    if (ramEnabled) {
        if (ramBankSelector_rtcRegisterSelector < 0x4) {
            if constexpr (Ram) {
                uint32_t ramAddress = (ramBankSelector_rtcRegisterSelector << 13) | keep_bits<13>(address);
                ramAddress = masked<RamSize>(ramAddress);
                return ram[ramAddress];
            }
        } else if (ramBankSelector_rtcRegisterSelector >= 0x08 && ramBankSelector_rtcRegisterSelector <= 0x0C) {
            if constexpr (Timer) {
                switch (ramBankSelector_rtcRegisterSelector) {
                case 0x08:
                    return keep_bits<6>(rtc.latched.seconds);
                case 0x09:
                    return keep_bits<6>(rtc.latched.minutes);
                case 0x0A:
                    return keep_bits<5>(rtc.latched.hours);
                case 0x0B:
                    return rtc.latched.days.low;
                default:
                    check(ramBankSelector_rtcRegisterSelector == 0x0C);
                    return get_bits<7, 6, 0>(rtc.latched.days.high);
                }
            }
        }
    }

    return 0xFF;
}

template <uint32_t RomSize, uint32_t RamSize, bool Battery, bool Timer>
void Mbc3<RomSize, RamSize, Battery, Timer>::writeRam(uint16_t address, uint8_t value) {
    check(address >= 0xA000 && address < 0xC000);

    // 0xA000 - 0xBFFF
    if (ramEnabled) {
        if (ramBankSelector_rtcRegisterSelector < 0x4) {
            if constexpr (Ram) {
                uint32_t ramAddress = (ramBankSelector_rtcRegisterSelector << 13) | keep_bits<13>(address);
                ramAddress = masked<RamSize>(ramAddress);
                ram[ramAddress] = value;
            }
        } else if (ramBankSelector_rtcRegisterSelector >= 0x08 && ramBankSelector_rtcRegisterSelector <= 0x0C) {
            if constexpr (Timer) {
                const uint8_t rtcReg = keep_bits<3>(ramBankSelector_rtcRegisterSelector);
                *rtcRealMap[rtcReg] = value;
                if (rtcReg == 0)
                    // Writing to RTC seconds register resets RTC internal cycle counter.
                    rtc.real.cycles = 0;
            }
        }
    }
}

template <uint32_t RomSize, uint32_t RamSize, bool Battery, bool Timer>
uint8_t* Mbc3<RomSize, RamSize, Battery, Timer>::getRamSaveData() {
    if constexpr (Ram && Battery) {
        return ram;
    }
    return nullptr;
}

template <uint32_t RomSize, uint32_t RamSize, bool Battery, bool Timer>
uint32_t Mbc3<RomSize, RamSize, Battery, Timer>::getRamSaveSize() const {
    if constexpr (Ram && Battery) {
        return RamSize;
    }
    return 0;
}

template <uint32_t RomSize, uint32_t RamSize, bool Battery, bool Timer>
void Mbc3<RomSize, RamSize, Battery, Timer>::tick() {
    if constexpr (Timer) {
        rtc.real.tick();
    }
}

#ifdef ENABLE_DEBUGGER
template <uint32_t RomSize, uint32_t RamSize, bool Battery, bool Timer>
uint8_t* Mbc3<RomSize, RamSize, Battery, Timer>::getRomData() {
    return rom;
}

template <uint32_t RomSize, uint32_t RamSize, bool Battery, bool Timer>
uint32_t Mbc3<RomSize, RamSize, Battery, Timer>::getRomSize() const {
    return RomSize;
}
#endif

template <uint32_t RomSize, uint32_t RamSize, bool Battery, bool Timer>
void Mbc3<RomSize, RamSize, Battery, Timer>::loadState(Parcel& parcel) {
    ramEnabled = parcel.readBool();
    romBankSelector = parcel.readUInt8();
    ramBankSelector_rtcRegisterSelector = parcel.readUInt8();
    rtc.real.cycles = parcel.readUInt32();
    rtc.real.seconds = parcel.readUInt8();
    rtc.real.minutes = parcel.readUInt8();
    rtc.real.hours = parcel.readUInt8();
    rtc.real.days.low = parcel.readUInt8();
    rtc.real.days.high = parcel.readUInt8();
    rtc.latched.seconds = parcel.readUInt8();
    rtc.latched.minutes = parcel.readUInt8();
    rtc.latched.hours = parcel.readUInt8();
    rtc.latched.days.low = parcel.readUInt8();
    rtc.latched.days.high = parcel.readUInt8();
    parcel.readBytes(rom, RomSize);
    if constexpr (Ram) {
        parcel.readBytes(ram, RamSize);
    }
}

template <uint32_t RomSize, uint32_t RamSize, bool Battery, bool Timer>
void Mbc3<RomSize, RamSize, Battery, Timer>::saveState(Parcel& parcel) const {
    parcel.writeBool(ramEnabled);
    parcel.writeUInt8(romBankSelector);
    parcel.writeUInt8(ramBankSelector_rtcRegisterSelector);
    parcel.writeUInt32(rtc.real.cycles);
    parcel.writeUInt8(rtc.real.seconds);
    parcel.writeUInt8(rtc.real.minutes);
    parcel.writeUInt8(rtc.real.hours);
    parcel.writeUInt8(rtc.real.days.low);
    parcel.writeUInt8(rtc.real.days.high);
    parcel.writeUInt8(rtc.latched.seconds);
    parcel.writeUInt8(rtc.latched.minutes);
    parcel.writeUInt8(rtc.latched.hours);
    parcel.writeUInt8(rtc.latched.days.low);
    parcel.writeUInt8(rtc.latched.days.high);
    parcel.writeBytes(rom, RomSize);
    if constexpr (Ram) {
        parcel.writeBytes(ram, RamSize);
    }
}
