#include <cstring>

#include "utils/arrays.h"
#include "utils/asserts.h"
#include "utils/bits.h"
#include "utils/parcel.h"

template <uint32_t RomSize, uint32_t RamSize>
HuC1<RomSize, RamSize>::HuC1(const uint8_t* data, uint32_t length) {
    ASSERT(length <= array_size(rom), "HuC1: actual ROM size (" + std::to_string(length) +
                                          ") exceeds nominal ROM size (" + std::to_string(array_size(rom)) + ")");
    memcpy(rom, data, length);
}

template <uint32_t RomSize, uint32_t RamSize>
uint8_t HuC1<RomSize, RamSize>::read_rom(uint16_t address) const {
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

template <uint32_t RomSize, uint32_t RamSize>
void HuC1<RomSize, RamSize>::write_rom(uint16_t address, uint8_t value) {
    ASSERT(address < 0x8000);

    if (address < 0x4000) {
        // 0000 - 0x1FFF
        if (address < 0x2000) {
            // 0xE enables IR mode, everything else enables RAM.
            ir_enabled = keep_bits<4>(value) == 0xE;
            return;
        }
        // 0x2000 - 0x3FFF
        rom_bank_selector = keep_bits<6>(value);
        rom_bank_selector = rom_bank_selector > 0 ? rom_bank_selector : 0b1;
        return;
    }

    // 0x4000 - 0x5FFF
    if (address < 0x6000) {
        ram_bank_selector = keep_bits<2>(value);
    }
}

template <uint32_t RomSize, uint32_t RamSize>
uint8_t HuC1<RomSize, RamSize>::read_ram(uint16_t address) const {
    ASSERT(address >= 0xA000 && address < 0xC000);

    // 0xA000 - 0xBFFF
    if (ir_enabled) {
        // A real cartridge would either read:
        // C0: IR signal not received
        // C1: IR signal received
        // Since we don't have a working implementation of IR, just return C0.
        return 0xC0;
    }

    uint32_t ram_address = (ram_bank_selector << 13) | keep_bits<13>(address);
    ram_address = mask_by_pow2<RamSize>(ram_address);
    return ram[ram_address];
}

template <uint32_t RomSize, uint32_t RamSize>
void HuC1<RomSize, RamSize>::write_ram(uint16_t address, uint8_t value) {
    ASSERT(address >= 0xA000 && address < 0xC000);

    // 0xA000 - 0xBFFF
    if (ir_enabled) {
        // A real cartridge would either write:
        // 00: do not send IR signal
        // 01: send IR signal
        // Since we don't have a working implementation of IR, just ignore the value.
    } else {
        uint32_t ram_address = (ram_bank_selector << 13) | keep_bits<13>(address);
        ram_address = mask_by_pow2<RamSize>(ram_address);
        ram[ram_address] = value;
    }
}

template <uint32_t RomSize, uint32_t RamSize>
void* HuC1<RomSize, RamSize>::get_ram_save_data() {
    return ram;
}

template <uint32_t RomSize, uint32_t RamSize>
uint32_t HuC1<RomSize, RamSize>::get_ram_save_size() const {
    return RamSize;
}

#ifdef ENABLE_DEBUGGER
template <uint32_t RomSize, uint32_t RamSize>
uint8_t* HuC1<RomSize, RamSize>::get_rom_data() {
    return rom;
}

template <uint32_t RomSize, uint32_t RamSize>
uint32_t HuC1<RomSize, RamSize>::get_rom_size() const {
    return RomSize;
}
#endif

template <uint32_t RomSize, uint32_t RamSize>
void HuC1<RomSize, RamSize>::save_state(Parcel& parcel) const {
    PARCEL_WRITE_BOOL(parcel, ir_enabled);
    PARCEL_WRITE_UINT8(parcel, rom_bank_selector);
    PARCEL_WRITE_UINT8(parcel, ram_bank_selector);
    PARCEL_WRITE_BYTES(parcel, ram, RamSize);
}

template <uint32_t RomSize, uint32_t RamSize>
void HuC1<RomSize, RamSize>::load_state(Parcel& parcel) {
    ir_enabled = parcel.read_bool();
    rom_bank_selector = parcel.read_uint8();
    ram_bank_selector = parcel.read_uint8();
    parcel.read_bytes(ram, RamSize);
}

template <uint32_t RomSize, uint32_t RamSize>
void HuC1<RomSize, RamSize>::reset() {
    ir_enabled = false;
    rom_bank_selector = 0b1;
    ram_bank_selector = 0;
    memset(ram, 0, RamSize);
}
