#include <cstring>

#include "utils/arrays.h"
#include "utils/asserts.h"
#include "utils/bits.h"
#include "utils/formatters.h"
#include "utils/parcel.h"

/*
 * Wisdom Tree mapper.
 *
 * The mapper is made of a single 74LS377 octal latch that switches the whole
 * 32 KB ROM area (0x0000 - 0x7FFF), not only 0x4000 - 0x7FFF as usual MBCs do.
 *
 * Since the latch is clocked on the rising edge of the write pulse, when the data bus
 * is no longer valid, the bank number is taken from the address lines A7-A0
 * instead of the data lines: to select bank $XX, any value is written to $YYXX
 * (the written value is ignored).
 *
 * There is no RAM.
 */

/*
 * ROM access.
 *
 * A := address
 * B := rom_bank_selector (8 bits)
 *
 *                   | Bank number   | Address in bank
 * Accessed address  | 22:15         | 14:0
 * 0x0000 - 0x7FFF   | B             | A[0:14]
 */

template <uint32_t RomSize>
WisdomTree<RomSize>::WisdomTree(const uint8_t* data, uint32_t length) {
    ASSERT(length <= array_size(rom), "WisdomTree: actual ROM size (" + std::to_string(length) +
                                          ") exceeds nominal ROM size (" + std::to_string(array_size(rom)) + ")");
    memcpy(rom, data, length);
}

template <uint32_t RomSize>
uint8_t WisdomTree<RomSize>::read_rom(uint16_t address) const {
    ASSERT(address < 0x8000);

    uint32_t rom_address = (rom_bank_selector << 15) | keep_bits<15>(address);
    rom_address = mask_by_pow2<RomSize>(rom_address);
    return rom[rom_address];
}

template <uint32_t RomSize>
void WisdomTree<RomSize>::write_rom(uint16_t address, uint8_t value) {
    ASSERT(address < 0x8000);

    // 0x0000 - 0x7FFF
    // The bank is selected by the lower byte of the address, the written value is ignored.
    rom_bank_selector = keep_bits<8>(address);
}

template <uint32_t RomSize>
uint8_t WisdomTree<RomSize>::read_ram(uint16_t address) const {
    ASSERT(address >= 0xA000 && address < 0xC000);
    return 0xFF;
}

template <uint32_t RomSize>
void WisdomTree<RomSize>::write_ram(uint16_t address, uint8_t value) {
    ASSERT(address >= 0xA000 && address < 0xC000);
}

template <uint32_t RomSize>
void* WisdomTree<RomSize>::get_ram_save_data() {
    return nullptr;
}

template <uint32_t RomSize>
uint32_t WisdomTree<RomSize>::get_ram_save_size() const {
    return 0;
}

#ifdef ENABLE_DEBUGGER
template <uint32_t RomSize>
uint8_t* WisdomTree<RomSize>::get_rom_data() {
    return rom;
}

template <uint32_t RomSize>
uint32_t WisdomTree<RomSize>::get_rom_size() const {
    return RomSize;
}

template <uint32_t RomSize>
uint8_t* WisdomTree<RomSize>::get_ram_data() {
    return nullptr;
}

template <uint32_t RomSize>
uint32_t WisdomTree<RomSize>::get_ram_size() const {
    return 0;
}

template <uint32_t RomSize>
uint8_t WisdomTree<RomSize>::read_rom_raw(uint16_t bank, uint16_t address) const {
    ASSERT(address < 0x8000);
    // Note: debugger banks are 16 KB wide, while Wisdom Tree banks are 32 KB wide.
    uint32_t rom_address = mask_by_pow2<RomSize>((bank << 14) | keep_bits<14>(address));
    return rom[rom_address];
}

template <uint32_t RomSize>
uint16_t WisdomTree<RomSize>::get_rom_bank(uint16_t address) const {
    ASSERT(address < 0x8000);
    // Express the bank in terms of 16 KB banks, for consistency with the other MBCs.
    return (rom_bank_selector << 1) | (address >= 0x4000);
}

template <uint32_t RomSize>
uint8_t WisdomTree<RomSize>::read_ram_raw(uint16_t bank, uint16_t address) const {
    ASSERT(address >= 0xA000 && address < 0xC000);
    return 0xFF;
}

template <uint32_t RomSize>
uint16_t WisdomTree<RomSize>::get_ram_bank(uint16_t address) const {
    ASSERT(address >= 0xA000 && address < 0xC000);
    return 0;
}
#endif

template <uint32_t RomSize>
void WisdomTree<RomSize>::save_state(Parcel& parcel) const {
    PARCEL_WRITE_UINT8(parcel, rom_bank_selector);
}

template <uint32_t RomSize>
void WisdomTree<RomSize>::load_state(Parcel& parcel) {
    rom_bank_selector = parcel.read_uint8();
}

template <uint32_t RomSize>
void WisdomTree<RomSize>::reset() {
    rom_bank_selector = 0;
}
