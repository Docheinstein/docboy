#include <cstring>
#include "cartridge.h"

bool Cartridge::Header::isValid() const {
    return isNintendoLogoValid() && isHeaderChecksumValid();
}

bool Cartridge::Header::isNintendoLogoValid() const {
    static const uint8_t NINTENDO_LOGO[] = {
        0xCE, 0xED, 0x66, 0x66, 0xCC, 0x0D, 0x00, 0x0B, 0x03, 0x73, 0x00, 0x83, 0x00, 0x0C, 0x00, 0x0D,
        0x00, 0x08, 0x11, 0x1F, 0x88, 0x89, 0x00, 0x0E, 0xDC, 0xCC, 0x6E, 0xE6, 0xDD, 0xDD, 0xD9, 0x99,
        0xBB, 0xBB, 0x67, 0x63, 0x6E, 0x0E, 0xEC, 0xCC, 0xDD, 0xDC, 0x99, 0x9F, 0xBB, 0xB9, 0x33, 0x3E
    };

    return memcmp(nintendo_logo.data(), NINTENDO_LOGO, sizeof(NINTENDO_LOGO)) == 0;
}

bool Cartridge::Header::isHeaderChecksumValid() const {
    uint8_t expected_header_checksum = 0;
    for (uint16_t address = 0x134 - 0x100; address <= 0x14C - 0x100; address++)
        expected_header_checksum = expected_header_checksum - data[address] - 1;

    return expected_header_checksum == header_checksum;
}

Cartridge::Header Cartridge::header() const {
    auto memcpy_range = [](uint8_t *dest, const uint8_t *src, size_t from, size_t to) {
        memcpy(dest, src + from, to - from + 1);
    };

    auto memcpy_range_s = [&memcpy_range](char *dest, const uint8_t * src, size_t from, size_t to) {
        dest[to - from + 1] = '\0';
        memcpy_range((uint8_t *) dest, src, from, to);
    };

    auto memcpy_range_v = [&memcpy_range](std::vector<uint8_t > &dest, const uint8_t * src, size_t from, size_t to) {
        dest.resize(to - from + 1);
        memcpy_range(dest.data(), src, from, to);
    };

    Cartridge::Header h;

    auto raw_data = (uint8_t *) rom.data();

    // Raw data
    memcpy_range_v(h.data, raw_data, 0x100, 0x14F);

    // Entry point (0x100 - 0x103)
    memcpy_range_v(h.entry_point, raw_data, 0x100, 0x103);

    // Nintendo Logo (0x104 - 0x133)
    memcpy_range_v(h.nintendo_logo, raw_data, 0x104, 0x133);

    // Title (0x134 - 0x143)
    char title[0x143 - 0x134 + 1 + sizeof('\0')];
    memcpy_range_s(title, raw_data, 0x134, 0x143);
    h.title = title;

    // CGB flag (0x143)
    h.cgb_flag = raw_data[0x143];

    // New licensee code (0x144 - 0x145)
    memcpy_range_v(h.new_licensee_code, raw_data, 0x144, 0x145);

    // SGB flag (0x146)
    h.sgb_flag = raw_data[0x146];

    // Cartridge type (0x147)
    h.cartridge_type = raw_data[0x147];

    // Cartridge type (0x148)
    h.rom_size = raw_data[0x148];

    // Cartridge type (0x149)
    h.ram_size = raw_data[0x149];

    // Destination code (0x14A)
    h.destination_code = raw_data[0x14A];

    // Old licensee code (0x14B)
    h.old_licensee_code = raw_data[0x14B];

    // ROM version number (0x14C)
    h.rom_version_number = raw_data[0x14C];

    // Header checksum (0x14D)
    h.header_checksum = raw_data[0x14D];

    // Global checksum (0x14E - 0x14F)
    memcpy_range_v(h.global_checksum, raw_data, 0x14E, 0x14F);

    return h;
}


Cartridge::Cartridge(const std::vector<uint8_t> &data) : rom(data) {

}

Cartridge::Cartridge(std::vector<uint8_t> &&data) : rom(data) {

}

