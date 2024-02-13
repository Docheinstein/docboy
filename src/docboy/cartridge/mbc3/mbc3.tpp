#include "utils/bits.hpp"
#include "utils/chrono.hpp"
#include "utils/exceptions.hpp"
#include "utils/formatters.hpp"
#include "utils/arrays.h"
#include <cstring>

template <uint32_t RomSize, uint32_t RamSize, bool Battery, bool Timer>
Mbc3<RomSize, RamSize, Battery, Timer>::Mbc3(const uint8_t* data, uint32_t length) :
    rtcRegistersMap {
        /* 08 */ &rtcRegisters.seconds,
        /* 09 */ &rtcRegisters.minutes,
        /* 0A */ &rtcRegisters.hours,
        /* 0B */ &rtcRegisters.day.low,
        /* 0C */ &rtcRegisters.day.high,
    } {
    check(length <= array_size(rom), "Mbc3: actual ROM size (" + std::to_string(length) +
                                         ") exceeds nominal ROM size (" + std::to_string(array_size(rom)) + ")");
    memcpy(rom, data, length);
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
            ramAndTimerEnabled = keep_bits<4>(value) == 0xA;
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
    if (latchClockData == 0x0 && value == 0x1) {
        // TODO: this is wrong
        const auto datetime = get_current_datetime();
        rtcRegisters.seconds = datetime.tm_sec;
        rtcRegisters.minutes = datetime.tm_min;
        rtcRegisters.hours = datetime.tm_hour;
        rtcRegisters.day.low = datetime.tm_yday;
    }
    latchClockData = value;
}

template <uint32_t RomSize, uint32_t RamSize, bool Battery, bool Timer>
uint8_t Mbc3<RomSize, RamSize, Battery, Timer>::readRam(uint16_t address) const {
    check(address >= 0xA000 && address < 0xC000);

    // 0xA000 - 0xBFFF
    if (ramBankSelector_rtcRegisterSelector < 0x4) {
        if constexpr (Ram) {
            if (ramAndTimerEnabled) {
                uint32_t ramAddress = (ramBankSelector_rtcRegisterSelector << 13) | keep_bits<13>(address);
                ramAddress = masked<RamSize>(ramAddress);
                return ram[ramAddress];
            }
        }
    } else if (ramBankSelector_rtcRegisterSelector >= 0x08 && ramBankSelector_rtcRegisterSelector <= 0x0C) {
        return *rtcRegistersMap[keep_bits<3>(ramBankSelector_rtcRegisterSelector)];
    }

    return 0xFF;
}

template <uint32_t RomSize, uint32_t RamSize, bool Battery, bool Timer>
void Mbc3<RomSize, RamSize, Battery, Timer>::writeRam(uint16_t address, uint8_t value) {
    check(address >= 0xA000 && address < 0xC000);

    // 0xA000 - 0xBFFF
    if (ramBankSelector_rtcRegisterSelector < 0x4) {
        if constexpr (Ram) {
            if (ramAndTimerEnabled) {
                uint32_t ramAddress = (ramBankSelector_rtcRegisterSelector << 13) | keep_bits<13>(address);
                ramAddress = masked<RamSize>(ramAddress);
                ram[ramAddress] = value;
            }
        }
    } else if (ramBankSelector_rtcRegisterSelector >= 0x08 && ramBankSelector_rtcRegisterSelector <= 0x0C) {
        *rtcRegistersMap[keep_bits<3>(ramBankSelector_rtcRegisterSelector)] = value;
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
void Mbc3<RomSize, RamSize, Battery, Timer>::loadState(Parcel& parcel) {
    ramAndTimerEnabled = parcel.readBool();
    romBankSelector = parcel.readUInt8();
    ramBankSelector_rtcRegisterSelector = parcel.readUInt8();
    latchClockData = parcel.readUInt8();
    rtcRegisters.seconds = parcel.readUInt8();
    rtcRegisters.minutes = parcel.readUInt8();
    rtcRegisters.hours = parcel.readUInt8();
    rtcRegisters.day.low = parcel.readUInt8();
    rtcRegisters.day.high = parcel.readUInt8();
    parcel.readBytes(rom, RomSize);
    if constexpr (Ram) {
        parcel.readBytes(ram, RamSize);
    }
}

template <uint32_t RomSize, uint32_t RamSize, bool Battery, bool Timer>
void Mbc3<RomSize, RamSize, Battery, Timer>::saveState(Parcel& parcel) const {
    parcel.writeBool(ramAndTimerEnabled);
    parcel.writeUInt8(romBankSelector);
    parcel.writeUInt8(ramBankSelector_rtcRegisterSelector);
    parcel.writeUInt8(latchClockData);
    parcel.writeUInt8(rtcRegisters.seconds);
    parcel.writeUInt8(rtcRegisters.minutes);
    parcel.writeUInt8(rtcRegisters.hours);
    parcel.writeUInt8(rtcRegisters.day.low);
    parcel.writeUInt8(rtcRegisters.day.high);
    parcel.writeBytes(rom, RomSize);
    if constexpr (Ram) {
        parcel.writeBytes(ram, RamSize);
    }
}
