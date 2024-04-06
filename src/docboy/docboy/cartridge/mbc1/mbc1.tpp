#include "docboy/cartridge/helpers.h"
#include "utils/arrays.h"
#include "utils/asserts.h"
#include "utils/bits.hpp"
#include "utils/exceptions.hpp"
#include "utils/formatters.hpp"
#include "utils/mathematics.h"
#include "utils/parcel.h"
#include <cstring>

/*
 * ROM access.
 *
 * A := address
 * U := upperRomBankSelector_ramBankSelector (2 bits)
 * L := romBankSelector (5 bits)
 *
 *                   |             | Bank number   | Address in bank
 * Accessed address  | bankingMode | 20:19 | 18:14 | 13:0
 * 0x0000 - 0x3FFF   | 0           | 0     | 0     | A[0:13]
 * 0x0000 - 0x3FFF   | 1           | U     | 0     | A[0:13]
 * 0x4000 - 0x7FFF   | 0/1         | U     | L     | A[0:13]
 *
 * MBC1M (multicart) works the same way as MBC1 but uses 4 bits for the RomBankSelector.
 *
 *                   |             | Bank number   | Address in bank
 * Accessed address  | bankingMode | 19:18 | 17:14 | 13:0
 * 0x0000 - 0x3FFF   | 0           | 0     | 0     | A[0:13]
 * 0x0000 - 0x3FFF   | 1           | U     | 0     | A[0:13]
 * 0x4000 - 0x7FFF   | 0/1         | U     | L     | A[0:13]
 */

/*
 * RAM access.
 *
 * A := address
 * U := upperRomBankSelector_ramBankSelector (2 bits)
 *
 *                   |             | Bank number   | Address in bank
 * Accessed address  | bankingMode | 14:13         | 12:0
 * 0xA000 - 0xBFFF   | 0           | 0             | A[0:12]
 * 0xA000 - 0xBFFF   | 1           | U             | A[0:12]
 */

template <uint32_t RomSize, uint32_t RamSize, bool Battery, uint8_t RomBankSelectorBits>
Mbc1<RomSize, RamSize, Battery, RomBankSelectorBits>::Mbc1(const uint8_t* data, uint32_t length) {
    check(length <= array_size(rom), "Mbc1: actual ROM size (" + std::to_string(length) +
                                         ") exceeds nominal ROM size (" + std::to_string(array_size(rom)) + ")");
    memcpy(rom, data, length);
}

template <uint32_t RomSize, uint32_t RamSize, bool Battery, uint8_t RomBankSelectorBits>
uint8_t Mbc1<RomSize, RamSize, Battery, RomBankSelectorBits>::readRom(uint16_t address) const {
    check(address < 0x8000);

    uint32_t romBankAddress {};

    if (address < 0x4000) {
        // 0000 - 0x3FFF
        if (bankingMode == 0b1)
            romBankAddress = upperRomBankSelector_ramBankSelector << RomBankSelectorBits;
    } else {
        // 4000 - 0x7FFF
        check(romBankSelector != 0 || RomBankSelectorBits < 5);
        romBankAddress = upperRomBankSelector_ramBankSelector << RomBankSelectorBits | romBankSelector;
    }

    uint32_t romAddress = (romBankAddress << 14) | keep_bits<14>(address);
    romAddress = masked<RomSize>(romAddress);
    return rom[romAddress];
}

template <uint32_t RomSize, uint32_t RamSize, bool Battery, uint8_t RomBankSelectorBits>
void Mbc1<RomSize, RamSize, Battery, RomBankSelectorBits>::writeRom(uint16_t address, uint8_t value) {
    check(address < 0x8000);

    if (address < 0x4000) {
        // 0000 - 0x1FFF
        if (address < 0x2000) {
            ramEnabled = keep_bits<4>(value) == 0xA;
            return;
        }
        // 0x2000 - 0x3FFF
        romBankSelector = keep_bits<5 /* always 5 bits: not related to RomBankSelectorBits */>(value) > 0
                              ? keep_bits<RomBankSelectorBits>(value)
                              : 0b1;
        return;
    }

    // 0x4000 - 0x5FFF
    if (address < 0x6000) {
        upperRomBankSelector_ramBankSelector = keep_bits<2>(value);
        return;
    }
    // 0x6000 - 0x7FFF
    bankingMode = keep_bits<1>(value);
}

template <uint32_t RomSize, uint32_t RamSize, bool Battery, uint8_t RomBankSelectorBits>
uint8_t Mbc1<RomSize, RamSize, Battery, RomBankSelectorBits>::readRam(uint16_t address) const {
    check(address >= 0xA000 && address < 0xC000);

    // 0xA000 - 0xBFFF
    if constexpr (Ram) {
        if (ramEnabled) {
            uint32_t ramAddress = keep_bits<13>(address);
            if (bankingMode == 0x1) {
                const uint32_t ramBankAddress = (upperRomBankSelector_ramBankSelector << 13);
                ramAddress = ramBankAddress | ramAddress;
            }
            ramAddress = masked<RamSize>(ramAddress);
            return ram[ramAddress];
        }
    }

    return 0xFF;
}

template <uint32_t RomSize, uint32_t RamSize, bool Battery, uint8_t RomBankSelectorBits>
void Mbc1<RomSize, RamSize, Battery, RomBankSelectorBits>::writeRam(uint16_t address, uint8_t value) {
    check(address >= 0xA000 && address < 0xC000);

    // 0xA000 - 0xBFFF
    if constexpr (Ram) {
        if (ramEnabled) {
            uint32_t ramAddress = keep_bits<13>(address);
            if (bankingMode == 0x1) {
                const uint32_t ramBankAddress = (upperRomBankSelector_ramBankSelector << 13);
                ramAddress = ramBankAddress | ramAddress;
            }
            ramAddress = masked<RamSize>(ramAddress);
            ram[ramAddress] = value;
        }
    }
}

template <uint32_t RomSize, uint32_t RamSize, bool Battery, uint8_t RomBankSelectorBits>
uint8_t* Mbc1<RomSize, RamSize, Battery, RomBankSelectorBits>::getRamSaveData() {
    if constexpr (Ram && Battery) {
        return ram;
    }
    return nullptr;
}

template <uint32_t RomSize, uint32_t RamSize, bool Battery, uint8_t RomBankSelectorBits>
uint32_t Mbc1<RomSize, RamSize, Battery, RomBankSelectorBits>::getRamSaveSize() const {
    if constexpr (Ram && Battery) {
        return RamSize;
    }
    return 0;
}

#ifdef ENABLE_DEBUGGER
template <uint32_t RomSize, uint32_t RamSize, bool Battery, uint8_t RomBankSelectorBits>
uint8_t *Mbc1<RomSize, RamSize, Battery, RomBankSelectorBits>::getRomData() {
    return rom;
}

template <uint32_t RomSize, uint32_t RamSize, bool Battery, uint8_t RomBankSelectorBits>
uint32_t Mbc1<RomSize, RamSize, Battery, RomBankSelectorBits>::getRomSize() const {
    return RomSize;
}
#endif

template <uint32_t RomSize, uint32_t RamSize, bool Battery, uint8_t RomBankSelectorBits>
void Mbc1<RomSize, RamSize, Battery, RomBankSelectorBits>::saveState(Parcel& parcel) const {
    parcel.writeBool(ramEnabled);
    parcel.writeUInt8(romBankSelector);
    parcel.writeUInt8(upperRomBankSelector_ramBankSelector);
    parcel.writeUInt8(bankingMode);
    parcel.writeBytes(rom, RomSize);
    if constexpr (Ram) {
        parcel.writeBytes(ram, RamSize);
    }
}

template <uint32_t RomSize, uint32_t RamSize, bool Battery, uint8_t RomBankSelectorBits>
void Mbc1<RomSize, RamSize, Battery, RomBankSelectorBits>::loadState(Parcel& parcel) {
    ramEnabled = parcel.readBool();
    romBankSelector = parcel.readUInt8();
    upperRomBankSelector_ramBankSelector = parcel.readUInt8();
    bankingMode = parcel.readUInt8();
    parcel.readBytes(rom, RomSize);
    if constexpr (Ram) {
        parcel.readBytes(ram, RamSize);
    }
}
