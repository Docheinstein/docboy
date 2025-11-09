#ifndef CARTRIDGEHEADER_H
#define CARTRIDGEHEADER_H

#include <cstdint>

struct CartridgeHeader {
    uint8_t entry_point[4];
    uint8_t nintendo_logo[48];
    uint8_t title[16];
    uint8_t new_licensee_code[2];
    uint8_t sgb_flag;
    uint8_t cartridge_type;
    uint8_t rom_size;
    uint8_t ram_size;
    uint8_t destination_code;
    uint8_t old_licensee_code;
    uint8_t rom_version_number;
    uint8_t header_checksum;
    uint8_t global_checksum[2];

    uint8_t cgb_flag() const {
        return title[15];
    }

#ifdef ENABLE_CGB
    uint8_t title_checksum() const {
        uint8_t cksum = 0;
        for (const uint8_t c : title) {
            cksum += c;
        }
        return cksum;
    }
#endif
};

static_assert(sizeof(CartridgeHeader) == 0x50);

#endif // CARTRIDGEHEADER_H