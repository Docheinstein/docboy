#include <cstring>

#include "utils/arrays.h"
#include "utils/bits.h"
#include "utils/exceptions.h"
#include "utils/formatters.h"

/*
 * ROM access.
 *
 * A := address
 * S := rom_bank_selector (9 bits)
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
 * S := ram_bank_selector (4 bits)
 *
 *                   | Bank number   | Address in bank
 * Accessed address  | 16:13         | 12:0
 * 0xA000 - 0xBFFF   | S             | A[0:12]
 */

template<uint32_t RomSize, uint32_t RamSize, bool Battery>
Mbc5<RomSize, RamSize, Battery>::Mbc5(const uint8_t *data, uint32_t length) {
    ASSERT(length <= array_size(rom), "Mbc5: actual ROM size (" + std::to_string(length) + ") exceeds nominal ROM size (" +  std::to_string(array_size(rom)) + ")");
    memcpy(rom, data, length);
}

template<uint32_t RomSize, uint32_t RamSize, bool Battery>
uint8_t Mbc5<RomSize, RamSize, Battery>::read_rom(uint16_t address) const {
    ASSERT(address < 0x8000);

    // 0000 - 0x3FFF
    if (address < 0x4000) {
        return rom[address];
    }

    // 4000 - 0x7FFF
    uint32_t rom_address = rom_bank_selector << 14 | keep_bits<14>(address);
    rom_address = mask_by_pow2<RomSize>(rom_address);
    return rom[rom_address];
}

template<uint32_t RomSize, uint32_t RamSize, bool Battery>
void Mbc5<RomSize, RamSize, Battery>::write_rom(uint16_t address, uint8_t value) {
    ASSERT(address < 0x8000);

    if (address < 0x3000) {
        // 0000 - 0x1FFF
        if (address < 0x2000) {
            ram_enabled = keep_bits<4>(value) == 0xA;
            return;
        }

        // 0x2000 - 0x2FFF
        rom_bank_selector = keep_bits_range<15, 8>(rom_bank_selector) | value;
        return;
    }

    if (address < 0x6000) {
        // 0x3000 - 0x3FFF
        if (address < 0x4000) {
            rom_bank_selector = concat(keep_bits<1>(value) ,keep_bits<8>(rom_bank_selector));
            return;
        }

        // 0x4000 - 0x5FFF
        ram_bank_selector = keep_bits<4>(value);
        return;
    }
}


template<uint32_t RomSize, uint32_t RamSize, bool Battery>
uint8_t Mbc5<RomSize, RamSize, Battery>::read_ram(uint16_t address) const {
    ASSERT(address >= 0xA000 && address < 0xC000);

    // 0xA000 - 0xBFFF
    if constexpr (Ram) {
        if (ram_enabled) {
            uint32_t ram_address = (ram_bank_selector << 13) | keep_bits<13>(address);
            ram_address = mask_by_pow2<RamSize>(ram_address);
            return ram[ram_address];
        }
    }

    return 0xFF;
}

template<uint32_t RomSize, uint32_t RamSize, bool Battery>
void Mbc5<RomSize, RamSize, Battery>::write_ram(uint16_t address, uint8_t value) {
    ASSERT(address >= 0xA000 && address < 0xC000);

    // 0xA000 - 0xBFFF
    if constexpr (Ram) {
        if (ram_enabled) {
            uint32_t ram_address = (ram_bank_selector << 13) | keep_bits<13>(address);
            ram_address = mask_by_pow2<RamSize>(ram_address);
            ram[ram_address] = value;
        }
    }
}


template<uint32_t RomSize, uint32_t RamSize, bool Battery>
uint8_t *Mbc5<RomSize, RamSize, Battery>::get_ram_save_data() {
    if constexpr (Ram && Battery) {
        return ram;
    }
    return nullptr;
}

template<uint32_t RomSize, uint32_t RamSize, bool Battery>
uint32_t Mbc5<RomSize, RamSize, Battery>::get_ram_save_size() const {
    if constexpr (Ram && Battery) {
        return RamSize;
    }
    return 0;
}

#ifdef ENABLE_DEBUGGER
template<uint32_t RomSize, uint32_t RamSize, bool Battery>
uint8_t *Mbc5<RomSize, RamSize, Battery>::get_rom_data() {
    return rom;
}

template<uint32_t RomSize, uint32_t RamSize, bool Battery>
uint32_t Mbc5<RomSize, RamSize, Battery>::get_rom_size() const {
    return RomSize;
}
#endif


template<uint32_t RomSize, uint32_t RamSize, bool Battery>
void Mbc5<RomSize, RamSize, Battery>::save_state(Parcel &parcel) const {
    parcel.write_bool(ram_enabled);
    parcel.write_uint16(rom_bank_selector);
    parcel.write_uint8(ram_bank_selector);
    parcel.write_bytes(rom, RomSize);
    if constexpr (Ram) {
        parcel.write_bytes(ram, RamSize);
    }
}

template<uint32_t RomSize, uint32_t RamSize, bool Battery>
void Mbc5<RomSize, RamSize, Battery>::load_state(Parcel &parcel) {
    ram_enabled = parcel.read_bool();
    rom_bank_selector = parcel.read_uint16();
    ram_bank_selector = parcel.read_uint8();
    parcel.read_bytes(rom, RomSize);
    if constexpr (Ram) {
        parcel.read_bytes(ram, RamSize);
    }
}

template <uint32_t RomSize, uint32_t RamSize, bool Battery>
void Mbc5<RomSize, RamSize, Battery>::reset() {
    ram_enabled = false;
    rom_bank_selector = 0b1;
    ram_bank_selector = 0;

    memset(ram, 0, RamSize);
}
