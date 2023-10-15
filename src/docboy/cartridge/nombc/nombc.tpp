#include "nombc.h"
#include "utils/arrays.h"
#include "utils/asserts.h"
#include "utils/log.h"
#include "utils/parcel.h"
#include <cstring>
#include <cstring>

template <uint32_t RamSize>
NoMbc<RamSize>::NoMbc(const uint8_t* data, uint32_t length) {
    check(length <= array_size(rom), "NoMbc: actual ROM size (" + std::to_string(length) + ") exceeds nominal ROM size (" +  std::to_string(array_size(rom)) + ")");
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
    WARN("NoMbc: read at address " + hex(address) + " is ignored");
    return 0xFF;
}

template <uint32_t RamSize>
void NoMbc<RamSize>::writeRam(uint16_t address, uint8_t value) {
    WARN("NoMbc: write at address " + hex(address) + " is ignored");
}

template <uint32_t RamSize>
uint8_t* NoMbc<RamSize>::getRamSaveData() {
    return nullptr;
}

template <uint32_t RamSize>
uint32_t NoMbc<RamSize>::getRamSaveSize() const {
    return 0;
}

template <uint32_t RamSize>
void NoMbc<RamSize>::saveState(Parcel& parcel) const {
    parcel.writeBytes(rom, sizeof(rom));
    parcel.writeBytes(ram, sizeof(ram));
}

template <uint32_t RamSize>
void NoMbc<RamSize>::loadState(Parcel& parcel) {
    parcel.readBytes(rom, sizeof(rom));
    parcel.readBytes(ram, sizeof(ram));
}
