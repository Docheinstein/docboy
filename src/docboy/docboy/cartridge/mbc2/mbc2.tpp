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
 * S := rom_bank_selector (4 bits)
 *
 *                   | Bank number  | Address in bank
 * Accessed address  | 17:14        | 13:0
 * 0x0000 - 0x3FFF   | 0            | A[0:13]
 * 0x4000 - 0x7FFF   | S            | A[0:13]
 */

/*
 * RAM access.
 *
 * A := address
 *
 *                   | Address in bank
 * Accessed address  | 8:0
 * 0xA000 - 0xBFFF   | A[0:8]
 */

template<uint32_t RomSize, bool Battery>
Mbc2<RomSize, Battery>::Mbc2(const uint8_t *data, uint32_t length) {
    ASSERT(length <= array_size(rom), "Mbc2: actual ROM size (" + std::to_string(length) + ") exceeds nominal ROM size (" +  std::to_string(array_size(rom)) + ")");
    memcpy(rom, data, length);
}

template<uint32_t RomSize, bool Battery>
uint8_t Mbc2<RomSize, Battery>::read_rom(uint16_t address) const {
    ASSERT(address < 0x8000);

    // 0000 - 0x3FFF
    if (address < 0x4000) {
        return rom[address];
    }

    // 4000 - 0x7FFF
    uint32_t rom_address = (rom_bank_selector << 14) | keep_bits<14>(address);
    rom_address = mask_by_pow2<RomSize>(rom_address);
    return rom[rom_address];
}

template<uint32_t RomSize, bool Battery>
void Mbc2<RomSize, Battery>::write_rom(uint16_t address, uint8_t value) {
    ASSERT(address < 0x8000);

    if (address < 0x4000) {
        if (test_bit<8>(address)) {
            rom_bank_selector = keep_bits<4>(value);
            rom_bank_selector = rom_bank_selector > 0 ? rom_bank_selector : 0b1;
        } else {
            ram_enabled = keep_bits<4>(value) == 0xA;
        }
        return;
    }
}

template<uint32_t RomSize, bool Battery>
uint8_t Mbc2<RomSize, Battery>::read_ram(uint16_t address) const {
    ASSERT(address >= 0xA000 && address < 0xC000);

    // 0xA000 - 0xBFFF
    if (ram_enabled) {
        uint16_t ram_address = keep_bits<9>(address);
        return 0xF0 | keep_bits<4>(ram[ram_address]);
    }

    return 0xFF;
}

template<uint32_t RomSize, bool Battery>
void Mbc2<RomSize, Battery>::write_ram(uint16_t address, uint8_t value) {
    ASSERT(address >= 0xA000 && address < 0xC000);

    // 0xA000 - 0xBFFF
    if (ram_enabled) {
        uint16_t ram_address = keep_bits<9>(address);
        ram[ram_address] = value;
    }
}


template<uint32_t RomSize, bool Battery>
uint8_t *Mbc2<RomSize, Battery>::get_ram_save_data() {
    if constexpr (Battery) {
        return ram;
    }
    return nullptr;
}

template<uint32_t RomSize, bool Battery>
uint32_t Mbc2<RomSize, Battery>::get_ram_save_size() const {
    if constexpr (Battery) {
        return RamSize;
    }
    return 0;
}

#ifdef ENABLE_DEBUGGER
template<uint32_t RomSize, bool Battery>
uint8_t *Mbc2<RomSize, Battery>::get_rom_data() {
    return rom;
}

template<uint32_t RomSize, bool Battery>
uint32_t Mbc2<RomSize, Battery>::get_rom_size() const {
    return RomSize;
}
#endif

template<uint32_t RomSize, bool Battery>
void Mbc2<RomSize, Battery>::save_state(Parcel &parcel) const {
    parcel.write_bool(ram_enabled);
    parcel.write_uint8(rom_bank_selector);
    parcel.write_bytes(rom, RomSize);
    parcel.write_bytes(ram, RamSize);
}

template<uint32_t RomSize, bool Battery>
void Mbc2<RomSize, Battery>::load_state(Parcel &parcel) {
    ram_enabled = parcel.read_bool();
    rom_bank_selector = parcel.read_uint8();
    parcel.read_bytes(rom, RomSize);
    parcel.read_bytes(ram, RamSize);
}

template <uint32_t RomSize, bool Battery>
void Mbc2<RomSize, Battery>::reset() {
    ram_enabled = false;
    rom_bank_selector = 0b1;

    memset(ram, 0, RamSize);
}
