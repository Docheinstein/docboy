#include "utils/formatters.hpp"
#include "utils/exceptions.hpp"
#include "utils/bits.hpp"
#include "utils/log.h"
#include "docboy/cartridge/helpers.h"
#include "utils/arrays.h"
#include <cstring>

/*
 * ROM access.
 *
 * A := address
 * S := romBankSelector (9 bits)
 *
 *                   | Bank number   | Address in bank
 * Accessed address  | 22:14         | 13:0
 * 0x0000 - 0x3FFF   | 0             | A[0:13]
 * 0x4000 - 0x7FFF   | S             | A[0:13]
 */

/*
 * RAM access.
 *
 * A := address
 * S := ramBankSelector (4 bits)
 *
 *                   | Bank number   | Address in bank
 * Accessed address  | 16:13         | 12:0
 * 0xA000 - 0xBFFF   | S             | A[0:12]
 */

template<uint32_t RomSize, uint32_t RamSize, bool Battery>
Mbc5<RomSize, RamSize, Battery>::Mbc5(const uint8_t *data, uint32_t length) {
    check(length <= array_size(rom), "Mbc5: actual ROM size (" + std::to_string(length) + ") exceeds nominal ROM size (" +  std::to_string(array_size(rom)) + ")");
    memcpy(rom, data, length);
}

template<uint32_t RomSize, uint32_t RamSize, bool Battery>
uint8_t Mbc5<RomSize, RamSize, Battery>::readRom(uint16_t address) const {
    check(address < 0x8000);

    // 0000 - 0x3FFF
    if (address < 0x4000)
        return rom[address];

    // 4000 - 0x7FFF
    uint32_t romAddress = romBankSelector << 14 | keep_bits<14>(address);
    romAddress = masked<RomSize>(romAddress);
    return rom[romAddress];
}

template<uint32_t RomSize, uint32_t RamSize, bool Battery>
void Mbc5<RomSize, RamSize, Battery>::writeRom(uint16_t address, uint8_t value) {
    check(address < 0x8000);

    if (address < 0x3000) {
        // 0000 - 0x1FFF
        if (address < 0x2000) {
            ramEnabled = keep_bits<4>(value) == 0xA;
            return;
        }

        // 0x2000 - 0x2FFF
        romBankSelector = keep_bits_range<15, 8>(romBankSelector) | value;
        return;
    }

    if (address < 0x6000) {
        // 0x3000 - 0x3FFF
        if (address < 0x4000) {
            romBankSelector = keep_bits<1>(value) << 8 | keep_bits<8>(romBankSelector);
            return;
        }

        // 0x4000 - 0x5FFF
        ramBankSelector = keep_bits<4>(value);
        return;
    }

    WARN("Mbc5: write at address " + hex(address) + " is ignored");
}


template<uint32_t RomSize, uint32_t RamSize, bool Battery>
uint8_t Mbc5<RomSize, RamSize, Battery>::readRam(uint16_t address) const {
    check(address >= 0xA000 && address < 0xC000);

    // 0xA000 - 0xBFFF
    if constexpr (Ram) {
        if (ramEnabled) {
            uint32_t ramAddress = (ramBankSelector << 13) | keep_bits<13>(address);
            ramAddress = masked<RamSize>(ramAddress);
            return ram[ramAddress];
        }
    }

    return 0xFF;
}

template<uint32_t RomSize, uint32_t RamSize, bool Battery>
void Mbc5<RomSize, RamSize, Battery>::writeRam(uint16_t address, uint8_t value) {
    check(address >= 0xA000 && address < 0xC000);

    // 0xA000 - 0xBFFF
    if constexpr (Ram) {
        if (ramEnabled) {
            uint32_t ramAddress = (ramBankSelector << 13) | keep_bits<13>(address);
            ramAddress = masked<RamSize>(ramAddress);
            ram[ramAddress] = value;
        }
    }
}


template<uint32_t RomSize, uint32_t RamSize, bool Battery>
uint8_t *Mbc5<RomSize, RamSize, Battery>::getRamSaveData() {
    if constexpr (Ram && Battery) {
        return ram;
    }
    return nullptr;
}

template<uint32_t RomSize, uint32_t RamSize, bool Battery>
uint32_t Mbc5<RomSize, RamSize, Battery>::getRamSaveSize() const {
    if constexpr (Ram && Battery) {
        return RamSize;
    }
    return 0;
}

#ifdef ENABLE_DEBUGGER
template<uint32_t RomSize, uint32_t RamSize, bool Battery>
uint8_t *Mbc5<RomSize, RamSize, Battery>::getRomData() {
    return rom;
}

template<uint32_t RomSize, uint32_t RamSize, bool Battery>
uint32_t Mbc5<RomSize, RamSize, Battery>::getRomSize() const {
    return RomSize;
}
#endif


template<uint32_t RomSize, uint32_t RamSize, bool Battery>
void Mbc5<RomSize, RamSize, Battery>::saveState(Parcel &parcel) const {
    parcel.writeBool(ramEnabled);
    parcel.writeUInt16(romBankSelector);
    parcel.writeUInt8(ramBankSelector);
    parcel.writeBytes(rom, RomSize);
    if constexpr (Ram) {
        parcel.writeBytes(ram, RamSize);
    }
}

template<uint32_t RomSize, uint32_t RamSize, bool Battery>
void Mbc5<RomSize, RamSize, Battery>::loadState(Parcel &parcel) {
    ramEnabled = parcel.readBool();
    romBankSelector = parcel.readUInt16();
    ramBankSelector = parcel.readUInt8();
    parcel.readBytes(rom, RomSize);
    if constexpr (Ram) {
        parcel.readBytes(ram, RamSize);
    }
}
