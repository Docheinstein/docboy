#include <cstring>

#include "utils/arrays.h"
#include "utils/asserts.h"
#include "utils/bits.h"
#include "utils/formatters.h"
#include "utils/parcel.h"

/*
 * M161 mapper.
 *
 * Multicart mapper used only by Mani's 4 in 1 (Tetris, Tennis, Alleyway, Yakuman).
 *
 * The mapper consists of a 74HC161A counter used as a latch: it switches the whole
 * 32 KB ROM area (0x0000 - 0x7FFF), not only 0x4000 - 0x7FFF as usual MBCs do.
 *
 * Writing to 0x0000 - 0x7FFF selects the ROM bank through the lower 3 bits of the
 * written value (the higher bits are discarded).
 *
 * Only one bank switch is allowed: the 4th input of the latch is tied high, therefore
 * after the first write (even if it selects bank 0) the latch pin can't go low anymore
 * and all the subsequent writes are ignored until the mapper is powered down.
 *
 * At startup, bank 0 (the menu) is mapped.
 *
 * There is no RAM.
 */

/*
 * ROM access.
 *
 * A := address
 * B := rom_bank_selector (3 bits)
 *
 *                   | Bank number   | Address in bank
 * Accessed address  | 17:15         | 14:0
 * 0x0000 - 0x7FFF   | B             | A[0:14]
 */

template <uint32_t RomSize>
M161<RomSize>::M161(const uint8_t* data, uint32_t length) {
    ASSERT(length <= array_size(rom), "M161: actual ROM size (" + std::to_string(length) +
                                          ") exceeds nominal ROM size (" + std::to_string(array_size(rom)) + ")");
    memcpy(rom, data, length);
}

template <uint32_t RomSize>
uint8_t M161<RomSize>::read_rom(uint16_t address) const {
    ASSERT(address < 0x8000);

    uint32_t rom_address = (rom_bank_selector << 15) | keep_bits<15>(address);
    rom_address = mask_by_pow2<RomSize>(rom_address);
    return rom[rom_address];
}

template <uint32_t RomSize>
void M161<RomSize>::write_rom(uint16_t address, uint8_t value) {
    ASSERT(address < 0x8000);

    // 0x0000 - 0x7FFF
    // Only the first write is taken into account: the subsequent ones are ignored.
    if (!locked) {
        rom_bank_selector = keep_bits<3>(value);
        locked = true;
    }
}

template <uint32_t RomSize>
uint8_t M161<RomSize>::read_ram(uint16_t address) const {
    ASSERT(address >= 0xA000 && address < 0xC000);
    return 0xFF;
}

template <uint32_t RomSize>
void M161<RomSize>::write_ram(uint16_t address, uint8_t value) {
    ASSERT(address >= 0xA000 && address < 0xC000);
}

template <uint32_t RomSize>
void* M161<RomSize>::get_ram_save_data() {
    return nullptr;
}

template <uint32_t RomSize>
uint32_t M161<RomSize>::get_ram_save_size() const {
    return 0;
}

#ifdef ENABLE_DEBUGGER
template <uint32_t RomSize>
CartridgeType M161<RomSize>::get_type() const {
    return CartridgeType::M161;
}

template <uint32_t RomSize>
uint8_t* M161<RomSize>::get_rom_data() {
    return rom;
}

template <uint32_t RomSize>
uint32_t M161<RomSize>::get_rom_size() const {
    return RomSize;
}

template <uint32_t RomSize>
uint8_t* M161<RomSize>::get_ram_data() {
    return nullptr;
}

template <uint32_t RomSize>
uint32_t M161<RomSize>::get_ram_size() const {
    return 0;
}

template <uint32_t RomSize>
uint8_t M161<RomSize>::read_rom_raw(uint16_t bank, uint16_t address) const {
    ASSERT(address < 0x8000);
    // Note: debugger banks are 16 KB wide, while M161 banks are 32 KB wide.
    uint32_t rom_address = mask_by_pow2<RomSize>((bank << 14) | keep_bits<14>(address));
    return rom[rom_address];
}

template <uint32_t RomSize>
uint16_t M161<RomSize>::get_rom_bank(uint16_t address) const {
    ASSERT(address < 0x8000);
    // Express the bank in terms of 16 KB banks, for consistency with the other MBCs.
    return (rom_bank_selector << 1) | (address >= 0x4000);
}

template <uint32_t RomSize>
uint8_t M161<RomSize>::read_ram_raw(uint16_t bank, uint16_t address) const {
    ASSERT(address >= 0xA000 && address < 0xC000);
    return 0xFF;
}

template <uint32_t RomSize>
uint16_t M161<RomSize>::get_ram_bank(uint16_t address) const {
    ASSERT(address >= 0xA000 && address < 0xC000);
    return 0;
}
#endif

template <uint32_t RomSize>
void M161<RomSize>::save_state(Parcel& parcel) const {
    PARCEL_WRITE_UINT8(parcel, rom_bank_selector);
    PARCEL_WRITE_BOOL(parcel, locked);
}

template <uint32_t RomSize>
void M161<RomSize>::load_state(Parcel& parcel) {
    rom_bank_selector = parcel.read_uint8();
    locked = parcel.read_bool();
}

template <uint32_t RomSize>
void M161<RomSize>::reset() {
    rom_bank_selector = 0;
    locked = false;
}
