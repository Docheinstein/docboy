#include "mbc3.h"
#include "utils/binutils.h"
#include "utils/timeutils.h"
#include <chrono>

MBC3::MBC3(const std::vector<uint8_t>& data) :
    Cartridge(data),
    rtcRegistersMap {
        /* 08 */ &rtcRegisters.seconds,
        /* 09 */ &rtcRegisters.minutes,
        /* 0A */ &rtcRegisters.hours,
        /* 0B */ &rtcRegisters.day.low,
        /* 0C */ &rtcRegisters.day.high,
    } {
}

MBC3::MBC3(std::vector<uint8_t>&& data) :
    Cartridge(data),
    // TODO: are two constructors necessary?
    rtcRegistersMap {
        /* 08 */ &rtcRegisters.seconds,
        /* 09 */ &rtcRegisters.minutes,
        /* 0A */ &rtcRegisters.hours,
        /* 0B */ &rtcRegisters.day.low,
        /* 0C */ &rtcRegisters.day.high,
    } {
}

void MBC3::loadState(IReadableState& state) {
    ramAndTimerEnabled = state.readBool();
    romBankSelector = state.readUInt8();
    ramBankSelector_rtcRegisterSelector = state.readUInt8();
}

void MBC3::saveState(IWritableState& state) {
    state.writeBool(ramAndTimerEnabled);
    state.writeUInt8(romBankSelector);
    state.writeUInt8(ramBankSelector_rtcRegisterSelector);
}

uint8_t MBC3::read(uint16_t address) const {
    /*
     *  Bits: 20 19 18 17 16 15 14 13 12 .. 01 00
     *        \___/ \____________/ \____________/
     *          |          |            \----------- From Game Boy address
     *          |          \------------------------ As 2000–3FFF bank register
     *          \----------------------------------- As 4000–5FFF bank register
     */
    if (address < 0x4000) {
        return rom[address];
    }
    if (address < 0x8000) {
        uint16_t bankAddress = address - 0x4000;
        uint8_t bankSelector = romBankSelector;
        // TODO: what?
        // if (bankingMode == 0x0)
        // bankSelector = bankSelector | (upperRomBankSelector_ramBankSelector << 5);
        size_t romAddress = (bankSelector << 14) + bankAddress;

        return rom[romAddress];
    }
    if (address >= 0xA000 && address < 0xC000) {
        if (ramBankSelector_rtcRegisterSelector < 0x4) {
            // TODO: error? must be implemented by subclass
        } else {
            if (ramBankSelector_rtcRegisterSelector >= 0x08 && ramBankSelector_rtcRegisterSelector <= 0x0C)
                return *rtcRegistersMap[ramBankSelector_rtcRegisterSelector - 0x08];
            // TODO: error? must be implemented by subclass
        }
    }

    throw ReadMemoryException("Read at address " + hex(address) + " is not allowed");
}

void MBC3::write(uint16_t address, uint8_t value) {
    if (address < 0x2000) {
        ramAndTimerEnabled = value == 0xA;
    } else if (address < 0x4000) {
        romBankSelector = keep_bits<7>(value > 0 ? value : 0x1);
    } else if (address < 0x6000) {
        ramBankSelector_rtcRegisterSelector = keep_bits<4>(value);
    } else if (address < 0x8000) {
        if (latchClockData == 0x0 && value == 0x1) {
            auto datetime = get_current_datetime();
            rtcRegisters.seconds = datetime.tm_sec;
            rtcRegisters.minutes = datetime.tm_min;
            rtcRegisters.hours = datetime.tm_hour;
            // TODO...
            rtcRegisters.day.low = datetime.tm_yday;
        }
        latchClockData = value;
    } else if (address >= 0xA000 && address < 0xC000) {
        if (ramBankSelector_rtcRegisterSelector < 0x4) {
            // TODO: error? must be implemented by subclass
        } else {
            if (ramBankSelector_rtcRegisterSelector >= 0x08 && ramBankSelector_rtcRegisterSelector <= 0x0C)
                *rtcRegistersMap[ramBankSelector_rtcRegisterSelector - 0x08] = value;
            // TODO: error? must be implemented by subclass
        }
    }

    throw WriteMemoryException("Write at address " + hex(address) + " is not allowed");
}