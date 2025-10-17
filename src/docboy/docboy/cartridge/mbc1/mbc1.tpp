#include <cstring>

#include "utils/arrays.h"
#include "utils/asserts.h"
#include "utils/bits.h"
#include "utils/exceptions.h"
#include "utils/formatters.h"
#include "utils/parcel.h"

/*
 * ROM access.
 *
 * A := address
 * U := upper_rom_bank_selector__ram_bank_selector (2 bits)
 * L := rom_bank_selector (5 bits)
 *
 *                   |             | Bank number   | Address in bank
 * Accessed address  | banking_mode | 20:19 | 18:14 | 13:0
 * 0x0000 - 0x3FFF   | 0           | 0     | 0     | A[0:13]
 * 0x0000 - 0x3FFF   | 1           | U     | 0     | A[0:13]
 * 0x4000 - 0x7FFF   | 0/1         | U     | L     | A[0:13]
 *
 * MBC1M (multicart) works the same way as MBC1 but uses 4 bits for the RomBankSelector.
 *
 *                   |             | Bank number   | Address in bank
 * Accessed address  | banking_mode | 19:18 | 17:14 | 13:0
 * 0x0000 - 0x3FFF   | 0           | 0     | 0     | A[0:13]
 * 0x0000 - 0x3FFF   | 1           | U     | 0     | A[0:13]
 * 0x4000 - 0x7FFF   | 0/1         | U     | L     | A[0:13]
 */

/*
 * RAM access.
 *
 * A := address
 * U := upper_rom_bank_selector__ram_bank_selector (2 bits)
 *
 *                   |             | Bank number   | Address in bank
 * Accessed address  | banking_mode | 14:13         | 12:0
 * 0xA000 - 0xBFFF   | 0           | 0             | A[0:12]
 * 0xA000 - 0xBFFF   | 1           | U             | A[0:12]
 */

template <uint32_t RomSize, uint32_t RamSize, bool Battery, uint8_t RomBankSelectorBits>
Mbc1<RomSize, RamSize, Battery, RomBankSelectorBits>::Mbc1(const uint8_t* data, uint32_t length) {
    ASSERT(length <= array_size(rom), "Mbc1: actual ROM size (" + std::to_string(length) +
                                          ") exceeds nominal ROM size (" + std::to_string(array_size(rom)) + ")");
    memcpy(rom, data, length);
}

template <uint32_t RomSize, uint32_t RamSize, bool Battery, uint8_t RomBankSelectorBits>
uint8_t Mbc1<RomSize, RamSize, Battery, RomBankSelectorBits>::read_rom(uint16_t address) const {
    ASSERT(address < 0x8000);

    uint32_t rom_bank_address {};

    if (address < 0x4000) {
        // 0000 - 0x3FFF
        if (banking_mode == 0b1) {
            rom_bank_address = upper_rom_bank_selector_ram_bank_selector << RomBankSelectorBits;
        }
    } else {
        // 4000 - 0x7FFF
        ASSERT(rom_bank_selector != 0 || RomBankSelectorBits < 5);
        rom_bank_address = upper_rom_bank_selector_ram_bank_selector << RomBankSelectorBits | rom_bank_selector;
    }

    uint32_t rom_address = (rom_bank_address << 14) | keep_bits<14>(address);
    rom_address = mask_by_pow2<RomSize>(rom_address);
    return rom[rom_address];
}

template <uint32_t RomSize, uint32_t RamSize, bool Battery, uint8_t RomBankSelectorBits>
void Mbc1<RomSize, RamSize, Battery, RomBankSelectorBits>::write_rom(uint16_t address, uint8_t value) {
    ASSERT(address < 0x8000);

    if (address < 0x4000) {
        // 0000 - 0x1FFF
        if (address < 0x2000) {
            ram_enabled = keep_bits<4>(value) == 0xA;
            return;
        }
        // 0x2000 - 0x3FFF
        rom_bank_selector = keep_bits<5 /* always 5 bits: not related to RomBankSelectorBits */>(value) > 0
                                ? keep_bits<RomBankSelectorBits>(value)
                                : 0b1;
        return;
    }

    // 0x4000 - 0x5FFF
    if (address < 0x6000) {
        upper_rom_bank_selector_ram_bank_selector = keep_bits<2>(value);
        return;
    }
    // 0x6000 - 0x7FFF
    banking_mode = keep_bits<1>(value);
}

template <uint32_t RomSize, uint32_t RamSize, bool Battery, uint8_t RomBankSelectorBits>
uint8_t Mbc1<RomSize, RamSize, Battery, RomBankSelectorBits>::read_ram(uint16_t address) const {
    ASSERT(address >= 0xA000 && address < 0xC000);

    // 0xA000 - 0xBFFF
    if constexpr (Ram) {
        if (ram_enabled) {
            uint32_t ram_address = keep_bits<13>(address);
            if (banking_mode == 0x1) {
                const uint32_t ram_bank_address = (upper_rom_bank_selector_ram_bank_selector << 13);
                ram_address = ram_bank_address | ram_address;
            }
            ram_address = mask_by_pow2<RamSize>(ram_address);
            return ram[ram_address];
        }
    }

    return 0xFF;
}

template <uint32_t RomSize, uint32_t RamSize, bool Battery, uint8_t RomBankSelectorBits>
void Mbc1<RomSize, RamSize, Battery, RomBankSelectorBits>::write_ram(uint16_t address, uint8_t value) {
    ASSERT(address >= 0xA000 && address < 0xC000);

    // 0xA000 - 0xBFFF
    if constexpr (Ram) {
        if (ram_enabled) {
            uint32_t ram_address = keep_bits<13>(address);
            if (banking_mode == 0x1) {
                const uint32_t ram_bank_address = (upper_rom_bank_selector_ram_bank_selector << 13);
                ram_address = ram_bank_address | ram_address;
            }
            ram_address = mask_by_pow2<RamSize>(ram_address);
            ram[ram_address] = value;
        }
    }
}

template <uint32_t RomSize, uint32_t RamSize, bool Battery, uint8_t RomBankSelectorBits>
void* Mbc1<RomSize, RamSize, Battery, RomBankSelectorBits>::get_ram_save_data() {
    if constexpr (Ram && Battery) {
        return ram;
    }
    return nullptr;
}

template <uint32_t RomSize, uint32_t RamSize, bool Battery, uint8_t RomBankSelectorBits>
uint32_t Mbc1<RomSize, RamSize, Battery, RomBankSelectorBits>::get_ram_save_size() const {
    if constexpr (Ram && Battery) {
        return RamSize;
    }
    return 0;
}

#ifdef ENABLE_DEBUGGER
template <uint32_t RomSize, uint32_t RamSize, bool Battery, uint8_t RomBankSelectorBits>
uint8_t* Mbc1<RomSize, RamSize, Battery, RomBankSelectorBits>::get_rom_data() {
    return rom;
}

template <uint32_t RomSize, uint32_t RamSize, bool Battery, uint8_t RomBankSelectorBits>
uint32_t Mbc1<RomSize, RamSize, Battery, RomBankSelectorBits>::get_rom_size() const {
    return RomSize;
}
#endif

template <uint32_t RomSize, uint32_t RamSize, bool Battery, uint8_t RomBankSelectorBits>
void Mbc1<RomSize, RamSize, Battery, RomBankSelectorBits>::save_state(Parcel& parcel) const {
    parcel.write_bool(ram_enabled);
    parcel.write_uint8(rom_bank_selector);
    parcel.write_uint8(upper_rom_bank_selector_ram_bank_selector);
    parcel.write_uint8(banking_mode);
    if constexpr (Ram) {
        parcel.write_bytes(ram, RamSize);
    }
}

template <uint32_t RomSize, uint32_t RamSize, bool Battery, uint8_t RomBankSelectorBits>
void Mbc1<RomSize, RamSize, Battery, RomBankSelectorBits>::load_state(Parcel& parcel) {
    ram_enabled = parcel.read_bool();
    rom_bank_selector = parcel.read_uint8();
    upper_rom_bank_selector_ram_bank_selector = parcel.read_uint8();
    banking_mode = parcel.read_uint8();
    if constexpr (Ram) {
        parcel.read_bytes(ram, RamSize);
    }
}

template <uint32_t RomSize, uint32_t RamSize, bool Battery, uint8_t RomBankSelectorBits>
void Mbc1<RomSize, RamSize, Battery, RomBankSelectorBits>::reset() {
    ram_enabled = false;
    rom_bank_selector = 0b1;
    upper_rom_bank_selector_ram_bank_selector = 0;
    banking_mode = 0;
    memset(ram, 0, RamSize);
}
