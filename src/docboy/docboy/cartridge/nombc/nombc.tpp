#include "nombc.h"
#include "utils/arrays.h"
#include "utils/asserts.h"
#include "utils/bits.hpp"
#include "utils/log.h"
#include "utils/parcel.h"
#include <cstring>

template <uint32_t RamSize>
NoMbc<RamSize>::NoMbc(const uint8_t* data, uint32_t length) {
    check(length <= array_size(rom), "NoMbc: actual ROM size (" + std::to_string(length) +
                                         ") exceeds nominal ROM size (" + std::to_string(array_size(rom)) + ")");
    memcpy(rom, data, length);
}

template <uint32_t RamSize>
uint8_t NoMbc<RamSize>::readRom(uint16_t address) const {
    return rom[address];
}

template <uint32_t RamSize>
void NoMbc<RamSize>::writeRom(uint16_t address, uint8_t value) {
    WARN("NoMbc: write at address " + hex(address) + " is ignored");
}

template <uint32_t RamSize>
uint8_t NoMbc<RamSize>::readRam(uint16_t address) const {
    check(address >= 0xA000 && address < 0xC000);

    if constexpr (Ram) {
        return ram[keep_bits<13>(address)];
    } else {
        WARN("NoMbc: read at address " + hex(address) + " is ignored");
        return 0xFF;
    }
}

template <uint32_t RamSize>
void NoMbc<RamSize>::writeRam(uint16_t address, uint8_t value) {
    check(address >= 0xA000 && address < 0xC000);

    if constexpr (Ram) {
        ram[keep_bits<13>(address)] = value;
    } else {
        WARN("NoMbc: write at address " + hex(address) + " is ignored");
    }
}

template <uint32_t RamSize>
uint8_t* NoMbc<RamSize>::getRamSaveData() {
    if constexpr (Ram) {
        return ram;
    }
    return nullptr;
}

template <uint32_t RamSize>
uint32_t NoMbc<RamSize>::getRamSaveSize() const {
    if constexpr (Ram) {
        return RamSize;
    }
    return 0;
}

#ifdef ENABLE_DEBUGGER
template <uint32_t RamSize>
uint8_t *NoMbc<RamSize>::getRomData() {
    return rom;
}

template <uint32_t RamSize>
uint32_t NoMbc<RamSize>::getRomSize() const {
    return RomSize;
}
#endif

template <uint32_t RamSize>
void NoMbc<RamSize>::saveState(Parcel& parcel) const {
    parcel.writeBytes(rom, RomSize);
    if constexpr (Ram) {
        parcel.writeBytes(ram, RamSize);
    }
}

template <uint32_t RamSize>
void NoMbc<RamSize>::loadState(Parcel& parcel) {
    parcel.readBytes(rom, RomSize);
    if constexpr (Ram) {
        parcel.readBytes(ram, RamSize);
    }
}

template <uint32_t RamSize>
void NoMbc<RamSize>::reset() {
    memset(ram, 0, RamSize);
}