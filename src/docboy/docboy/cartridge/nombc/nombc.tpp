#include <cstring>

#include "utils/arrays.h"
#include "utils/asserts.h"
#include "utils/bits.h"
#include "utils/formatters.h"
#include "utils/parcel.h"

template <uint32_t RamSize>
NoMbc<RamSize>::NoMbc(const uint8_t* data, uint32_t length) {
    ASSERT(length <= array_size(rom), "NoMbc: actual ROM size (" + std::to_string(length) +
                                          ") exceeds nominal ROM size (" + std::to_string(array_size(rom)) + ")");
    memcpy(rom, data, length);
}

template <uint32_t RamSize>
uint8_t NoMbc<RamSize>::read_rom(uint16_t address) const {
    return rom[address];
}

template <uint32_t RamSize>
void NoMbc<RamSize>::write_rom(uint16_t address, uint8_t value) {
}

template <uint32_t RamSize>
uint8_t NoMbc<RamSize>::read_ram(uint16_t address) const {
    ASSERT(address >= 0xA000 && address < 0xC000);

    if constexpr (Ram) {
        return ram[keep_bits<13>(address)];
    } else {
        return 0xFF;
    }
}

template <uint32_t RamSize>
void NoMbc<RamSize>::write_ram(uint16_t address, uint8_t value) {
    ASSERT(address >= 0xA000 && address < 0xC000);

    if constexpr (Ram) {
        ram[keep_bits<13>(address)] = value;
    }
}

template <uint32_t RamSize>
void* NoMbc<RamSize>::get_ram_save_data() {
    if constexpr (Ram) {
        return ram;
    }
    return nullptr;
}

template <uint32_t RamSize>
uint32_t NoMbc<RamSize>::get_ram_save_size() const {
    if constexpr (Ram) {
        return RamSize;
    }
    return 0;
}

#ifdef ENABLE_DEBUGGER
template <uint32_t RamSize>
uint8_t* NoMbc<RamSize>::get_rom_data() {
    return rom;
}

template <uint32_t RamSize>
uint32_t NoMbc<RamSize>::get_rom_size() const {
    return RomSize;
}
#endif

template <uint32_t RamSize>
void NoMbc<RamSize>::save_state(Parcel& parcel) const {
    if constexpr (Ram) {
        parcel.write_bytes(ram, RamSize);
    }
}

template <uint32_t RamSize>
void NoMbc<RamSize>::load_state(Parcel& parcel) {
    if constexpr (Ram) {
        parcel.read_bytes(ram, RamSize);
    }
}

template <uint32_t RamSize>
void NoMbc<RamSize>::reset() {
    memset(ram, 0, RamSize);
}