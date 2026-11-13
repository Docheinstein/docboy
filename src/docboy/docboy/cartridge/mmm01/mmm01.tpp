#include <cstring>

#include "utils/arrays.h"
#include "utils/asserts.h"
#include "utils/bits.h"
#include "utils/formatters.h"
#include "utils/parcel.h"

/*
 * MMM01 mapper.
 *
 * Multicart mapper that emulates MBC1 for the contained games.
 *
 * At startup the mapper is "unmapped": the last 32 KB of the ROM (containing the menu)
 * are mapped to 0x0000 - 0x7FFF, regardless of the registers.
 * The menu configures the base bank and the size of the selected game through
 * some extended register bits, then sets the "Mapping Enable" bit: from now on the
 * cartridge is "mapped", the extended bits are frozen and the mapper behaves like
 * an MBC1 restricted to the game's ROM/RAM area (until power down).
 *
 * Registers.
 *
 * 0x0000 - 0x1FFF
 *   Bit 6    : Mapping Enable               (unmapped only)
 *   Bit 5-4  : RAM Bank Mask                (unmapped only)
 *   Bit 3-0  : RAM Enable (0xA enables)
 *
 * 0x2000 - 0x3FFF
 *   Bit 6-5  : ROM Bank Mid                 (unmapped only)
 *   Bit 4-0  : ROM Bank Low
 *
 * 0x4000 - 0x5FFF
 *   Bit 6    : MBC1 Mode Write Lock         (unmapped only)
 *   Bit 5-4  : ROM Bank High                (unmapped only)
 *   Bit 3-2  : RAM Bank High                (unmapped only)
 *   Bit 1-0  : RAM Bank Low
 *
 * 0x6000 - 0x7FFF
 *   Bit 6    : Multiplex Enable             (unmapped only)
 *   Bit 5-2  : ROM Bank Mask                (unmapped only)
 *   Bit 1    : ROM Bank Mask (ignored)      (unmapped only)
 *   Bit 0    : MBC1 Mode                    (locked by MBC1 Mode Write Lock)
 *
 * The bits set in ROM Bank Mask (resp. RAM Bank Mask) prevent writes to the
 * matching bits of ROM Bank Low (resp. RAM Bank Low): bit N + 1 of ROM Bank Mask
 * locks bit N of ROM Bank Low, bit N of RAM Bank Mask locks bit N of RAM Bank Low.
 *
 * When multiplex is enabled, the roles of ROM Bank Mid and RAM Bank Low are swapped,
 * and RAM Bank Mask applies to ROM Bank Mid instead.
 */

/*
 * ROM access.
 *
 * A := address
 * H := rom_bank_high (2 bits)
 * M := rom_bank_mid (2 bits) [ram_bank_low if multiplex]
 * L := rom_bank_low (5 bits)
 * K := rom_bank_mask (applied to L)
 * R := ram_bank_mask (applied to M)
 * L' := L, with bit 0 forced to 1 if the unmasked bits of L are 0
 *
 *                   |          |           |      | Bank number                   | Address in bank
 * Accessed address  | mapped   | multiplex | mode | 22:21 | 20:19   | 18:14       | 13:0
 * 0x0000 - 0x7FFF   | 0        | -         | -    | Last 32 KB of the ROM
 * 0x0000 - 0x3FFF   | 1        | 0         | -    | H     | M       | L & K       | A[0:13]
 * 0x0000 - 0x3FFF   | 1        | 1         | 0    | H     | M & R   | L & K       | A[0:13]
 * 0x0000 - 0x3FFF   | 1        | 1         | 1    | H     | M       | L & K       | A[0:13]
 * 0x4000 - 0x7FFF   | 1        | -         | -    | H     | M       | L'          | A[0:13]
 */

/*
 * RAM access.
 *
 * A := address
 * H := ram_bank_high (2 bits)
 * L := ram_bank_low (2 bits) [rom_bank_mid if multiplex]
 * R := ram_bank_mask (applied to L)
 *
 *                   |           |      | Bank number   | Address in bank
 * Accessed address  | multiplex | mode | 16:15 | 14:13 | 12:0
 * 0xA000 - 0xBFFF   | 0         | 0    | H     | L & R | A[0:12]
 * 0xA000 - 0xBFFF   | 0         | 1    | H     | L     | A[0:12]
 * 0xA000 - 0xBFFF   | 1         | -    | H     | L     | A[0:12]
 */

template <uint32_t RomSize, uint32_t RamSize, bool Battery>
MMM01<RomSize, RamSize, Battery>::MMM01(const uint8_t* data, uint32_t length) {
    ASSERT(length <= array_size(rom), "MMM01: actual ROM size (" + std::to_string(length) +
                                          ") exceeds nominal ROM size (" + std::to_string(array_size(rom)) + ")");
    memcpy(rom, data, length);
}

template <uint32_t RomSize, uint32_t RamSize, bool Battery>
uint16_t MMM01<RomSize, RamSize, Battery>::get_rom_bank_0000_3FFF() const {
    ASSERT(mapped);

    uint8_t mid = multiplex ? ram_bank_low : rom_bank_mid;
    if (multiplex && banking_mode == 0) {
        mid &= ram_bank_mask;
    }

    const uint8_t low = rom_bank_low & (rom_bank_mask << 1);

    return rom_bank_high << 7 | mid << 5 | low;
}

template <uint32_t RomSize, uint32_t RamSize, bool Battery>
uint16_t MMM01<RomSize, RamSize, Battery>::get_rom_bank_4000_7FFF() const {
    ASSERT(mapped);

    const uint8_t mid = multiplex ? ram_bank_low : rom_bank_mid;

    uint8_t low = rom_bank_low;
    if ((low & ~(rom_bank_mask << 1) & bitmask<5>) == 0) {
        // Bank 0 of the game is remapped to bank 1.
        low |= 0x1;
    }

    return rom_bank_high << 7 | mid << 5 | low;
}

template <uint32_t RomSize, uint32_t RamSize, bool Battery>
uint8_t MMM01<RomSize, RamSize, Battery>::get_ram_bank_A000_BFFF() const {
    uint8_t low = multiplex ? rom_bank_mid : ram_bank_low;
    if (!multiplex && banking_mode == 0) {
        low &= ram_bank_mask;
    }

    return ram_bank_high << 2 | low;
}

template <uint32_t RomSize, uint32_t RamSize, bool Battery>
uint8_t MMM01<RomSize, RamSize, Battery>::read_rom(uint16_t address) const {
    ASSERT(address < 0x8000);

    if (!mapped) {
        // The last 32 KB of the ROM (menu) are mapped to 0x0000 - 0x7FFF.
        return rom[RomSize - 0x8000 + address];
    }

    const uint32_t rom_bank_address = address < 0x4000 ? get_rom_bank_0000_3FFF() : get_rom_bank_4000_7FFF();

    uint32_t rom_address = (rom_bank_address << 14) | keep_bits<14>(address);
    rom_address = mask_by_pow2<RomSize>(rom_address);
    return rom[rom_address];
}

template <uint32_t RomSize, uint32_t RamSize, bool Battery>
void MMM01<RomSize, RamSize, Battery>::write_rom(uint16_t address, uint8_t value) {
    ASSERT(address < 0x8000);

    if (address < 0x4000) {
        // 0x0000 - 0x1FFF
        if (address < 0x2000) {
            ram_enabled = keep_bits<4>(value) == 0xA;
            if (!mapped) {
                ram_bank_mask = get_bits_range<5, 4>(value);
                mapped = test_bit<6>(value);
            }
            return;
        }

        // 0x2000 - 0x3FFF
        // Bits locked by the ROM bank mask can't be changed.
        const uint8_t rom_bank_low_lock = rom_bank_mask << 1;
        rom_bank_low = (rom_bank_low & rom_bank_low_lock) | (keep_bits<5>(value) & ~rom_bank_low_lock);
        if (!mapped) {
            rom_bank_mid = get_bits_range<6, 5>(value);
        }
        return;
    }

    // 0x4000 - 0x5FFF
    if (address < 0x6000) {
        // Bits locked by the RAM bank mask can't be changed.
        ram_bank_low = (ram_bank_low & ram_bank_mask) | (keep_bits<2>(value) & ~ram_bank_mask);
        if (!mapped) {
            ram_bank_high = get_bits_range<3, 2>(value);
            rom_bank_high = get_bits_range<5, 4>(value);
            mode_write_lock = test_bit<6>(value);
        }
        return;
    }

    // 0x6000 - 0x7FFF
    if (!mode_write_lock) {
        banking_mode = keep_bits<1>(value);
    }
    if (!mapped) {
        // Bit 1 of the mask is ignored: bit 0 of the ROM bank low is always writable.
        rom_bank_mask = get_bits_range<5, 2>(value);
        multiplex = test_bit<6>(value);
    }
}

template <uint32_t RomSize, uint32_t RamSize, bool Battery>
uint8_t MMM01<RomSize, RamSize, Battery>::read_ram(uint16_t address) const {
    ASSERT(address >= 0xA000 && address < 0xC000);

    // 0xA000 - 0xBFFF
    if constexpr (Ram) {
        if (ram_enabled) {
            uint32_t ram_address = (get_ram_bank_A000_BFFF() << 13) | keep_bits<13>(address);
            ram_address = mask_by_pow2<RamSize>(ram_address);
            return ram[ram_address];
        }
    }

    // TODO: open bus read is not guaranteed to be 0xFF.
    return 0xFF;
}

template <uint32_t RomSize, uint32_t RamSize, bool Battery>
void MMM01<RomSize, RamSize, Battery>::write_ram(uint16_t address, uint8_t value) {
    ASSERT(address >= 0xA000 && address < 0xC000);

    // 0xA000 - 0xBFFF
    if constexpr (Ram) {
        if (ram_enabled) {
            uint32_t ram_address = (get_ram_bank_A000_BFFF() << 13) | keep_bits<13>(address);
            ram_address = mask_by_pow2<RamSize>(ram_address);
            ram[ram_address] = value;
        }
    }
}

template <uint32_t RomSize, uint32_t RamSize, bool Battery>
void* MMM01<RomSize, RamSize, Battery>::get_ram_save_data() {
    if constexpr (Ram && Battery) {
        return ram;
    }
    return nullptr;
}

template <uint32_t RomSize, uint32_t RamSize, bool Battery>
uint32_t MMM01<RomSize, RamSize, Battery>::get_ram_save_size() const {
    if constexpr (Ram && Battery) {
        return RamSize;
    }
    return 0;
}

#ifdef ENABLE_DEBUGGER
template <uint32_t RomSize, uint32_t RamSize, bool Battery>
CartridgeType MMM01<RomSize, RamSize, Battery>::get_type() const {
    return CartridgeType::MMM01;
}

template <uint32_t RomSize, uint32_t RamSize, bool Battery>
uint8_t* MMM01<RomSize, RamSize, Battery>::get_rom_data() {
    return rom;
}

template <uint32_t RomSize, uint32_t RamSize, bool Battery>
uint32_t MMM01<RomSize, RamSize, Battery>::get_rom_size() const {
    return RomSize;
}

template <uint32_t RomSize, uint32_t RamSize, bool Battery>
uint8_t* MMM01<RomSize, RamSize, Battery>::get_ram_data() {
    return ram;
}

template <uint32_t RomSize, uint32_t RamSize, bool Battery>
uint32_t MMM01<RomSize, RamSize, Battery>::get_ram_size() const {
    return RamSize;
}

template <uint32_t RomSize, uint32_t RamSize, bool Battery>
uint8_t MMM01<RomSize, RamSize, Battery>::read_rom_raw(uint16_t bank, uint16_t address) const {
    ASSERT(address < 0x8000);
    uint32_t rom_address = mask_by_pow2<RomSize>((bank << 14) | keep_bits<14>(address));
    return rom[rom_address];
}

template <uint32_t RomSize, uint32_t RamSize, bool Battery>
uint16_t MMM01<RomSize, RamSize, Battery>::get_rom_bank(uint16_t address) const {
    ASSERT(address < 0x8000);

    if (!mapped) {
        return ((RomSize - 0x8000) >> 14) | (address >= 0x4000);
    }

    return address < 0x4000 ? get_rom_bank_0000_3FFF() : get_rom_bank_4000_7FFF();
}

template <uint32_t RomSize, uint32_t RamSize, bool Battery>
uint8_t MMM01<RomSize, RamSize, Battery>::read_ram_raw(uint16_t bank, uint16_t address) const {
    ASSERT(address >= 0xA000 && address < 0xC000);

    if constexpr (Ram) {
        uint32_t ram_address = mask_by_pow2<RamSize>((bank << 13) | keep_bits<13>(address));
        return ram[ram_address];
    }

    return 0xFF;
}

template <uint32_t RomSize, uint32_t RamSize, bool Battery>
uint16_t MMM01<RomSize, RamSize, Battery>::get_ram_bank(uint16_t address) const {
    ASSERT(address >= 0xA000 && address < 0xC000);
    return get_ram_bank_A000_BFFF();
}
#endif

template <uint32_t RomSize, uint32_t RamSize, bool Battery>
void MMM01<RomSize, RamSize, Battery>::save_state(Parcel& parcel) const {
    PARCEL_WRITE_BOOL(parcel, mapped);
    PARCEL_WRITE_BOOL(parcel, ram_enabled);
    PARCEL_WRITE_BOOL(parcel, mode_write_lock);
    PARCEL_WRITE_BOOL(parcel, multiplex);
    PARCEL_WRITE_UINT8(parcel, banking_mode);
    PARCEL_WRITE_UINT8(parcel, rom_bank_low);
    PARCEL_WRITE_UINT8(parcel, rom_bank_mid);
    PARCEL_WRITE_UINT8(parcel, rom_bank_high);
    PARCEL_WRITE_UINT8(parcel, rom_bank_mask);
    PARCEL_WRITE_UINT8(parcel, ram_bank_low);
    PARCEL_WRITE_UINT8(parcel, ram_bank_high);
    PARCEL_WRITE_UINT8(parcel, ram_bank_mask);
    if constexpr (Ram) {
        PARCEL_WRITE_BYTES(parcel, ram, RamSize);
    }
}

template <uint32_t RomSize, uint32_t RamSize, bool Battery>
void MMM01<RomSize, RamSize, Battery>::load_state(Parcel& parcel) {
    mapped = parcel.read_bool();
    ram_enabled = parcel.read_bool();
    mode_write_lock = parcel.read_bool();
    multiplex = parcel.read_bool();
    banking_mode = parcel.read_uint8();
    rom_bank_low = parcel.read_uint8();
    rom_bank_mid = parcel.read_uint8();
    rom_bank_high = parcel.read_uint8();
    rom_bank_mask = parcel.read_uint8();
    ram_bank_low = parcel.read_uint8();
    ram_bank_high = parcel.read_uint8();
    ram_bank_mask = parcel.read_uint8();
    if constexpr (Ram) {
        parcel.read_bytes(ram, RamSize);
    }
}

template <uint32_t RomSize, uint32_t RamSize, bool Battery>
void MMM01<RomSize, RamSize, Battery>::reset() {
    mapped = false;
    ram_enabled = false;
    mode_write_lock = false;
    multiplex = false;
    banking_mode = 0;
    rom_bank_low = 0;
    rom_bank_mid = 0;
    rom_bank_high = 0;
    rom_bank_mask = 0;
    ram_bank_low = 0;
    ram_bank_high = 0;
    ram_bank_mask = 0;
    memset(ram, 0xFF, RamSize);
}
