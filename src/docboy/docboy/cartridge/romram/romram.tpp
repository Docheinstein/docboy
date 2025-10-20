#include <cstring>

#include "utils/arrays.h"
#include "utils/asserts.h"
#include "utils/bits.h"
#include "utils/exceptions.h"
#include "utils/formatters.h"
#include "utils/parcel.h"

/*
 * From Pandocs:
 * No licensed cartridge makes use of this option. The exact behavior is unknown.
 */

template <uint32_t RomSize, uint32_t RamSize, bool Battery>
RomRam<RomSize, RamSize, Battery>::RomRam(const uint8_t* data, uint32_t length) {
    ASSERT(length <= array_size(rom), "RomRam: actual ROM size (" + std::to_string(length) +
                                          ") exceeds nominal ROM size (" + std::to_string(array_size(rom)) + ")");
    memcpy(rom, data, length);
}

template <uint32_t RomSize, uint32_t RamSize, bool Battery>
uint8_t RomRam<RomSize, RamSize, Battery>::read_rom(uint16_t address) const {
    ASSERT(address < 0x8000);
    return rom[address];
}

template <uint32_t RomSize, uint32_t RamSize, bool Battery>
void RomRam<RomSize, RamSize, Battery>::write_rom(uint16_t address, uint8_t value) {
    ASSERT(address < 0x8000);
}

template <uint32_t RomSize, uint32_t RamSize, bool Battery>
uint8_t RomRam<RomSize, RamSize, Battery>::read_ram(uint16_t address) const {
    ASSERT(address >= 0xA000 && address < 0xC000);

    if constexpr (Ram) {
        uint32_t ram_address = keep_bits<13>(address);
        ram_address = mask_by_pow2<RamSize>(ram_address);
        return ram[ram_address];
    }

    return 0xFF;
}

template <uint32_t RomSize, uint32_t RamSize, bool Battery>
void RomRam<RomSize, RamSize, Battery>::write_ram(uint16_t address, uint8_t value) {
    ASSERT(address >= 0xA000 && address < 0xC000);

    if constexpr (Ram) {
        uint32_t ram_address = keep_bits<13>(address);
        ram_address = mask_by_pow2<RamSize>(ram_address);
        ram[ram_address] = value;
    }
}

template <uint32_t RomSize, uint32_t RamSize, bool Battery>
void* RomRam<RomSize, RamSize, Battery>::get_ram_save_data() {
    if constexpr (Ram && Battery) {
        return ram;
    }
    return nullptr;
}

template <uint32_t RomSize, uint32_t RamSize, bool Battery>
uint32_t RomRam<RomSize, RamSize, Battery>::get_ram_save_size() const {
    if constexpr (Ram && Battery) {
        return RamSize;
    }
    return 0;
}

#ifdef ENABLE_DEBUGGER
template <uint32_t RomSize, uint32_t RamSize, bool Battery>
uint8_t* RomRam<RomSize, RamSize, Battery>::get_rom_data() {
    return rom;
}

template <uint32_t RomSize, uint32_t RamSize, bool Battery>
uint32_t RomRam<RomSize, RamSize, Battery>::get_rom_size() const {
    return RomSize;
}
#endif

template <uint32_t RomSize, uint32_t RamSize, bool Battery>
void RomRam<RomSize, RamSize, Battery>::save_state(Parcel& parcel) const {
    if constexpr (Ram) {
        PARCEL_WRITE_BYTES(parcel, ram, RamSize);
    }
}

template <uint32_t RomSize, uint32_t RamSize, bool Battery>
void RomRam<RomSize, RamSize, Battery>::load_state(Parcel& parcel) {
    if constexpr (Ram) {
        parcel.read_bytes(ram, RamSize);
    }
}

template <uint32_t RomSize, uint32_t RamSize, bool Battery>
void RomRam<RomSize, RamSize, Battery>::reset() {
    memset(ram, 0, RamSize);
}