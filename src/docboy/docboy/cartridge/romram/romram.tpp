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
 * From Pandocs:
 * No licensed cartridge makes use of this option. The exact behavior is unknown.
 */

template <uint32_t RomSize, uint32_t RamSize, bool Battery>
RomRam<RomSize, RamSize, Battery>::RomRam(const uint8_t* data, uint32_t length) {
    check(length <= array_size(rom), "RomRam: actual ROM size (" + std::to_string(length) +
                                         ") exceeds nominal ROM size (" + std::to_string(array_size(rom)) + ")");
    memcpy(rom, data, length);
}

template <uint32_t RomSize, uint32_t RamSize, bool Battery>
uint8_t RomRam<RomSize, RamSize, Battery>::readRom(uint16_t address) const {
    check(address < 0x8000);
    return rom[address];
}

template <uint32_t RomSize, uint32_t RamSize, bool Battery>
void RomRam<RomSize, RamSize, Battery>::writeRom(uint16_t address, uint8_t value) {
    check(address < 0x8000);
}

template <uint32_t RomSize, uint32_t RamSize, bool Battery>
uint8_t RomRam<RomSize, RamSize, Battery>::readRam(uint16_t address) const {
    check(address >= 0xA000 && address < 0xC000);

    if constexpr (Ram) {
        uint32_t ramAddress = keep_bits<13>(address);
        ramAddress = masked<RamSize>(ramAddress);
        return ram[ramAddress];
    }

    return 0xFF;
}

template <uint32_t RomSize, uint32_t RamSize, bool Battery>
void RomRam<RomSize, RamSize, Battery>::writeRam(uint16_t address, uint8_t value) {
    check(address >= 0xA000 && address < 0xC000);

    if constexpr (Ram) {
        uint32_t ramAddress = keep_bits<13>(address);
        ramAddress = masked<RamSize>(ramAddress);
        ram[ramAddress] = value;
    }
}

template <uint32_t RomSize, uint32_t RamSize, bool Battery>
uint8_t* RomRam<RomSize, RamSize, Battery>::getRamSaveData() {
    if constexpr (Ram && Battery) {
        return ram;
    }
    return nullptr;
}

template <uint32_t RomSize, uint32_t RamSize, bool Battery>
uint32_t RomRam<RomSize, RamSize, Battery>::getRamSaveSize() const {
    if constexpr (Ram && Battery) {
        return RamSize;
    }
    return 0;
}

#ifdef ENABLE_DEBUGGER
template <uint32_t RomSize, uint32_t RamSize, bool Battery>
uint8_t *RomRam<RomSize, RamSize, Battery>::getRomData() {
    return rom;
}

template <uint32_t RomSize, uint32_t RamSize, bool Battery>
uint32_t RomRam<RomSize, RamSize, Battery>::getRomSize() const {
    return RomSize;
}
#endif

template <uint32_t RomSize, uint32_t RamSize, bool Battery>
void RomRam<RomSize, RamSize, Battery>::saveState(Parcel& parcel) const {
    parcel.writeBytes(rom, RomSize);
    if constexpr (Ram) {
        parcel.writeBytes(ram, RamSize);
    }
}

template <uint32_t RomSize, uint32_t RamSize, bool Battery>
void RomRam<RomSize, RamSize, Battery>::loadState(Parcel& parcel) {
    parcel.readBytes(rom, RomSize);
    if constexpr (Ram) {
        parcel.readBytes(ram, RamSize);
    }
}
