#include "docboy/cartridge/helpers.h"
#include "utils/arrays.h"
#include "utils/asserts.h"
#include "utils/bits.hpp"
#include "utils/exceptions.hpp"
#include "utils/formatters.hpp"
#include "utils/log.h"
#include "utils/mathematics.h"
#include "utils/parcel.h"
#include <cstring>
/*
 * ROM access.
 *
 * A := address
 * S := romBankSelector (4 bits)
 *
 *                   | Bank number  | Address in bank
 * Accessed address  | 17:14        | 13:0
 * 0x0000 - 0x3FFF   | 0            | A[0:13]
 * 0x4000 - 0x7FFF   | S            | A[0:13]
 */

/*
 * RAM access.
 *
 * A := address
 *
 *                   | Address in bank
 * Accessed address  | 8:0
 * 0xA000 - 0xBFFF   | A[0:8]
 */

template<uint32_t RomSize, bool Battery>
Mbc2<RomSize, Battery>::Mbc2(const uint8_t *data, uint32_t length) {
    check(length <= array_size(rom), "Mbc2: actual ROM size (" + std::to_string(length) + ") exceeds nominal ROM size (" +  std::to_string(array_size(rom)) + ")");
    memcpy(rom, data, length);
}

template<uint32_t RomSize, bool Battery>
uint8_t Mbc2<RomSize, Battery>::readRom(uint16_t address) const {
    check(address < 0x8000);

    // 0000 - 0x3FFF
    if (address < 0x4000)
        return rom[address];

    // 4000 - 0x7FFF
    uint32_t romAddress = (romBankSelector << 14) | keep_bits<14>(address);
    romAddress = masked<RomSize>(romAddress);
    return rom[romAddress];
}

template<uint32_t RomSize, bool Battery>
void Mbc2<RomSize, Battery>::writeRom(uint16_t address, uint8_t value) {
    check(address < 0x8000);

    if (address < 0x4000) {
        if (test_bit<8>(address)) {
            romBankSelector = keep_bits<4>(value);
            romBankSelector = romBankSelector > 0 ? romBankSelector : 0b1;
        } else {
            ramEnabled = keep_bits<4>(value) == 0xA;
        }
        return;
    }

    WARN("Mbc2: write at address " + hex(address) + " is ignored");
}

template<uint32_t RomSize, bool Battery>
uint8_t Mbc2<RomSize, Battery>::readRam(uint16_t address) const {
    check(address >= 0xA000 && address < 0xC000);

    // 0xA000 - 0xBFFF
    if (ramEnabled) {
        uint16_t ramAddress = keep_bits<9>(address);
        return 0xF0 | keep_bits<4>(ram[ramAddress]);
    }

    return 0xFF;
}

template<uint32_t RomSize, bool Battery>
void Mbc2<RomSize, Battery>::writeRam(uint16_t address, uint8_t value) {
    check(address >= 0xA000 && address < 0xC000);

    // 0xA000 - 0xBFFF
    if (ramEnabled) {
        uint16_t ramAddress = keep_bits<9>(address);
        ram[ramAddress] = value;
    }
}


template<uint32_t RomSize, bool Battery>
uint8_t *Mbc2<RomSize, Battery>::getRamSaveData() {
    if constexpr (Battery) {
        return ram;
    }
    return nullptr;
}

template<uint32_t RomSize, bool Battery>
uint32_t Mbc2<RomSize, Battery>::getRamSaveSize() const {
    if constexpr (Battery) {
        return RamSize;
    }
    return 0;
}


template<uint32_t RomSize, bool Battery>
void Mbc2<RomSize, Battery>::saveState(Parcel &parcel) const {
    parcel.writeBool(ramEnabled);
    parcel.writeUInt8(romBankSelector);
    parcel.writeBytes(rom, RomSize);
    parcel.writeBytes(ram, RamSize);
}

template<uint32_t RomSize, bool Battery>
void Mbc2<RomSize, Battery>::loadState(Parcel &parcel) {
    ramEnabled = parcel.readBool();
    romBankSelector = parcel.readUInt8();
    parcel.readBytes(rom, RomSize);
    parcel.readBytes(ram, RamSize);
}
